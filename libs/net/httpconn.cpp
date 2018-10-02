// Copyright Glen Knowles 2016 - 2018.
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
constexpr int ntoh31(const char frame[]) {
    return ntoh32(frame) & 0x7fff'ffff;
}

//===========================================================================
constexpr char * hton31(char * out, int val) {
    return hton32(out, unsigned(val) & 0x7fff'ffff);
}


/****************************************************************************
*
*   Reset streams
*
***/

constexpr Duration kMaxResetStreamAge = 10s;
constexpr Duration kMinResetStreamCheckInterval = 2s;

namespace {
class ResetStreamTimer : public ITimerNotify {
    Duration onTimer(TimePoint now) override;
};
} // namespace
static deque<ResetStream> s_resetStreams;
static ResetStreamTimer s_resetTimer;

//===========================================================================
Duration ResetStreamTimer::onTimer(TimePoint now) {
    auto expire = now - kMaxResetStreamAge;
    while (!empty(s_resetStreams)) {
        auto & rs = s_resetStreams.front();
        if (auto sleep = rs.sm->m_closed - expire; sleep > 0s)
            return min(sleep, kMinResetStreamCheckInterval);

        if (rs.sm->m_localState == HttpStream::kClosed
            && rs.sm->m_remoteState == HttpStream::kClosed
        ) {
            if (HttpConn * conn = s_conns.find(rs.hc))
                conn->deleteStream(rs.stream, rs.sm.get());
        } else {
            assert(rs.sm->m_localState == HttpStream::kDeleted);
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
    explicit MsgDecoder(HttpMsg & msg);

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
*   HttpStream
*
***/

auto & s_perfStreams = uperf("http.streams (current)");

//===========================================================================
HttpStream::HttpStream() {
    s_perfStreams += 1;
}

//===========================================================================
HttpStream::~HttpStream() {
    s_perfStreams -= 1;
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
    , m_nextOutputStream(2)
    , m_encoder{kDefaultHeaderTableSize}
    , m_decoder{kDefaultHeaderTableSize}
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
            return false;
    }
    return true;
}

//===========================================================================
bool HttpConn::replyGoAway(
    CharBuf * out,
    int lastStream,
    HttpConn::FrameError error,
    string_view msg
) {
    // GoAway frame
    //  reserved : 1
    //  lastStreamId : 31
    //  errorCode : 32
    //  data[]

    m_errmsg = msg = msg.substr(0, 256);
    char buf[8];
    hton31(buf, lastStream);
    hton32(buf + 4, (int) error);
    startFrame(out, 0, FrameType::kGoAway, sizeof(buf) + msg.size(), {});
    out->append(buf, sizeof(buf));
    out->append(msg);
    return false;
}

//===========================================================================
bool HttpConn::replyGoAway(
    Dim::CharBuf * out,
    HttpConn::FrameError error,
    std::string_view msg
) {
    return replyGoAway(out, m_lastInputStream, error, msg);
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
    vector<unique_ptr<HttpMsg>> * msgs,
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
        if (ptr == eptr && srcLen + size(m_input) < size(kPrefaceData) - 1) {
            m_input.insert(m_input.end(), ptr - srcLen, eptr);
            return true;
        }
        m_input.clear();

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
            assert(used < kFrameHeaderLen);
            size_t need = kFrameHeaderLen - used;
            if (avail < need) {
                m_input.insert(m_input.end(), ptr, eptr);
                return true;
            }
            m_input.insert(m_input.end(), ptr, ptr + need);
            ptr += need;
            avail -= need;
            m_inputFrameLen = getFrameLen(data(m_input));
            if (avail < (size_t) m_inputFrameLen) {
                if (m_inputFrameLen > m_maxInputFrame)
                    return onFrame(out, msgs, data(m_input));
                m_input.insert(m_input.end(), ptr, eptr);
                m_byteMode = ByteMode::kPayload;
                return true;
            }
            m_input.insert(m_input.end(), ptr, ptr + m_inputFrameLen);
            ptr += m_inputFrameLen;
            if (!onFrame(out, msgs, data(m_input)))
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
        if (avail < (size_t) m_inputFrameLen
            && m_inputFrameLen <= m_maxInputFrame
        ) {
            m_input.assign(ptr, eptr);
            m_byteMode = ByteMode::kPayload;
            return true;
        }
        if (!onFrame(out, msgs, ptr))
            return false;
        ptr += kFrameHeaderLen + m_inputFrameLen;
        goto next_frame;

    default:
        assert(m_byteMode == ByteMode::kPayload);
        [[fallthrough]];

    case ByteMode::kPayload:
        avail = eptr - ptr;
        size_t used = size(m_input);
        assert(used >= kFrameHeaderLen);
        assert(used < (size_t) kFrameHeaderLen + m_inputFrameLen);
        size_t need = kFrameHeaderLen + m_inputFrameLen - used;
        if (avail < need) {
            m_input.insert(m_input.end(), ptr, eptr);
            return true;
        }
        m_input.insert(m_input.end(), ptr, ptr + need);
        ptr += need;
        if (!onFrame(out, msgs, data(m_input)))
            return false;
        m_input.clear();
        m_byteMode = ByteMode::kHeader;
        goto next_frame;
    };
}

//===========================================================================
void HttpConn::resetStream(CharBuf * out, int stream, FrameError error) {
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
    sm->m_localState = HttpStream::kClosed;
    sm->m_remoteState = HttpStream::kClosed;
    sm->m_closed = timeNow();
    auto & rs = s_resetStreams.emplace_back();
    rs.hc = m_handle;
    rs.stream = stream;
    rs.sm = sm;
    timerUpdate(&s_resetTimer, kMaxResetStreamAge, true);
}

//===========================================================================
HttpStream * HttpConn::findAlways(CharBuf * out, int stream) {
    auto ib = m_streams.emplace(stream, nullptr);
    if (!ib.second)
        return ib.first->second.get();
    if (stream % 2 == 0) {
        replyGoAway(
            out,
            FrameError::kProtocolError,
            "recv headers frame on even stream id"
        );
        return nullptr;
    }
    if (stream < m_lastInputStream) {
        replyGoAway(
            out,
            FrameError::kProtocolError,
            "headers frame on closed stream"
        );
        return nullptr;
    }

    m_lastInputStream = stream;
    ib.first->second = make_shared<HttpStream>();
    auto sm = ib.first->second.get();
    sm->m_flowWindow = m_initialFlowWindow;
    return sm;
}

//===========================================================================
bool HttpConn::onFrame(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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
        return replyGoAway(
            out,
            FrameError::kFrameSizeError,
            "frame size too large"
        );
    }

    switch (type) {
    case FrameType::kContinuation:
        return onContinuation(out, msgs, src, stream, flags);
    case FrameType::kData:
        return onData(out, msgs, src, stream, flags);
    case FrameType::kGoAway:
        return onGoAway(out, msgs, src, stream, flags);
    case FrameType::kHeaders:
        return onHeaders(out, msgs, src, stream, flags);
    case FrameType::kPing:
        return onPing(out, msgs, src, stream, flags);
    case FrameType::kPriority:
        return onPriority(out, msgs, src, stream, flags);
    case FrameType::kPushPromise:
        return onPushPromise(out, msgs, src, stream, flags);
    case FrameType::kRstStream:
        return onRstStream(out, msgs, src, stream, flags);
    case FrameType::kSettings:
        return onSettings(out, msgs, src, stream, flags);
    case FrameType::kWindowUpdate:
        return onWindowUpdate(out, msgs, src, stream, flags);
    };

    switch (m_frameMode) {
    case FrameMode::kContinuation:
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "unknown frame, expected continuation"
        );
    case FrameMode::kNormal:
        // ignore unknown frames unless a specific frame type is required
        return true;
    case FrameMode::kSettings:
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "unknown frame, expected settings"
        );
    }

    assert(!"unreachable");
    return false;
}

