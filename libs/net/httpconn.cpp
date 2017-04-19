// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// httpconn.cpp - dim http
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kDefaultBlockSize = 4096;
const unsigned kDefaultHeaderTableSize = 4096;

const int kMaximumWindowSize = 0x7fff'ffff;


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

enum class FrameType : int8_t {
    kData = 0,
    kHeaders = 1,
    kPriority = 2,
    kRstStream = 3,
    kSettings = 4,
    kPushPromise = 5,
    kPing = 6,
    kGoAway = 7,
    kWindowUpdate = 8,
    kContinuation = 9,
};

enum FrameParam : int16_t {
    kSettingsHeaderTableSize = 1,
    kSettingsEnablePush = 2,
    kSettingsMaxConcurrentStreams = 3,
    kSettingsInitialWindowSize = 4,
    kSettingsMaxFrameSize = 5,
    kSettingsMaxHeaderListSize = 6,
};

struct PriorityData {
    int stream;
    int weight;
    bool exclusive;
};
struct UnpaddedData {
    const char * hdr;
    const char * data;
    int dataLen;
    int padLen;
};

struct ResetStream {
    HttpConnHandle hc;
    int stream;
    shared_ptr<HttpStream> sm;
};

} // namespace

enum class HttpConn::FrameError : int {
    kNoError = 0,
    kProtocolError = 1,
    kInternalError = 2,
    kFlowControlError = 3,
    kSettingsTimeout = 4,
    kStreamClosed = 5,
    kFrameSizeError = 6,
    kRefusedStream = 7,
    kCancel = 8,
    kCompressionError = 9,
    kConnectError = 10,
    kEnhanceYourCalm = 11,
    kInadequateSecurity = 12,
    kHttp11Required = 13,
};

enum HttpConn::FrameFlags : uint8_t {
    fNone = 0x00,
    fAck = 0x01,
    fEndStream = 0x01,
    fEndHeaders = 0x04,
    fPadded = 0x08,
    fPriority = 0x20,
};


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<HttpConnHandle, HttpConn> s_conns;


/****************************************************************************
*
*   Host to/from network translation
*
***/

//===========================================================================
static inline int ntoh16(const char frame[]) {
    return ((uint32_t)(uint8_t)frame[0] << 8) 
        + (uint8_t)frame[1];
}

//===========================================================================
static inline int ntoh24(const char frame[]) {
    return ((uint32_t)(uint8_t)frame[0] << 16) 
        + ((uint32_t)(uint8_t)frame[1] << 8)
        + (uint8_t)frame[2];
}

//===========================================================================
static inline int ntoh32(const char frame[]) {
    return ((uint32_t)(uint8_t)frame[0] << 24) 
        + ((uint32_t)(uint8_t)frame[1] << 16)
        + ((uint32_t)(uint8_t)frame[2] << 8) 
        + (uint8_t)frame[3];
}

//===========================================================================
static inline int ntoh31(const char frame[]) {
    return ntoh32(frame) & 0x7fff'ffff;
}

