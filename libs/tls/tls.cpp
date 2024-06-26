// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// tls.cpp - dim tls
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Private
*
***/

namespace {

class ClientConn : public TlsConnBase {
public:
    ClientConn(
        const char hostName[],
        const TlsCipherSuite suites[],
        size_t count);
    void connect(CharBuf * out);

private:
    string m_host;
};

class ServerConn : public TlsConnBase {
public:
private:
    void onTlsHandshake(const TlsClientHelloMsg & msg) override;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<TlsConnHandle, TlsConnBase> s_conns;


/****************************************************************************
*
*   TlsConnBase
*
***/

//===========================================================================
TlsConnBase::TlsConnBase() {}

//===========================================================================
void TlsConnBase::setSuites(const TlsCipherSuite suites[], size_t count) {
    m_suites.assign(suites, suites + count);
    ranges::sort(m_suites);
    auto last = unique(m_suites.begin(), m_suites.end());
    m_suites.erase(last, m_suites.end());
}

//===========================================================================
const vector<TlsCipherSuite> & TlsConnBase::suites() const {
    return m_suites;
}

//===========================================================================
bool TlsConnBase::recv(
    CharBuf * out,
    CharBuf * data,
    const void * src,
    size_t srcLen) {
    m_reply = out;
    bool success = m_in.parse(data, this, src, srcLen);
    m_reply = nullptr;
    return success;
}

//===========================================================================
void TlsConnBase::addAlert(TlsAlertDesc desc, TlsAlertLevel level) {
    uint8_t alert[] = {level, desc};
    m_encrypt.add(m_reply, kContentAlert, alert, size(alert));
}

//===========================================================================
void TlsConnBase::onTlsAlert(TlsAlertDesc desc, TlsAlertLevel level) {
    assert(level != kFatal);
}

//===========================================================================
template <typename T> void TlsConnBase::handshake(TlsRecordReader & in) {
    T msg;
    if (tlsParse(&msg, in) && !in.size())
        return onTlsHandshake(msg);
    in.setAlert(kDecodeError);
}

//===========================================================================
void TlsConnBase::onTlsHandshake(
    TlsHandshakeType type,
    const uint8_t data[],
    size_t dataLen) {
    TlsRecordReader in(*this, data, dataLen);
    switch (type) {
    case kClientHello: return handshake<TlsClientHelloMsg>(in);
    case kServerHello: return handshake<TlsServerHelloMsg>(in);
    case kSessionTicket:
    case kHelloRetryRequest:
    case kEncryptedExtensions:
    case kCertificate:
    case kCertificateRequest:
    case kCertificateVerify:
    case kServerConfiguration:
    case kFinished:
    case kKeyUpdate:
        assert(0);
    };
}

//===========================================================================
void TlsConnBase::onTlsHandshake(const TlsClientHelloMsg & msg) {}

//===========================================================================
void TlsConnBase::onTlsHandshake(const TlsServerHelloMsg & msg) {}

//===========================================================================
void TlsConnBase::onTlsHandshake(const TlsHelloRetryRequestMsg & msg) {}


/****************************************************************************
*
*   TlsRecordWriter
*
***/

//===========================================================================
TlsRecordWriter::TlsRecordWriter(CharBuf * out, TlsConnBase & conn)
    : m_out(out)
    , m_rec(conn.m_encrypt)
{}

//===========================================================================
TlsRecordWriter::~TlsRecordWriter() {
    if (size_t count = m_buf.size()) {
        assert(m_stack.empty());
        m_rec.add(m_out, (TlsContentType)m_type, m_buf.data(), count);
    }
}

//===========================================================================
void TlsRecordWriter::contentType(TlsContentType type) {
    m_type = type;
}

//===========================================================================
void TlsRecordWriter::number(uint8_t val) {
    fixed(&val, 1);
}

//===========================================================================
void TlsRecordWriter::number16(uint16_t val) {
    uint8_t buf[2] = {uint8_t(val >> 8), uint8_t(val)};
    fixed(buf, size(buf));
}

//===========================================================================
void TlsRecordWriter::fixed(const void * ptr, size_t count) {
    if (m_buf.size()) {
        m_buf.append((const char *)ptr, count);
    } else {
        m_rec.add(m_out, (TlsContentType)m_type, ptr, count);
    }
}

//===========================================================================
void TlsRecordWriter::var(const void * ptr, size_t count) {
    assert(count < 1 << 8);
    number((uint8_t)count);
    fixed(ptr, count);
}

//===========================================================================
void TlsRecordWriter::var16(const void * ptr, size_t count) {
    assert(count < 1 << 16);
    number16((uint16_t)count);
    fixed(ptr, count);
}

//===========================================================================
void TlsRecordWriter::start() {
    m_stack.push_back({m_buf.size(), 1});
    m_buf.append(1, 0);
}

//===========================================================================
void TlsRecordWriter::start16() {
    m_stack.push_back({m_buf.size(), 2});
    m_buf.append(2, 0);
}

//===========================================================================
void TlsRecordWriter::start24() {
    m_stack.push_back({m_buf.size(), 3});
    m_buf.append(3, 0);
}

//===========================================================================
void TlsRecordWriter::end() {
    Pos & pos = m_stack.back();
    size_t count = m_buf.size() - pos.pos;
    char buf[4];
    switch (pos.width) {
    default:
        assert(0);
        break;
    case 3:
        buf[1] = uint8_t(count >> 16);
        [[fallthrough]];
    case 2:
        buf[2] = uint8_t(count >> 8);
        [[fallthrough]];
    case 1:
        buf[3] = uint8_t(count);
    };
    m_buf.replace(pos.pos, pos.width, buf + 4 - pos.width, pos.width);
    m_stack.pop_back();
    if (!m_stack.size()) {
        m_rec.add(m_out, (TlsContentType)m_type, m_buf.data(), m_buf.size());
        m_buf.clear();
    }
}


/****************************************************************************
*
*   TlsRecordReader
*
***/

//===========================================================================
TlsRecordReader::TlsRecordReader(
    TlsConnBase & conn,
    const void * ptr,
    size_t count
)
    : m_conn(conn)
    , m_ptr((const uint8_t *)ptr)
{
    assert(count < (size_t) numeric_limits<int>::max());
    m_count = (int)count;
}

//===========================================================================
uint8_t TlsRecordReader::number() {
    m_count -= 1;
    if (m_count >= 0) {
        return *m_ptr++;
    }
    setAlert(kRecordOverflow);
    m_count = 0;
    return 0;
}

//===========================================================================
uint16_t TlsRecordReader::number16() {
    m_count -= 2;
    if (m_count >= 0) {
        uint16_t val = (m_ptr[0] << 8) + m_ptr[1];
        m_ptr += 2;
        return val;
    }
    setAlert(kRecordOverflow);
    m_count = 0;
    return 0;
}

//===========================================================================
unsigned TlsRecordReader::number24() {
    m_count -= 3;
    if (m_count >= 0) {
        unsigned val = (m_ptr[0] << 16) + (m_ptr[1] << 8) + m_ptr[2];
        m_ptr += 3;
        return val;
    }
    setAlert(kRecordOverflow);
    m_count = 0;
    return 0;
}

//===========================================================================
void TlsRecordReader::fixed(uint8_t * dst, size_t count) {
    assert(count < 1 << 24);
    m_count -= (int)count;
    if (m_count >= 0) {
        memcpy(dst, m_ptr, count);
        m_ptr += count;
        return;
    }
    setAlert(kRecordOverflow);
    m_count = 0;
    memset(dst, 0, count);
}

//===========================================================================
void TlsRecordReader::skip(size_t count) {
    assert(count < 1 << 24);
    m_count -= (int)count;
    if (m_count >= 0) {
        m_ptr += count;
        return;
    }
    setAlert(kRecordOverflow);
    m_count = 0;
}

//===========================================================================
void TlsRecordReader::setAlert(TlsAlertDesc desc, TlsAlertLevel level) {
    if (!m_failed) {
        m_conn.addAlert(desc, level);
        m_failed = true;
    }
}

//===========================================================================
size_t TlsRecordReader::size() const {
    return m_count;
}


/****************************************************************************
*
*   ClientConn
*
***/

const uint8_t kClientVersion[] = {3, 4};

//===========================================================================
ClientConn::ClientConn(
    const char hostName[],
    const TlsCipherSuite suites[],
    size_t count
) {
    if (hostName)
        m_host = hostName;
    setSuites(suites, count);
}

//===========================================================================
void ClientConn::connect(CharBuf * outbuf) {
    TlsRecordWriter out(outbuf, *this);

    TlsClientHelloMsg msg;
    msg.majorVersion = kClientVersion[0];
    msg.minorVersion = kClientVersion[1];
    msg.draftVersion = 0x3132;
    cryptRandomBytes(msg.random, sizeof msg.random);
    msg.suites = suites();
    msg.groups.resize(1);
    TlsKeyShare & key = msg.groups.back();
    tlsSetKeyShare(&key, kGroupX25519);
    msg.sigSchemes.push_back(kSigEd25519);
    msg.hostName.assign(m_host.begin(), m_host.end());
    tlsWrite(&out, msg);
}


/****************************************************************************
*
*   ServerConn
*
***/

//===========================================================================
void ServerConn::onTlsHandshake(const TlsClientHelloMsg & msg) {}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
TlsConnHandle Dim::tlsAccept(const TlsCipherSuite suites[], size_t count) {
    auto conn = new ServerConn;
    conn->setSuites(suites, count);
    return s_conns.insert(conn);
}

//===========================================================================
TlsConnHandle Dim::tlsConnect(
    CharBuf * out,
    const char hostName[],
    const TlsCipherSuite suites[],
    size_t count
) {
    auto conn = new ClientConn(hostName, suites, count);
    conn->connect(out);
    return s_conns.insert(conn);
}

//===========================================================================
void Dim::tlsClose(TlsConnHandle h) {
    s_conns.erase(h);
}

//===========================================================================
bool Dim::tlsRecv(
    CharBuf * out,
    CharBuf * data,
    TlsConnHandle h,
    const void * src,
    size_t srcLen
) {
    auto conn = s_conns.find(h);
    return conn->recv(out, data, src, srcLen);
}