//===========================================================================
bool HttpConn::onData(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "data frame on stream 0"
        );
    }

    // adjust for any included padding
    UnpaddedData data;
    if (!removePadding(&data, src, m_inputFrameLen, 0, flags)) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "data frame with too much or non-zero padding"
        );
    }

    // TODO: receive flow control, check for misbehaving peer
    if (data.dataLen) {
        replyWindowUpdate(out, stream, m_inputFrameLen);
        replyWindowUpdate(out, 0, m_inputFrameLen);
    }

    shared_ptr<HttpStream> sm;
    auto it = m_streams.find(stream);
    if (it != m_streams.end())
        sm = it->second;
    if (!sm || sm->m_remoteState != HttpStream::kOpen) {
        // data frame on non-open stream
        resetStream(out, stream, FrameError::kStreamClosed);
        return true;
    }

    CharBuf & buf = sm->m_msg->body();
    buf.append(data.data, data.dataLen);
    if (flags & fEndStream) {
        sm->m_remoteState = HttpStream::kClosed;
        msgs->push_back(move(sm->m_msg));
    }
    return true;
}

//===========================================================================
bool HttpConn::onHeaders(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "headers frame on stream 0"
        );
    }

    // adjust for any included padding
    UnpaddedData ud;
    int hdrLen = (flags & fPriority) ? 5 : 0;
    if (!removePadding(&ud, src, m_inputFrameLen, hdrLen, flags)) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "headers frame with too much or non-zero padding"
        );
    }

    // parse priority
    if (flags & fPriority) {
        PriorityData pri;
        if (!removePriority(&pri, stream, ud.hdr, hdrLen)) {
            return replyGoAway(
                out,
                FrameError::kProtocolError,
                "headers frame with invalid priority"
            );
        }
        updatePriority();
    }

    // find stream
    auto sm = findAlways(out, stream);
    if (!sm)
        return false;

    switch (sm->m_remoteState) {
    case HttpStream::kIdle:
        sm->m_remoteState = (flags & fEndStream)
            ? HttpStream::kClosed
            : HttpStream::kOpen;
        switch (sm->m_localState) {
        case HttpStream::kIdle:
            sm->m_msg = make_unique<HttpRequest>(stream);
            break;
        case HttpStream::kReserved:
            assert(!"mismatched states, local reserved, remote idle");
            return false;
        case HttpStream::kOpen:
        case HttpStream::kClosed:
            sm->m_msg = make_unique<HttpResponse>(stream);
            break;
        case HttpStream::kDeleted:
            assert(!"mismatched states, local deleted, remote idle");
            return false;
        }
        break;
    case HttpStream::kReserved:
        sm->m_remoteState = (flags & fEndStream)
            ? HttpStream::kClosed
            : HttpStream::kOpen;
        sm->m_msg = make_unique<HttpResponse>(stream);
        break;
    case HttpStream::kOpen:
        if (flags & fEndStream) {
            // trailing headers not supported
            // !!! should probably send a stream error and process
            //     the headers (to maintain the connection decompression
            //     context) instead of dropping the connection.
            return replyGoAway(
                out,
                stream,
                FrameError::kProtocolError,
                "trailing headers not supported"
            );
        }
        [[fallthrough]];
    case HttpStream::kClosed:
    case HttpStream::kDeleted:
        // !!! should probably send a stream error and process
        //     the headers (to maintain the connection decompression
        //     context) instead of dropping the connection.
        return replyGoAway(
            out,
            stream,
            FrameError::kProtocolError,
            "headers frame on closed stream"
        );
    }

    if (~flags & fEndHeaders)
        m_frameMode = FrameMode::kContinuation;

    auto * msg = sm->m_msg.get();
    MsgDecoder mdec(*msg);
    if (!m_decoder.parse(&mdec, &msg->heap(), ud.data, ud.dataLen)) {
        return replyGoAway(
            out,
            FrameError::kCompressionError,
            "header decoding failure"
        );
    }
    if (!mdec) {
        resetStream(out, stream, mdec.Error());
    } else {
        if (sm->m_remoteState == HttpStream::kClosed)
            msgs->push_back(move(sm->m_msg));
    }
    return true;
}