//===========================================================================
static inline char * hton16(char * out, int val) {
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
static inline char * hton24(char * out, int val) {
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
static inline char * hton32(char * out, int val) {
    *out++ = (unsigned(val) >> 24) & 0xff;
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

//===========================================================================
static inline char * hton31(char * out, int val) {
    return hton32(out, unsigned(val) & 0x7fff'ffff);
}


/****************************************************************************
*
*   Reset streams
*
***/

const Duration kMaxResetStreamAge = 10s;
const Duration kMinResetStreamCheckInterval = 2s;

namespace {
class ResetStreamTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};
} // namespace
static deque<ResetStream> s_resetStreams;
static ResetStreamTimer s_resetTimer;

//===========================================================================
Duration ResetStreamTimer::onTimer(TimePoint now) {
    TimePoint expire = now - kMaxResetStreamAge;
    Duration sleep;
    while (!empty(s_resetStreams)) {
        auto & rs = s_resetStreams.front();
        sleep = rs.sm->m_closed - expire;
        if (sleep > 0s)
            return min(sleep, kMinResetStreamCheckInterval);

        if (rs.sm->m_state == HttpStream::kClosed) {
            HttpConn * conn = s_conns.find(rs.hc);
            conn->deleteStream(rs.stream, rs.sm.get());
        } else {
            assert(rs.sm->m_state == HttpStream::kDeleted);
        }

        s_resetStreams.pop_front();
    }

    return kTimerInfinite;
}


/****************************************************************************
*
*   HPACK message decoder
*
***/

namespace {
class MsgDecoder : public IHpackDecodeNotify {
public:
    MsgDecoder(HttpMsg & msg);

    explicit operator bool() const;
    HttpConn::FrameError Error() const;

private:
    void onHpackHeader(
        HttpHdr id,
        const char name[],
        const char value[],
        HpackFlags flags
    ) override;

    HttpMsg & m_msg;
    HttpConn::FrameError m_error{HttpConn::FrameError::kNoError};
};
} // namespace

//===========================================================================
MsgDecoder::MsgDecoder(HttpMsg & msg)
    : m_msg(msg) {}

//===========================================================================
void MsgDecoder::onHpackHeader(
    HttpHdr id,
    const char name[],
    const char value[],
    HpackFlags flags
) {
    if (id) {
        m_msg.addHeaderRef(id, value);
    } else {
        m_msg.addHeaderRef(name, value);
    }
}

//===========================================================================
MsgDecoder::operator bool() const {
    return m_error == HttpConn::FrameError::kNoError;
}

//===========================================================================
HttpConn::FrameError MsgDecoder::Error() const {
    return m_error;
}


/****************************************************************************
*
*   Frame header
*
***/

const int kFrameHeaderLen = 9;
const char kPrefaceData[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

//===========================================================================
static void setFrameHeader(
    uint8_t out[kFrameHeaderLen],
    unsigned stream,
    FrameType type,
    size_t length,
    HttpConn::FrameFlags flags
) {
    // Frame header
    //  length : 24
    //  type : 8
    //  flags : 8
    //  reserved : 1
    //  stream : 31

    assert(length <= 0xff'ffff);
    assert((stream >> 31) == 0);
    out[0] = (uint8_t)(length >> 16);
    out[1] = (uint8_t)(length >> 8);
    out[2] = (uint8_t)length;
    out[3] = (uint8_t)type;
    out[4] = (uint8_t)flags;
    out[5] = (uint8_t)(stream >> 24);
    out[6] = (uint8_t)(stream >> 16);
    out[7] = (uint8_t)(stream >> 8);
    out[8] = (uint8_t)stream;
}

//===========================================================================
static void startFrame(
    CharBuf * out,
    unsigned stream,
    FrameType type,
    size_t length,
    HttpConn::FrameFlags flags
) {
    uint8_t buf[kFrameHeaderLen];
    setFrameHeader(buf, stream, type, length, flags);
    out->append((char *)&buf, sizeof(buf));
}

//===========================================================================
static int getFrameLen(const char frame[kFrameHeaderLen]) {
    return ntoh24(frame);
}

//===========================================================================
static FrameType getFrameType(const char frame[kFrameHeaderLen]) {
    return (FrameType) frame[3];
}

//===========================================================================
static HttpConn::FrameFlags getFrameFlags(const char frame[kFrameHeaderLen]) {
    return (HttpConn::FrameFlags) frame[4];
}

//===========================================================================
static int getFrameStream(const char frame[kFrameHeaderLen]) {
    return ntoh31(frame + 5);
}


/****************************************************************************
*
*   Connection initialization
*
***/

enum class HttpConn::ByteMode : int {
    kInvalid,
    kPreface,
    kHeader,
    kPayload,
};

enum class HttpConn::FrameMode : int {
    kSettings,
    kNormal,
    kContinuation,
};

//===========================================================================
HttpConn::HttpConn()
    : m_byteMode{ByteMode::kPreface}
    , m_frameMode{FrameMode::kSettings}
    , m_encoder{kDefaultHeaderTableSize}
    , m_decoder{kDefaultHeaderTableSize}
    , m_nextOutputStream(2) 
{}

//===========================================================================
void HttpConn::connect(CharBuf * out, HttpConnHandle h) {
    assert(m_byteMode == ByteMode::kPreface);
    m_handle = h;
    m_outgoing = true;
    m_nextOutputStream = 1;
    out->append(kPrefaceData);
    startFrame(out, 0, FrameType::kSettings, 0, {});
    m_unackSettings += 1;
    m_byteMode = ByteMode::kHeader;
}

//===========================================================================
void HttpConn::accept(HttpConnHandle h) {
    assert(m_byteMode == ByteMode::kPreface);
    m_handle = h;
}


/****************************************************************************
*
*   Receiving data
*
***/

//===========================================================================
static bool skip(const char *& ptr, const char * eptr, const char literal[]) {
    while (ptr != eptr && *literal) {
        if (*ptr++ != *literal++)
            false;
    }
    return true;
}

//===========================================================================
static void replyGoAway(
    CharBuf * out, 
    int lastStream, 
    HttpConn::FrameError error,
    string_view msg = {}
) {
    // GoAway frame
    //  reserved : 1
    //  lastStreamId : 31
    //  errorCode : 32
    //  data[]

    msg = msg.substr(0, 256); 
    char buf[8];
    hton31(buf, lastStream);
    hton32(buf + 4, (int) error);
    startFrame(out, 0, FrameType::kGoAway, sizeof(buf) + msg.size(), {});
    out->append(buf, sizeof(buf));
    out->append(msg);
}

//===========================================================================
static void replyWindowUpdate(
    CharBuf * out,
    int stream,
    int increment
) {
    // WindowUpdate frame
    //  reserved : 1
    //  increment : 31

    char buf[4];
    hton31(buf, increment);
    startFrame(out, stream, FrameType::kWindowUpdate, sizeof(buf), {});
    out->append(buf, sizeof(buf));
}

//===========================================================================
static bool removePadding(
    UnpaddedData * out,
    const char src[],
    int frameLen,
    int hdrLen,
    HttpConn::FrameFlags flags
) {
    out->hdr = src + kFrameHeaderLen;
    out->data = out->hdr + hdrLen;
    out->dataLen = frameLen - hdrLen;
    if (~flags & HttpConn::fPadded) {
        out->padLen = 0;
        return true;
    }

    if (out->dataLen) {
        out->padLen = *out->data;
        out->dataLen -= out->padLen + 1;
    }
    if (out->dataLen <= 0)
        return false;
    out->hdr += 1;
    out->data += 1;

    // verify that the padding is zero filled
    const char * ptr = out->data + out->dataLen;
    const char * term = ptr + out->padLen;
    for (; ptr != term; ++ptr) {
        if (*ptr)
            return false;
    }

    return true;
}

//===========================================================================
static bool removePriority(
    PriorityData * out, 
    int stream, 
    const char hdr[], 
    int hdrLen
) {
    if (hdrLen < 5)
        return false;
    unsigned tmp = ntoh32(hdr);
    out->exclusive = tmp & 0x80000000;
    out->stream = tmp & 0x7fffffff;
    out->weight = (unsigned)hdr[4] + 1;
    if (out->stream == stream)
        return false;
    return true;
}

//===========================================================================
static void updatePriority() {
}

//===========================================================================
bool HttpConn::recv(
    CharBuf * out,
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    const void * src,
    size_t srcLen
) {
    const char * ptr = static_cast<const char *>(src);
    const char * eptr = ptr + srcLen;
    size_t avail;
    switch (m_byteMode) {
    case ByteMode::kPreface:
        if (!skip(ptr, eptr, kPrefaceData + size(m_input)))
            return false;
        if (ptr == eptr) {
            m_input.insert(m_input.end(), ptr - srcLen, eptr);
            return true;
        }

        // Servers must send a settings frame after the connection preface
        if (!m_outgoing) {
            startFrame(out, 0, FrameType::kSettings, 0, {});
            m_unackSettings += 1;
        }

        m_byteMode = ByteMode::kHeader;
        [[fallthrough]];

    case ByteMode::kHeader:
    next_frame:
        avail = eptr - ptr;
        if (!avail)
            return true;
        if (size_t used = size(m_input)) {
            size_t need = kFrameHeaderLen - used;
            if (avail < need) {
                m_input.insert(m_input.end(), ptr, eptr);
                return true;
            }
            m_input.insert(m_input.end(), ptr, ptr + need);
            ptr += need;
            avail -= need;
            m_inputFrameLen = getFrameLen(data(m_input));
            if (avail < m_inputFrameLen) {
                if (m_inputFrameLen > m_maxInputFrame)
                    return onFrame(msgs, out, data(m_input));
                m_input.insert(m_input.end(), ptr, eptr);
                m_byteMode = ByteMode::kPayload;
                return true;
            }
            m_input.insert(m_input.end(), ptr, ptr + m_inputFrameLen);
            ptr += m_inputFrameLen;
            if (!onFrame(msgs, out, data(m_input)))
                return false;
            m_input.clear();
            goto next_frame;
        }

        if (avail < kFrameHeaderLen) {
            m_input.assign(ptr, eptr);
            return true;
        }
        avail -= kFrameHeaderLen;
        m_inputFrameLen = getFrameLen(ptr);
        if (avail < m_inputFrameLen && m_inputFrameLen <= m_maxInputFrame) {
            m_input.assign(ptr, eptr);
            return true;
        }
        if (!onFrame(msgs, out, ptr))
            return false;
        ptr += kFrameHeaderLen + m_inputFrameLen;
        goto next_frame;

    default:
        assert(m_byteMode == ByteMode::kPayload);
        [[fallthrough]];

    case ByteMode::kPayload:
        avail = eptr - ptr;
        size_t used = size(m_input);
        size_t need = kFrameHeaderLen + m_inputFrameLen - used;
        if (avail < need) {
            m_input.insert(m_input.end(), ptr, eptr);
            return true;
        }
        m_input.insert(m_input.end(), ptr, ptr + need);
        ptr += need;
        if (!onFrame(msgs, out, data(m_input)))
            return false;
        m_input.clear();
        m_byteMode = ByteMode::kHeader;
        goto next_frame;
    };
}

//===========================================================================
void HttpConn::replyRstStream(CharBuf * out, int stream, FrameError error) {
    // RstStream frame
    //  errorCode : 32

    char buf[4];
    hton32(buf, (int) error);
    startFrame(out, stream, FrameType::kRstStream, 4, {});
    out->append(buf, sizeof(buf));
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) 
        return;

    auto sm = it->second;
    sm->m_state = HttpStream::kClosed;
    sm->m_closed = Clock::now();
    ResetStream rs;
    rs.hc = m_handle;
    rs.stream = stream;
    rs.sm = sm;
    s_resetStreams.push_back(rs);
    timerUpdate(&s_resetTimer, kMaxResetStreamAge, true);
}

//===========================================================================
HttpStream * HttpConn::findAlways(CharBuf * out, int stream) {
    auto ib = m_streams.emplace(stream, nullptr);
    if (!ib.second)
        return ib.first->second.get();
    if (stream % 2 == 0) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return nullptr;
    }
    if (stream < m_lastInputStream) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return nullptr;
    }

    m_lastInputStream = stream;
    ib.first->second = make_shared<HttpStream>();
    return ib.first->second.get();
}

//===========================================================================
bool HttpConn::onFrame(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[]
) {
    // Frame header
    //  length : 24
    //  type : 8
    //  flags : 8
    //  reserved : 1
    //  stream : 31

    auto * hdr = src;
    auto type = getFrameType(hdr);
    auto flags = getFrameFlags(hdr);
    int stream = getFrameStream(hdr);
    m_inputFrameLen = getFrameLen(hdr);
    if (m_inputFrameLen > m_maxInputFrame) {
        replyGoAway(out, m_lastInputStream, FrameError::kFrameSizeError);
        return false;
    }

    switch (type) {
    case FrameType::kContinuation:
        return onContinuation(msgs, out, src, stream, flags);
    case FrameType::kData: return onData(msgs, out, src, stream, flags);
    case FrameType::kGoAway: return onGoAway(msgs, out, src, stream, flags);
    case FrameType::kHeaders: return onHeaders(msgs, out, src, stream, flags);
    case FrameType::kPing: return onPing(msgs, out, src, stream, flags);
    case FrameType::kPriority:
        return onPriority(msgs, out, src, stream, flags);
    case FrameType::kPushPromise:
        return onPushPromise(msgs, out, src, stream, flags);
    case FrameType::kRstStream:
        return onRstStream(msgs, out, src, stream, flags);
    case FrameType::kSettings:
        return onSettings(msgs, out, src, stream, flags);
    case FrameType::kWindowUpdate:
        return onWindowUpdate(msgs, out, src, stream, flags);
    };

    // ignore unknown frames unless a specific frame type is required
    if (m_frameMode == FrameMode::kNormal)
        return true;
    replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
    return false;
}

//===========================================================================
bool HttpConn::onData(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Data frame
    //  if PADDED
    //      padLen : 8
    //  data[]
    //  padding[]

    if (m_frameMode != FrameMode::kNormal || !stream) {
        // data frames aren't allowed on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    // adjust for any included padding
    UnpaddedData data;
    if (!removePadding(&data, src, m_inputFrameLen, 0, flags)) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    // TODO: flow control
    if (data.dataLen) {
        replyWindowUpdate(out, stream, data.dataLen);
        replyWindowUpdate(out, 0, data.dataLen);
    }

    // TODO: check total buffer size

    shared_ptr<HttpStream> sm;
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) 
        sm = it->second;
    if (!sm
        || sm->m_state != HttpStream::kOpen
            && sm->m_state != HttpStream::kLocalClosed) {
        // data frame on non-open stream
        replyRstStream(out, stream, FrameError::kStreamClosed);
        return true;
    }

    CharBuf & buf = sm->m_msg->body();
    buf.append(data.data, data.dataLen);
    if (flags & fEndStream) {
        msgs->push_back(move(sm->m_msg));
    }
    return true;
}

//===========================================================================
bool HttpConn::onHeaders(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Headers frame
    //  if PADDED flag
    //      padLen : 8
    //  if PRIORITY flag
    //      exclusive dependency : 1
    //      stream dependency : 31
    //      weight : 8
    //  headerBlock[]
    //  padding[]

    if (m_frameMode != FrameMode::kNormal || !stream) {
        // header frames aren't allowed on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    // adjust for any included padding
    UnpaddedData ud;
    int hdrLen = (flags & fPriority) ? 5 : 0;
    if (!removePadding(&ud, src, m_inputFrameLen, hdrLen, flags)) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    // parse priority
    if (flags & fPriority) {
        PriorityData pri;
        if (!removePriority(&pri, stream, ud.hdr, hdrLen)) {
            replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
            return false;
        }
        updatePriority();
    }

    // find stream
    auto sm = findAlways(out, stream);
    if (!sm)
        return false;

    switch (sm->m_state) {
    case HttpStream::kIdle:
        if (~flags & fEndStream) {
            sm->m_state = HttpStream::kOpen;
        } else {
            sm->m_state = HttpStream::kRemoteClosed;
        }
        sm->m_msg = make_unique<HttpRequest>(stream);
        break;
    case HttpStream::kRemoteReserved:
        if (~flags & fEndStream) {
            sm->m_state = HttpStream::kLocalClosed;
        } else {
            sm->m_state = HttpStream::kClosed;
        }
        sm->m_msg = make_unique<HttpResponse>(stream);
        break;
    case HttpStream::kOpen:
    case HttpStream::kLocalClosed:
        if (flags & fEndStream) {
            // trailing headers not supported
            // !!! should probably send a stream error and process
            //     the headers (to maintain the connection decompression
            //     context) instead of dropping the connection.
            replyGoAway(out, stream, FrameError::kProtocolError);
            return false;
        }
        [[fallthrough]];
    default:
        // !!! should probably send a stream error and process
        //     the headers (to maintain the connection decompression
        //     context) instead of dropping the connection.
        replyGoAway(out, stream, FrameError::kProtocolError);
        return false;
    }

    if (~flags & fEndHeaders)
        m_frameMode = FrameMode::kContinuation;

    auto * msg = sm->m_msg.get();
    MsgDecoder notify(*msg);
    if (!m_decoder.parse(&notify, &msg->heap(), ud.data, ud.dataLen)) {
        replyGoAway(out, m_lastInputStream, FrameError::kCompressionError);
        return false;
    }
    if (!notify) {
        replyRstStream(out, stream, notify.Error());
    } else {
        if (sm->m_state == HttpStream::kRemoteClosed)
            msgs->push_back(move(sm->m_msg));
    }
    return true;
}

//===========================================================================
bool HttpConn::onPriority(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Priority frame
    //  exclusive dependency : 1
    //  stream dependency : 31
    //  weight : 8

    if (m_frameMode != FrameMode::kNormal || !stream) {
        // priority frames aren't allowed on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    if (m_inputFrameLen != 5) {
        replyRstStream(out, stream, FrameError::kFrameSizeError);
        return true;
    }

    PriorityData pri;
    if (!removePriority(
        &pri, 
        stream, 
        src + kFrameHeaderLen, 
        m_inputFrameLen
    )) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    return true;
}

//===========================================================================
bool HttpConn::onRstStream(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // RstStream frame
    //  errorCode : 32

    if (m_frameMode != FrameMode::kNormal || !stream) {
        // priority frames aren't allowed on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    if (m_inputFrameLen != 4) {
        replyGoAway(out, m_lastInputStream, FrameError::kFrameSizeError);
        return false;
    }

    // TODO: actually close the stream...

    return true;
}

//===========================================================================
bool HttpConn::onSettings(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Settings frame
    //  array of 0 or more
    //      identifier : 16
    //      value : 32

    if (m_frameMode != FrameMode::kNormal
            && m_frameMode != FrameMode::kSettings
        || stream) {
        // settings frames MUST be on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }
    m_frameMode = FrameMode::kNormal;

    if (flags & fAck) {
        if (m_inputFrameLen) {
            replyGoAway(out, m_lastInputStream, FrameError::kFrameSizeError);
            return false;
        }
        if (!m_unackSettings) {
            replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
            return false;
        }
        m_unackSettings -= 1;
        return true;
    }

    // must be an even multiple of identifier/value pairs
    if (m_inputFrameLen % 6) {
        replyGoAway(out, m_lastInputStream, FrameError::kFrameSizeError);
        return false;
    }

    for (int pos = 0; pos < m_inputFrameLen; pos += 6) {
        unsigned identifier = ntoh16(src + kFrameHeaderLen + pos);
        unsigned value = ntoh32(src + kFrameHeaderLen + pos + 2);
        (void)identifier;
        (void)value;
    }

    startFrame(out, 0, FrameType::kSettings, 0, fAck);
    return true;
}

//===========================================================================
bool HttpConn::onPushPromise(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // PushPromise frame
    //  if PADDED flag
    //      padLen : 8
    //  reserved : 1
    //  stream : 31
    //  headerBlock[]
    //  padding[]

    return false;
}

//===========================================================================
bool HttpConn::onPing(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Ping frame
    //  data[8]

    return false;
}

//===========================================================================
bool HttpConn::onGoAway(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // GoAway frame
    //  reserved : 1
    //  lastStreamId : 31
    //  errorCode : 32
    //  data[]

    if (!stream) {
        // goaway frames aren't allowed on stream 0
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    int lastStreamId = ntoh31(src + kFrameHeaderLen);
    FrameError errorCode = (FrameError)ntoh32(src + kFrameHeaderLen + 4);

    if (lastStreamId > m_lastOutputStream) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    m_lastOutputStream = lastStreamId;
    if (errorCode == FrameError::kNoError) {
        if (m_lastOutputStream > m_nextOutputStream) {
            // TODO: report shutdown requested
            return true;
        }
    }

    replyGoAway(out, m_lastInputStream, FrameError::kNoError);
    return false;
}

//===========================================================================
bool HttpConn::onWindowUpdate(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // WindowUpdate frame
    //  reserved : 1
    //  increment : 31

    if (m_frameMode != FrameMode::kNormal) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }
    if (m_inputFrameLen != 4) {
        replyGoAway(out, m_lastInputStream, FrameError::kFrameSizeError);
        return false;
    }

    int increment = ntoh31(src + kFrameHeaderLen);
    if (!increment) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    if (!stream) {
        // update window of connection
        if (kMaximumWindowSize - increment < m_flowWindow) {
            replyGoAway(out, m_lastInputStream, FrameError::kFlowControlError);
            return false;
        }
        m_flowWindow += increment;
        // TODO: release blocked data frames
        return true;
    }

    // update window of individual stream
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) {
        if (stream > m_lastInputStream) {
            replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
            return false;
        }
        // ignore window updates on completed streams
        return true;
    }

    auto sm = it->second.get();
    if (kMaximumWindowSize - increment < sm->m_flowWindow) {
        replyRstStream(out, stream, FrameError::kFlowControlError);
        return true;
    }
    sm->m_flowWindow += increment;
    // TODO: release blocked data frames
    return true;
}

//===========================================================================
bool HttpConn::onContinuation(
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    CharBuf * out,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Continuation frame
    //  headerBlock[]

    if (m_frameMode != FrameMode::kContinuation
        || stream != m_continueStream) {
        replyGoAway(out, m_lastInputStream, FrameError::kProtocolError);
        return false;
    }

    return false;
}


/****************************************************************************
*
*   Sending data
*
***/

//===========================================================================
void HttpConn::writeMsg(
    CharBuf * out, 
    int stream, 
    const HttpMsg & msg, 
    bool more
) {
    const CharBuf & body = msg.body();
    size_t framePos = out->size();
    size_t maxEndPos = framePos + m_maxOutputFrame + kFrameHeaderLen;
    auto ftype = FrameType::kHeaders;
    auto flags = (!body.empty() || more) ? (FrameFlags) 0 : fEndStream;
    uint8_t frameHdr[kFrameHeaderLen];
    setFrameHeader(frameHdr, stream, ftype, 0, {});
    out->append((char *)frameHdr, size(frameHdr));
    m_encoder.startBlock(out);
    for (auto && hdr : msg.headers()) {
        for (auto && hv : hdr) {
            if (hdr.m_id) {
                m_encoder.header(hdr.m_id, hv.m_value);
            } else {
                m_encoder.header(hdr.m_name, hv.m_value);
            }
            while (size(*out) > maxEndPos) {
                setFrameHeader(
                    frameHdr, stream, ftype, m_maxOutputFrame, flags);
                out->replace(
                    framePos,
                    size(frameHdr),
                    (char *)frameHdr,
                    size(frameHdr));
                ftype = FrameType::kContinuation;
                flags = {};
                framePos = maxEndPos;
                maxEndPos += m_maxOutputFrame + kFrameHeaderLen;
                out->insert(framePos, (char *)frameHdr, size(frameHdr));
            }
        }
    }
    setFrameHeader(
        frameHdr,
        stream,
        ftype,
        int(size(*out) - framePos - kFrameHeaderLen),
        flags | fEndHeaders
    );
    out->replace(framePos, size(frameHdr), (char *)frameHdr, size(frameHdr));

    addData(out, stream, body, more);
}

//===========================================================================
template<typename T>
void HttpConn::addData(
    CharBuf * out, 
    int stream, 
    const T & data, 
    bool more
) {
    size_t count = size(data);

    size_t pos = 0;
    while (pos + m_maxOutputFrame < count) {
        startFrame(out, stream, FrameType::kData, m_maxOutputFrame, {});
        out->append(data, pos, m_maxOutputFrame);
        pos += m_maxOutputFrame;
    }
    if (pos == count && more) 
        return;

    startFrame(
        out,
        stream,
        FrameType::kData,
        int(count - pos),
        more ? (FrameFlags) 0 : fEndStream
    );
    out->append(data, pos);
}

//===========================================================================
// Serializes a request and returns the stream id used
int HttpConn::request(CharBuf * out, const HttpMsg & msg, bool more) {
    auto strm = make_shared<HttpStream>();
    strm->m_state = HttpStream::kOpen;
    unsigned id = m_nextOutputStream;
    m_nextOutputStream += 2;
    m_streams[id] = strm;
    writeMsg(out, id, msg, more);
    return id;
}

//===========================================================================
// Serializes a push promise
int HttpConn::pushPromise(CharBuf * out, const HttpMsg & msg, bool more) {
    return 0;
}

//===========================================================================
// Serializes a reply on the specified stream
void HttpConn::reply(
    CharBuf * out, 
    int stream, 
    const HttpMsg & msg, 
    bool more
) {
    auto it = m_streams.find(stream);
    if (it == m_streams.end())
        return;

    HttpStream * strm = it->second.get();
    switch (strm->m_state) {
    case HttpStream::kOpen: strm->m_state = HttpStream::kLocalClosed; break;
    case HttpStream::kLocalReserved:
    case HttpStream::kRemoteClosed: strm->m_state = HttpStream::kClosed; break;
    default:
        logMsgCrash() << "httpReply invalid state, {" << strm->m_state << "}";
        return;
    }

    writeMsg(out, stream, msg, more);
}

//===========================================================================
void HttpConn::resetStream(CharBuf * out, int stream) {}

//===========================================================================
void HttpConn::deleteStream(int stream, HttpStream * sm) {
    auto it = m_streams.find(stream);
    if (it != m_streams.end() && sm == it->second.get()) {
        sm->m_state = sm->kDeleted;
        m_streams.erase(it);
    }
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
HttpConnHandle Dim::httpConnect(CharBuf * out) {
    auto * conn = new HttpConn;
    auto h = s_conns.insert(conn);
    conn->connect(out, h);
    return h;
}

//===========================================================================
HttpConnHandle Dim::httpAccept() {
    auto * conn = new HttpConn;
    auto h = s_conns.insert(conn);
    conn->accept(h);
    return h;
}

//===========================================================================
void Dim::httpClose(HttpConnHandle hc) {
    s_conns.erase(hc);
}

//===========================================================================
bool Dim::httpRecv(
    HttpConnHandle hc,
    CharBuf * out,
    std::vector<std::unique_ptr<HttpMsg>> * msgs,
    const void * src,
    size_t srcLen
) {
    if (auto * conn = s_conns.find(hc))
        return conn->recv(out, msgs, src, srcLen);
    return false;
}

//===========================================================================
int Dim::httpRequest(
    HttpConnHandle hc, 
    CharBuf * out, 
    const HttpMsg & msg,
    bool more
) {
    if (auto * conn = s_conns.find(hc))
        return conn->request(out, msg, more);
    return 0;
}

//===========================================================================
int Dim::httpPushPromise(
    HttpConnHandle hc,
    CharBuf * out,
    const HttpMsg & msg,
    bool more) {
    if (auto * conn = s_conns.find(hc))
        return conn->pushPromise(out, msg, more);
    return 0;
}

//===========================================================================
void Dim::httpReply(
    HttpConnHandle hc,
    CharBuf * out,
    int stream,
    const HttpMsg & msg,
    bool more
) {
    auto * conn = s_conns.find(hc);
    conn->reply(out, stream, msg, more);
}

//===========================================================================
void Dim::httpData(
    HttpConnHandle hc, 
    CharBuf * out, 
    int stream, 
    const CharBuf & data,
    bool more
) {
    if (auto * conn = s_conns.find(hc))
        conn->addData(out, stream, data, more);
}

//===========================================================================
void Dim::httpData(
    HttpConnHandle hc, 
    CharBuf * out, 
    int stream, 
    string_view data,
    bool more
) {
    if (auto * conn = s_conns.find(hc))
        conn->addData(out, stream, data, more);
}

//===========================================================================
void Dim::httpResetStream(HttpConnHandle hc, CharBuf * out, int stream) {
    if (auto * conn = s_conns.find(hc))
        return conn->resetStream(out, stream);
}
