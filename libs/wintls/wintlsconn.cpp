// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// wintls.cpp - dim windows platform tls
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

using NtStatus = WinError::NtStatus;
using SecStatus = WinError::SecurityStatus;


/****************************************************************************
*
*   Tuning parameters
*
***/

const bool kMakeCert = false;


/****************************************************************************
*
*   Private
*
***/

namespace {

class ServerConn {
public:
    ServerConn();
    ~ServerConn();

    bool recv(CharBuf * out, CharBuf * data, string_view src);
    void send(CharBuf * out, string_view src);

private:
    CtxtHandle m_context;
    bool m_appData{false};
    SecPkgContext_StreamSizes m_sizes;
    SecPkgContext_ApplicationProtocol m_alpn;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static shared_mutex s_mut;
static HandleMap<WinTlsConnHandle, ServerConn> s_conns;
static unique_ptr<CredHandle> s_srvCred;

static auto & s_perfTlsReneg = uperf("tls renegotiate");
static auto & s_perfTlsExpired = uperf("tls expired context");
static auto & s_perfTlsFinish = uperf("tls finish");


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   ServerConn
*
***/

//===========================================================================
ServerConn::ServerConn() {
    SecInvalidateHandle(&m_context);
}

//===========================================================================
ServerConn::~ServerConn() {
    if (!SecIsValidHandle(&m_context))
        return;

    if (int err = DeleteSecurityContext(&m_context)) {
        WinError werr = (SecStatus) err;
        logMsgCrash() << "DeleteSecurityContext: " << werr;
    }
}

//===========================================================================
bool ServerConn::recv(CharBuf * out, CharBuf * data, string_view src) {
    int err{0};

    // Manually constructed SEC_APPLICATION_PROTOCOLS struct
    char alpn_chars[] = {
        9, 0, 0, 0, // ProtocolListsSize
        SecApplicationProtocolNegotiationExt_ALPN, 0, 0, 0, // ProtoNegoExt
        3, 0, // ProtocolListSize
        2, 'h', '2', // ProtocolList
        0 // trailing null for the debugger
    };

    // Rebuild the alpn just in case endianness matters, rendering the 
    // manually constructed little-endian version invalid.
    auto alpn = (SEC_APPLICATION_PROTOCOLS *) alpn_chars;
    alpn->ProtocolListsSize = 9;
    alpn->ProtocolLists[0].ProtoNegoExt = 
        SecApplicationProtocolNegotiationExt_ALPN;
    alpn->ProtocolLists[0].ProtocolListSize = 3;

NEGOTIATE:
    while (!m_appData) {
        unsigned flags = ASC_REQ_STREAM 
            | ASC_REQ_CONFIDENTIALITY
            | ASC_REQ_EXTENDED_ERROR
            | ASC_REQ_ALLOCATE_MEMORY
            ;

        SecBuffer inBufs[] = { 
            { (unsigned) src.size(), SECBUFFER_TOKEN, (void *) src.data() },
            { (unsigned) size(alpn_chars) - 1, 
                SECBUFFER_APPLICATION_PROTOCOLS, 
                alpn },
        };
        SecBufferDesc inDesc{ 
            SECBUFFER_VERSION, 
            (unsigned) size(inBufs), 
            inBufs 
        };

        SecBuffer outBufs[] = {
            { 0, SECBUFFER_TOKEN, nullptr },
            { 0, SECBUFFER_EXTRA, nullptr },
            { 0, SECBUFFER_ALERT, nullptr },
        };
        SecBufferDesc outDesc{
            SECBUFFER_VERSION, 
            (unsigned) size(outBufs), 
            outBufs
        };

        ULONG outFlags;
        err = AcceptSecurityContext(
            s_srvCred.get(),
            SecIsValidHandle(&m_context) ? &m_context : NULL,
            &inDesc, // input buffer
            flags,
            0, // target data representation
            &m_context,
            &outDesc,
            &outFlags, // attrs of established connection, mirrors input flags
            NULL // remote's cert expiry
        );
        if (outBufs[0].cbBuffer) {
            assert(outBufs[0].pvBuffer);
            out->append((char *) outBufs[0].pvBuffer, outBufs[0].cbBuffer);
            FreeContextBuffer(outBufs[0].pvBuffer);
        }
        if (outBufs[1].cbBuffer) {
            // TODO: figure out what is in the buffer, is it crypted data?
            // uncrypted? ignorable handshake cruft?
            assert(!outBufs[1].cbBuffer); 
            data->append((char *) outBufs[1].pvBuffer, outBufs[1].cbBuffer);
            FreeContextBuffer(outBufs[1].pvBuffer);
        }
        if (inBufs[1].BufferType == SECBUFFER_EXTRA && inBufs[1].cbBuffer) {
            // The pvBuffer field of _EXTRA is not valid
            src = src.substr(src.size() - inBufs[1].cbBuffer);
        } else {
            src = {};
        }
        if (err == SEC_E_OK) {
            s_perfTlsFinish += 1;
            m_appData = true;
            err = QueryContextAttributes(
                &m_context, 
                SECPKG_ATTR_STREAM_SIZES, 
                &m_sizes
            );
            if (err) {
                WinError werr = (SecStatus) err;
                logMsgError()
                    << "QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES): "
                    << werr;
                return false;
            }
            err = QueryContextAttributes(
                &m_context,
                SECPKG_ATTR_APPLICATION_PROTOCOL,
                &m_alpn
            );
            if (err) {
                WinError werr = (SecStatus) err;
                logMsgError()
                    << "QueryContextAttributes("
                        "SECPKG_ATTR_APPLICATION_PROTOCOL): "
                    << werr;
                return false;
            }
            break;
        }
        if (err == SEC_E_INCOMPLETE_MESSAGE 
            || err == SEC_E_INCOMPLETE_CREDENTIALS
        ) {
            // need to receive more data
            assert(src.empty());
            return true;
        }
        if (err == SEC_I_CONTINUE_NEEDED) {
            continue;
        }
        WinError werr = (SecStatus) err;
        logMsgError() << "AcceptSecurityContext: " << werr;
        if (outBufs[2].cbBuffer) {
            out->append((char *) outBufs[2].pvBuffer, outBufs[2].cbBuffer);
            FreeContextBuffer(outBufs[2].pvBuffer);
        }
        return false;
    }

    // Decrypt application data 
    for (;;) {
        if (src.empty())
            return true;

        SecBuffer bufs[] = {
            { (unsigned) src.size(), SECBUFFER_DATA, (void *) src.data() },
            { 0, SECBUFFER_EMPTY, nullptr },
            { 0, SECBUFFER_EMPTY, nullptr },
            { 0, SECBUFFER_EMPTY, nullptr },
        };
        SecBufferDesc desc{
            SECBUFFER_VERSION, 
            (unsigned) size(bufs), 
            bufs
        };
        err = DecryptMessage(&m_context, &desc, 0, NULL);
        if (err == SEC_E_INCOMPLETE_MESSAGE) 
            return true;

        if (err != SEC_E_OK 
            && err != SEC_I_CONTEXT_EXPIRED 
            && err != SEC_I_RENEGOTIATE
        ) {
            WinError werr = (SecStatus) err;
            logMsgError() << "DecryptMessage: " << werr;
            return false;
        }

        if (bufs[1].BufferType != SECBUFFER_EMPTY && bufs[1].cbBuffer) {
            assert(bufs[1].BufferType == SECBUFFER_DATA);
            data->append((char *) bufs[1].pvBuffer, bufs[1].cbBuffer);
        }
        if (bufs[3].BufferType != SECBUFFER_EMPTY && bufs[3].cbBuffer) {
            assert(bufs[3].BufferType == SECBUFFER_EXTRA);
            // Again, upon return, the pvBuffer field of _EXTRA is invalid
            src = src.substr(src.size() - bufs[3].cbBuffer);
            if (err == SEC_E_OK) 
                continue;
            if (err == SEC_I_RENEGOTIATE) {
                logMsgError() << "DecryptMessage: renegotiate requested with "
                    "encrypted data";
                continue;
            }
        }

        if (err == SEC_E_OK)
            return true;
        if (err == SEC_I_CONTEXT_EXPIRED) {
            s_perfTlsExpired += 1;
            return false;
        }

        assert(err == SEC_I_RENEGOTIATE);
        // Proceed with renegotiate by making another call to 
        // AcceptSecurityContext with no data.
        s_perfTlsReneg += 1;
        src = {};
        m_appData = false;
        goto NEGOTIATE;
    }
}

//===========================================================================
void ServerConn::send(CharBuf * out, string_view src) {
    int err{0};

    while (!src.empty()) {
        auto num = min(src.size(), (size_t) m_sizes.cbMaximumMessage);
        auto tmp = src.substr(0, num);
        src.remove_prefix(num);

        auto pos = out->size();
        out->append(m_sizes.cbHeader, 0);
        out->append(tmp);
        out->append(m_sizes.cbTrailer, 0);
        auto ptr = out->data(pos);

        SecBuffer bufs[] = {
            { m_sizes.cbHeader, SECBUFFER_STREAM_HEADER, ptr },
            { (unsigned) tmp.size(), SECBUFFER_DATA, ptr += m_sizes.cbHeader },
            { m_sizes.cbTrailer, SECBUFFER_STREAM_TRAILER, ptr += tmp.size() },
            { 0, SECBUFFER_EMPTY, nullptr },
        };
        SecBufferDesc desc{
            SECBUFFER_VERSION, 
            (unsigned) size(bufs), 
            bufs
        };
        err = EncryptMessage(&m_context, 0, &desc, 0);
        if (err) {
            WinError werr = (SecStatus) err;
            logMsgCrash() << "EncryptMessage(" << num << "): " << werr;
        }
    }
}


/****************************************************************************
*
*   ShutdownNotify
*
***/

namespace {
class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole (bool firstTry) override;
};
} // namespace
static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole (bool firstTry) {
    shared_lock<shared_mutex> lk{s_mut};
    if (!s_conns.empty())
        return shutdownIncomplete();

    s_srvCred.reset();
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winTlsInitialize() {
    shutdownMonitor(&s_cleanup);

    s_srvCred = iWinTlsCreateCred();
}

//===========================================================================
WinTlsConnHandle Dim::winTlsAccept() {
    auto conn = new ServerConn;
    lock_guard<shared_mutex> lk{s_mut};
    return s_conns.insert(conn);
}

//===========================================================================
void Dim::winTlsClose(WinTlsConnHandle h) {
    lock_guard<shared_mutex> lk{s_mut};
    s_conns.erase(h);
}

//===========================================================================
bool Dim::winTlsRecv(
    WinTlsConnHandle h,
    CharBuf * reply,
    CharBuf * data,
    string_view src
) {
    shared_lock<shared_mutex> lk{s_mut};
    auto conn = s_conns.find(h);
    return conn->recv(reply, data, src);
}

//===========================================================================
void Dim::winTlsSend(
    WinTlsConnHandle h,
    CharBuf * out,
    string_view src
) {
    shared_lock<shared_mutex> lk{s_mut};
    auto conn = s_conns.find(h);
    conn->send(out, src);
}