//===========================================================================
bool HttpConn::onPriority(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "priority frame on stream 0"
        );
    }

    if (m_inputFrameLen != 5) {
        resetStream(out, stream, FrameError::kFrameSizeError);
        return true;
    }

    PriorityData pri;
    if (!removePriority(
        &pri,
        stream,
        src + kFrameHeaderLen,
        m_inputFrameLen
    )) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "invalid priority on priority frame"
        );
    }

    return true;
}

//===========================================================================
bool HttpConn::onRstStream(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // RstStream frame
    //  errorCode : 32

    if (m_frameMode != FrameMode::kNormal || !stream) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "rststream frame on stream 0"
        );
    }
    if (m_inputFrameLen != 4) {
        return replyGoAway(
            out,
            FrameError::kFrameSizeError,
            "rststream frame size must be 4"
        );
    }
    // find stream to reset
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) {
        if (stream > m_lastInputStream) {
            return replyGoAway(
                out,
                FrameError::kProtocolError,
                "rststream frame on idle stream"
            );
        }
        // ignore reset on closed streams
        return true;
    }

    m_streams.erase(it);
    return true;
}

//===========================================================================
bool HttpConn::onSettings(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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
        || stream
    ) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "settings frames must be on stream 0"
        );
    }
    m_frameMode = FrameMode::kNormal;

    if (flags & fAck) {
        if (m_inputFrameLen) {
            return replyGoAway(
                out,
                FrameError::kFrameSizeError,
                "settings ack must not have settings changes"
            );
        }
        if (!m_unackSettings) {
            return replyGoAway(
                out,
                FrameError::kProtocolError,
                "unmatched settings ack"
            );
        }
        m_unackSettings -= 1;
        return true;
    }

    // must be an even multiple of identifier/value pairs
    if (m_inputFrameLen % 6) {
        return replyGoAway(
            out,
            FrameError::kFrameSizeError,
            "settings frame size must be a multiple of 6"
        );
    }

    for (int pos = 0; pos < m_inputFrameLen; pos += 6) {
        unsigned identifier = ntoh16(src + kFrameHeaderLen + pos);
        unsigned value = ntoh32(src + kFrameHeaderLen + pos + 2);
        switch (identifier) {
        case kSettingsInitialWindowSize:
            if (!setInitialWindowSize(out, value))
                return false;
            break;
        }
    }

    startFrame(out, 0, FrameType::kSettings, 0, fAck);
    return true;
}

