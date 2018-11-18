// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// tlsint.h - dim tls
#pragma once

#include <cstdint>
#include <vector>

namespace Dim {

class TlsRecordReader;
class TlsRecordWriter;


/****************************************************************************
*
*   Handshake messages
*
***/

struct TlsKeyShare {
    std::vector<uint8_t> keyExchange;
    TlsNamedGroup group;
};

struct TlsPresharedKey {
    std::vector<uint8_t> data;
};

struct TlsClientHelloMsg {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t random[32];
    std::vector<TlsCipherSuite> suites;

    // extensions
    uint16_t draftVersion;
    std::vector<TlsKeyShare> groups;
    std::vector<TlsPresharedKey> identities;
    std::vector<TlsSignatureScheme> sigSchemes;
    std::vector<uint8_t> hostName;
};

struct TlsServerHelloMsg {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t random[32];
    TlsCipherSuite suite;

    // extensions
    uint16_t draftVersion;
    TlsKeyShare keyShare;
    TlsPresharedKey identity;
};

struct TlsHelloRetryRequestMsg {
    uint8_t majorVersion;
    uint8_t minorVersion;
    TlsCipherSuite suite;
    TlsNamedGroup group;

    // extensions
    uint16_t draftVersion;
};

struct TlsEncryptedExtensionsMsg {};

void tlsSetKeyShare(TlsKeyShare * out, TlsNamedGroup group);

void tlsWrite(TlsRecordWriter * out, TlsClientHelloMsg const & msg);
void tlsWrite(TlsRecordWriter * out, TlsServerHelloMsg const & msg);
void tlsWrite(TlsRecordWriter * out, TlsHelloRetryRequestMsg const & msg);

bool tlsParse(TlsClientHelloMsg * msg, TlsRecordReader & in);
bool tlsParse(TlsServerHelloMsg * msg, TlsRecordReader & in);


/****************************************************************************
*
*   Tls connection
*
***/

class TlsRecordWriter;
class TlsRecordReader;

class TlsConnBase
    : public ITlsRecordDecryptNotify
    , public HandleContent
{
public:
    TlsConnBase();
    void setSuites(TlsCipherSuite const suites[], size_t count);
    std::vector<TlsCipherSuite> const & suites() const;

    bool recv(
        CharBuf * reply,
        CharBuf * data,
        void const * src,
        size_t srcLen
    );

    // ITlsRecordDecryptNotify
    virtual void onTlsAlert(TlsAlertDesc desc, TlsAlertLevel level) override;
    virtual void onTlsHandshake(
        TlsHandshakeType type,
        uint8_t const msg[],
        size_t msgLen) override;

    virtual void onTlsHandshake(TlsClientHelloMsg const & msg);
    virtual void onTlsHandshake(TlsServerHelloMsg const & msg);
    virtual void onTlsHandshake(TlsHelloRetryRequestMsg const & msg);

private:
    friend class TlsRecordWriter;
    friend class TlsRecordReader;
    void addAlert(TlsAlertDesc desc, TlsAlertLevel level = kFatal);

    template <typename T> void handshake(TlsRecordReader & in);

    std::vector<TlsCipherSuite> m_suites;

    CharBuf * m_reply{};
    TlsRecordEncrypt m_encrypt;
    TlsRecordDecrypt m_in;
};

class TlsRecordWriter {
public:
    TlsRecordWriter(CharBuf * out, TlsConnBase & conn);
    ~TlsRecordWriter();

    void contentType(TlsContentType type);

    void number(uint8_t val);
    void number16(uint16_t val);
    void fixed(void const * ptr, size_t count);

    // Complete variable length vector
    void var(void const * ptr, size_t count);
    void var16(void const * ptr, size_t count);

    // Variable length vector. Start the vector, use number and fixed to set
    // the content, and then end the vector. May be nested.
    void start();
    void start16();
    void start24();
    void end();

private:
    CharBuf * m_out{};
    TlsRecordEncrypt & m_rec;

    unsigned m_type{256};
    CharBuf m_buf;
    struct Pos {
        size_t pos;
        uint8_t width;
    };
    std::vector<Pos> m_stack;
};

class TlsRecordReader {
public:
    TlsRecordReader(TlsConnBase & conn, void const * ptr, size_t count);

    uint8_t number();
    uint16_t number16();
    unsigned number24();
    template <typename T> T number();
    void fixed(uint8_t * dst, size_t dstLen);
    void skip(size_t count);

    void setAlert(TlsAlertDesc desc, TlsAlertLevel level = kFatal);

    size_t size() const;

private:
    bool m_failed{false};
    TlsConnBase & m_conn;
    uint8_t const * m_ptr;
    int m_count;
};

//===========================================================================
template <typename T> inline T TlsRecordReader::number() {
    if constexpr (sizeof(T) == 1) {
        return (T)number();
    } else if constexpr (sizeof(T) == 2) {
        return (T)number16();
    } else {
        assert(sizeof(T) == 3);
        return (T)number24();
    }
}

} // namespace