//===========================================================================
bool HttpConn::setInitialWindowSize(CharBuf * out, unsigned value) {
    if (value > kMaximumWindowSize) {
        return replyGoAway(
            out,
            FrameError::kFlowControlError,
            "initial window size too large"
        );
    }
    m_initialFlowWindow = (int) value;
    auto diff = (int) value - m_initialFlowWindow;
    for (auto & sm : m_streams) {
        auto & win = sm.second->m_flowWindow;
        if (diff > 0 && win > kMaximumWindowSize - diff) {
            return replyGoAway(
                out,
                FrameError::kFlowControlError,
                "initial window size too large for existing streams"
            );
        }
        sm.second->m_flowWindow += diff;
    }
    return true;
}

//===========================================================================
bool HttpConn::onPushPromise(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
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

    if (m_frameMode != FrameMode::kNormal || !stream) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "push promise frame on stream 0"
        );
    }

    // TODO: actually process promises
    return replyGoAway(
        out,
        FrameError::kProtocolError,
        "push promise not supported"
    );
}

//===========================================================================
bool HttpConn::onPing(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Ping frame
    //  data[8]

    if (m_frameMode != FrameMode::kNormal || stream) {
        // Ping frames MUST be on stream 0
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "ping frame must be on stream 0"
        );
    }
    if (m_inputFrameLen != 8) {
        return replyGoAway(
            out,
            FrameError::kFrameSizeError,
            "ping frame size must be multiple of 8"
        );
    }

    if (~flags & fAck) {
        startFrame(out, 0, FrameType::kPing, 8, fAck);
        out->append(src + kFrameHeaderLen, m_inputFrameLen);
    }
    return true;
}

//===========================================================================
bool HttpConn::onGoAway(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // GoAway frame
    //  reserved : 1
    //  lastStreamId : 31
    //  errorCode : 32
    //  data[]

    if (stream) {
        // GoAway frames MUST be on stream 0
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "goaway frame must be on stream 0"
        );
    }

    int lastStreamId = ntoh31(src + kFrameHeaderLen);
    FrameError errorCode = (FrameError)ntoh32(src + kFrameHeaderLen + 4);

    if (m_lastOutputStream && lastStreamId > m_lastOutputStream) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "multiple goaway frames must have decreasing last stream ids"
        );
    }

    m_lastOutputStream = lastStreamId;
    if (errorCode == FrameError::kNoError) {
        if (m_lastOutputStream > m_nextOutputStream) {
            // TODO: report shutdown requested
            return true;
        }
    }

    return replyGoAway(
        out,
        FrameError::kNoError,
        "graceful shutdown"
    );
}

//===========================================================================
bool HttpConn::onWindowUpdate(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // WindowUpdate frame
    //  reserved : 1
    //  increment : 31

    if (m_frameMode != FrameMode::kNormal) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "windowupdate unexpected"
        );
    }
    if (m_inputFrameLen != 4) {
        return replyGoAway(
            out,
            FrameError::kFrameSizeError,
            "windowupdate frame size must be 4"
        );
    }

    int increment = ntoh31(src + kFrameHeaderLen);
    if (!increment) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "windowupdate with 0 increment"
        );
    }

    if (!stream) {
        // update window of connection
        if (kMaximumWindowSize - increment < m_flowWindow) {
            return replyGoAway(
                out,
                FrameError::kFlowControlError,
                "windowupdate with increment too large"
            );
        }
        m_flowWindow += increment;
        for (auto && [id, sm] : m_streams) {
            if (!m_flowWindow)
                break;
            writeUnsent(out, id, sm.get());
        }
        return true;
    }

    // update window of individual stream
    auto it = m_streams.find(stream);
    if (it == m_streams.end()) {
        if (stream > m_lastInputStream) {
            return replyGoAway(
                out,
                FrameError::kProtocolError,
                "windowupdate on idle stream"
            );
        }
        // ignore window updates on completed streams
        return true;
    }

    auto sm = it->second.get();
    if (kMaximumWindowSize - increment < sm->m_flowWindow) {
        resetStream(out, stream, FrameError::kFlowControlError);
        return true;
    }
    sm->m_flowWindow += increment;
    writeUnsent(out, stream, sm);
    return true;
}

//===========================================================================
bool HttpConn::onContinuation(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    const char src[],
    int stream,
    FrameFlags flags
) {
    // Continuation frame
    //  headerBlock[]

    if (m_frameMode != FrameMode::kContinuation
        || stream != m_continueStream
    ) {
        return replyGoAway(
            out,
            FrameError::kProtocolError,
            "continuation unexpected"
        );
    }

    // TODO: actually process continuations
    return replyGoAway(
        out,
        FrameError::kProtocolError,
        "continuation frames not supported"
    );
}


/****************************************************************************
*
*   Sending data
*
***/

//===========================================================================
bool HttpConn::writeMsg(
    CharBuf * out,
    int stream,
    HttpStream * sm,
    const HttpMsg & msg,
    bool more
) {
    if (sm->m_localState == HttpStream::kClosed)
        return false;

    const CharBuf & body = msg.body();
    size_t framePos = out->size();
    size_t maxEndPos = framePos + m_maxOutputFrame + kFrameHeaderLen;
    bool headersOnly = body.empty() && !more;
    auto ftype = FrameType::kHeaders;
    auto flags = headersOnly ? fEndStream : (FrameFlags) 0;
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
                    frameHdr,
                    stream,
                    ftype,
                    m_maxOutputFrame,
                    flags
                );
                out->replace(
                    framePos,
                    size(frameHdr),
                    (char *)frameHdr,
                    size(frameHdr)
                );
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

    switch (sm->m_localState) {
    case HttpStream::kIdle:
    case HttpStream::kReserved:
        sm->m_localState = HttpStream::kOpen;
        [[fallthrough]];
    case HttpStream::kOpen:
        if (headersOnly)
            sm->m_localState = HttpStream::kClosed;
        break;
    default:
        logMsgFatal() << "httpReply invalid state, {"
            << sm->m_localState << "}";
        return false;
    }

    return headersOnly || addData(out, stream, sm, body, more);
}

//===========================================================================
template<typename T>
bool HttpConn::addData(
    CharBuf * out,
    int stream,
    HttpStream * sm,
    const T & data,
    bool more
) {
    if (!sm) {
        auto it = m_streams.find(stream);
        if (it == m_streams.end())
            return false;
        sm = it->second.get();
    }

    size_t dataLen = size(data);
    size_t count = dataLen;
    if (!sm->m_unsent.empty()) {
        count = 0;
    } else {
        auto windowAvail = min(m_flowWindow, sm->m_flowWindow);
        if (windowAvail < dataLen)
            count = (windowAvail < 0) ? 0 : windowAvail;
    }
    size_t pos = 0;
    while (pos + m_maxOutputFrame < count) {
        startFrame(out, stream, FrameType::kData, m_maxOutputFrame, {});
        out->append(data, pos, m_maxOutputFrame);
        pos += m_maxOutputFrame;
    }
    if (pos != count || !more && count == dataLen) {
        startFrame(
            out,
            stream,
            FrameType::kData,
            int(count - pos),
            (!more && count == dataLen) ? fEndStream : (FrameFlags) 0
        );
        out->append(data, pos, count - pos);
        pos = count;
    }

    m_flowWindow -= (int) count;
    sm->m_flowWindow -= (int) count;
    if (auto unsent = dataLen - count)
        sm->m_unsent.append(data, pos, unsent);

    switch (sm->m_localState) {
    case HttpStream::kOpen:
        if (!more)
            sm->m_localState = HttpStream::kClosed;
        break;
    default:
        logMsgFatal() << "httpData invalid state, {" << sm->m_localState << "}";
        return false;
    }

    // erase if fully closed and no pending data frames
    if (sm->m_localState == HttpStream::kClosed
        && sm->m_unsent.empty()
        && sm->m_remoteState == HttpStream::kClosed
    ) {
        m_streams.erase(stream);
    }
    return true;
}

//===========================================================================
void HttpConn::writeUnsent(
    CharBuf * out,
    int stream,
    HttpStream * sm
) {
    bool more = sm->m_localState != HttpStream::kClosed;

    size_t dataLen = size(sm->m_unsent);
    size_t count = dataLen;
    auto windowAvail = min(m_flowWindow, sm->m_flowWindow);
    if (windowAvail < dataLen) {
        if (windowAvail < m_maxOutputFrame)
            return;
        count = windowAvail;
    }
    size_t pos = 0;
    while (pos + m_maxOutputFrame < count) {
        startFrame(out, stream, FrameType::kData, m_maxOutputFrame, {});
        out->append(sm->m_unsent, pos, m_maxOutputFrame);
        pos += m_maxOutputFrame;
    }
    if (pos != count || !more && count == dataLen) {
        startFrame(
            out,
            stream,
            FrameType::kData,
            int(count - pos),
            (!more && count == dataLen) ? fEndStream : (FrameFlags) 0
        );
        out->append(sm->m_unsent, pos, count - pos);
        pos = count;
    }

    m_flowWindow -= (int) count;
    sm->m_flowWindow -= (int) count;
    sm->m_unsent.erase(0, count);

    // erase if fully closed and no pending data frames
    if (sm->m_localState == HttpStream::kClosed
        && sm->m_unsent.empty()
        && sm->m_remoteState == HttpStream::kClosed
    ) {
        m_streams.erase(stream);
    }
}

//===========================================================================
// Serializes a request and returns the stream id used
int HttpConn::request(CharBuf * out, const HttpMsg & msg, bool more) {
    auto sm = make_shared<HttpStream>();
    unsigned id = m_nextOutputStream;
    m_nextOutputStream += 2;
    m_streams[id] = sm;
    writeMsg(out, id, sm.get(), msg, more);
    return id;
}

//===========================================================================
// Serializes a push promise
int HttpConn::pushPromise(CharBuf * out, const HttpMsg & msg, bool more) {
    assert(!"push promises not implemented");
    return 0;
}

//===========================================================================
// Serializes a reply on the specified stream
bool HttpConn::reply(
    CharBuf * out,
    int stream,
    const HttpMsg & msg,
    bool more
) {
    auto it = m_streams.find(stream);
    if (it == m_streams.end())
        return false;
    HttpStream * sm = it->second.get();

    return writeMsg(out, stream, sm, msg, more);
}

//===========================================================================
void HttpConn::deleteStream(int stream, HttpStream * sm) {
    auto it = m_streams.find(stream);
    if (it != m_streams.end() && sm == it->second.get()) {
        sm->m_localState = HttpStream::kDeleted;
        sm->m_remoteState = HttpStream::kDeleted;
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
    auto conn = new HttpConn;
    auto h = s_conns.insert(conn);
    conn->connect(out, h);
    return h;
}

//===========================================================================
HttpConnHandle Dim::httpAccept() {
    auto conn = new HttpConn;
    auto h = s_conns.insert(conn);
    conn->accept(h);
    return h;
}

//===========================================================================
void Dim::httpClose(HttpConnHandle hc) {
    s_conns.erase(hc);
}

//===========================================================================
string_view Dim::httpGetError(HttpConnHandle hc) {
    if (auto conn = s_conns.find(hc))
        return conn->errmsg();
    return {};
}

//===========================================================================
bool Dim::httpRecv(
    CharBuf * out,
    vector<unique_ptr<HttpMsg>> * msgs,
    HttpConnHandle hc,
    const void * src,
    size_t srcLen
) {
    if (auto conn = s_conns.find(hc))
        return conn->recv(out, msgs, src, srcLen);
    return false;
}

//===========================================================================
int Dim::httpRequest(
    CharBuf * out,
    HttpConnHandle hc,
    const HttpMsg & msg,
    bool more
) {
    if (auto conn = s_conns.find(hc))
        return conn->request(out, msg, more);
    return 0;
}

//===========================================================================
int Dim::httpPushPromise(
    CharBuf * out,
    HttpConnHandle hc,
    const HttpMsg & msg,
    bool more
) {
    if (auto conn = s_conns.find(hc))
        return conn->pushPromise(out, msg, more);
    return 0;
}

//===========================================================================
bool Dim::httpReply(
    CharBuf * out,
    HttpConnHandle hc,
    int stream,
    const HttpMsg & msg,
    bool more
) {
    if (auto conn = s_conns.find(hc))
        return conn->reply(out, stream, msg, more);
    return false;
}

//===========================================================================
bool Dim::httpData(
    CharBuf * out,
    HttpConnHandle hc,
    int stream,
    const CharBuf & data,
    bool more
) {
    if (auto conn = s_conns.find(hc))
        return conn->addData(out, stream, nullptr, data, more);
    return false;
}

//===========================================================================
bool Dim::httpData(
    CharBuf * out,
    HttpConnHandle hc,
    int stream,
    string_view data,
    bool more
) {
    if (auto conn = s_conns.find(hc))
        return conn->addData(out, stream, nullptr, data, more);
    return false;
}

//===========================================================================
void Dim::httpResetStream(
    CharBuf * out,
    HttpConnHandle hc,
    int stream,
    bool internal
) {
    if (auto conn = s_conns.find(hc)) {
        conn->resetStream(
            out,
            stream,
            internal
                ? HttpConn::FrameError::kInternalError
                : HttpConn::FrameError::kCancel
        );
    }
}
