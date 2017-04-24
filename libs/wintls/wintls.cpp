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
static CredHandle s_srvCred;

static auto & s_perfTlsReneg = uperf("tls renegotiate");
static auto & s_perfTlsExpired = uperf("tls expired context");
static auto & s_perfTlsFinish = uperf("tls finish");


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static NCRYPT_KEY_HANDLE toNCrypt (BCRYPT_KEY_HANDLE bkey) {
    WinError err{0};
    ULONG count;
    err = (NtStatus) BCryptExportKey(
        bkey, 
        NULL, // export key (if also signing)
        BCRYPT_RSAPRIVATE_BLOB,
        NULL, // output buffer
        0, // output len
        &count,
        0 // flags
    );
    if (err)
        logMsgCrash() << "BCryptExportKey(NULL): " << err;
    auto blob = (BYTE *) alloca(count);
    err = (NtStatus) BCryptExportKey(
        bkey,
        NULL,
        BCRYPT_RSAPRIVATE_BLOB,
        blob,
        count,
        &count,
        0 // flags
    );
    if (err)
        logMsgCrash() << "BCryptExportKey: " << err;
    NCRYPT_PROV_HANDLE hProv;
    err = (SecStatus) NCryptOpenStorageProvider(
        &hProv, 
        MS_KEY_STORAGE_PROVIDER, 
        0 // flags
    );
    if (err)
        logMsgCrash() << "NCryptOpenStorageProvider: " << err;
    NCRYPT_KEY_HANDLE nkey = 0;
    err = (SecStatus) NCryptImportKey(
        hProv, 
        NULL, // import key
        BCRYPT_RSAPRIVATE_BLOB, 
        NULL, // parameter list,
        &nkey,
        blob,
        count,
        0 // flags
    );
    if (err)
        logMsgCrash() << "NCryptImportKey: " << err;
    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgCrash() << "NCryptFreeObject: " << err;
    memset(blob, 0, count);
    return nkey;
}


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
            &s_srvCred,
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


    if (SecIsValidHandle(&s_srvCred)) {
        FreeCredentialsHandle(&s_srvCred);
        SecInvalidateHandle(&s_srvCred);
    }
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BCRYPT_KEY_HANDLE createBCryptKey() {
    WinError err{0};
    BCRYPT_ALG_HANDLE bAlgo;
    err = (NtStatus) BCryptOpenAlgorithmProvider(
        &bAlgo,
        BCRYPT_RSA_ALGORITHM,
        NULL, // implementation
        0 // flags
    );
    if (err) 
        logMsgCrash() << "BCryptOpenAlgorithmProvider: " << err;

    BCRYPT_KEY_HANDLE bKey;
    err = (NtStatus) BCryptGenerateKeyPair(bAlgo, &bKey, 2048, 0);
    if (err)
        logMsgCrash() << "BCryptGenerateKeyPair: " << err;
    err = (NtStatus) BCryptFinalizeKeyPair(bKey, 0);
    if (err)
        logMsgCrash() << "BCryptFinalizeKeyPair: " << err;

    err = (NtStatus) BCryptCloseAlgorithmProvider(bAlgo, 0);
    if (err)
        logMsgCrash() << "BCryptCloseAlgorithmProvider: " << err;

    return bKey;
}

//===========================================================================
static NCRYPT_KEY_HANDLE createKeyViaBCrypt() {
    WinError err{0};
    auto bkey = createBCryptKey();

    // Convert to from BCRYPT to NCRYPT
    auto nkey = toNCrypt(bkey);

    err = (NtStatus) BCryptDestroyKey(bkey);
    if (err)
        logMsgCrash() << "BCryptDestroykey: " << err;

    return nkey;
}

//===========================================================================
static NCRYPT_KEY_HANDLE createKey() {
    WinError err{0};
    NCRYPT_PROV_HANDLE hProv;
    err = (SecStatus) NCryptOpenStorageProvider(
        &hProv, 
        MS_KEY_STORAGE_PROVIDER, 
        0 // flags
    );
    if (err)
        logMsgCrash() << "NCryptOpenStorageProvider: " << err;

    NCRYPT_KEY_HANDLE nkey;
    err = (SecStatus) NCryptCreatePersistedKey(
        hProv,
        &nkey,
        BCRYPT_RSA_ALGORITHM,
        NULL, // L"wintls-dimapp",
        AT_SIGNATURE,
        NCRYPT_OVERWRITE_KEY_FLAG // | NCRYPT_MACHINE_KEY_FLAG
    );
    if (err)
        logMsgCrash() << "NCryptCreatePersistedKey: " << err;

    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgCrash() << "NCryptFreeObject(hProv): " << err;

    err = (SecStatus) NCryptFinalizeKey(nkey, NCRYPT_SILENT_FLAG);
    if (err)
        logMsgCrash() << "NCryptFinalizeKey: " << err;

    return nkey;
}

//===========================================================================
static const CERT_CONTEXT * makeCert(string_view issuerName) {
    WinError err{0};
    auto nkey = createKey();
    if (err)
        nkey = createKeyViaBCrypt();

    string name = "CN=";
    name += issuerName;
    // get size of issuer blob
    CERT_NAME_BLOB issuer = {};
    const char * errstr;
    if (!CertStrToName(
        X509_ASN_ENCODING,
        name.data(),
        CERT_X500_NAME_STR,
        NULL, // reserved
        issuer.pbData,
        &issuer.cbData,
        &errstr
    )) {
        logMsgCrash() << "CertStrToName(NULL): " << WinError{};
    }
    // get issuer blob
    string blob(issuer.cbData, 0);
    issuer.pbData = (BYTE *) blob.data();
    if (!CertStrToName(
        X509_ASN_ENCODING,
        name.data(),
        CERT_X500_NAME_STR,
        NULL, // reserved
        issuer.pbData,
        &issuer.cbData,
        &errstr
    )) {
        logMsgCrash() << "CertStrToName(issuer): " << WinError{};
    }

    CRYPT_KEY_PROV_INFO kpinfo = {};
    kpinfo.pwszContainerName = (wchar_t *) L"wintls-dimapp";
    kpinfo.pwszProvName = (wchar_t *) MS_KEY_STORAGE_PROVIDER;
    kpinfo.dwProvType = 0;
    kpinfo.dwFlags = NCRYPT_SILENT_FLAG;
    kpinfo.dwKeySpec = AT_SIGNATURE;

    CRYPT_ALGORITHM_IDENTIFIER sigalgo = {};
    sigalgo.pszObjId = const_cast<char *>(szOID_RSA_SHA256RSA);
    
    SYSTEMTIME startTime;
    {
        // set startTime to 1 week ago
        LARGE_INTEGER tmp;
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        tmp.HighPart = ft.dwHighDateTime;
        tmp.LowPart = ft.dwLowDateTime;
        tmp.QuadPart -= 7 * 24 * 60 * 60 * 10'000'000ull;
        ft.dwHighDateTime = tmp.HighPart;
        ft.dwLowDateTime = tmp.LowPart;
        FileTimeToSystemTime(&ft, &startTime);
    }

    const CERT_CONTEXT * cert = CertCreateSelfSignCertificate(
        nkey,
        &issuer,
        0, // flags
        &kpinfo, // key provider info
        &sigalgo,
        &startTime,
        NULL, // end time - defaults to 1 year
        NULL // extensions
    );
    if (!cert) {
        WinError err;
        logMsgCrash() << "CertCreateSelfSignCertificate: " << err;
    }

    err = (SecStatus) NCryptFreeObject(nkey);
    if (err)
        logMsgCrash() << "NCryptFreeObject: " << err;

    if (!CertAddEnhancedKeyUsageIdentifier(cert, szOID_PKIX_KP_SERVER_AUTH)) {
        err.set();
        logMsgCrash() << "CertAddEnhancedKeyUsageIdentifier: " << err;
    }

    return cert;
}

//===========================================================================
static bool matchHost(string_view authority, string_view host) {
    if (!host.size())
        return true;
    if (DnsNameCompare(authority.data(), host.data()))
        return true;

    // TODO: match against wildcard authority

    return false;
}

//===========================================================================
static bool matchHost(const CERT_CONTEXT * cert, string_view host) {
    // get length of Common Name
    auto nameLen = CertGetNameString(
        cert,
        CERT_NAME_DNS_TYPE,
        CERT_NAME_SEARCH_ALL_NAMES_FLAG,
        (void *) szOID_SUBJECT_ALT_NAME2, 
        0, // pszName
        0 // cchName
    );
    if (!nameLen) {
        // length 0 means no names found
        return false;
    }
    string name(nameLen, 0);
    CertGetNameString(
        cert,
        CERT_NAME_DNS_TYPE,
        CERT_NAME_SEARCH_ALL_NAMES_FLAG,
        (void *) szOID_SUBJECT_ALT_NAME2, 
        name.data(),
        (DWORD) name.size()
    );
    const char * ptr = name.data();
    while (*ptr) {
        string_view authority{ptr};
        if (matchHost(authority, host))
            return true;
        ptr += authority.size() + 1;
    }

    return false;
}


namespace std {

template<>
struct default_delete<const CERT_CONTEXT> {
    void operator()(const CERT_CONTEXT * ptr) const {
        if (ptr) {
            // free (aka decref) always succeeds
            CertFreeCertificateContext(ptr);
        }
    }
};

} // namespace

//===========================================================================
static void getCerts(
    vector<unique_ptr<const CERT_CONTEXT>> & certs,
    string_view host, 
    bool userStore
) {
    auto hstore = HCERTSTORE{};
    DWORD flags = userStore
        ? CERT_SYSTEM_STORE_CURRENT_USER
        : CERT_SYSTEM_STORE_LOCAL_MACHINE;
    hstore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        NULL,
        NULL,
        flags | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
        L"MY"
    );
    if (!hstore) {
        WinError err;
        logMsgCrash() << "CertOpenStore('MY',"
            << (userStore ? "CURRENT_USER" : "LOCAL_MACHINE")
            << "): " << err;
    }

    const char * oids[] = { szOID_PKIX_KP_SERVER_AUTH };
    CERT_ENHKEY_USAGE eu = {};
    eu.cUsageIdentifier = (DWORD) size(oids);
    eu.rgpszUsageIdentifier = (char **) oids;
    
    const CERT_CONTEXT * cert{nullptr};
    unique_ptr<const CERT_CONTEXT> selfsigned;
    unique_ptr<const CERT_CONTEXT> tmpcert;
    for (;;) {
        cert = CertFindCertificateInStore(
            hstore,
            X509_ASN_ENCODING,
            CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG,
            CERT_FIND_ENHKEY_USAGE,
            &eu,
            cert
        );
        if (!cert)
            break;
        if (!matchHost(cert, host))
            continue;

        // TODO: check cert->pCertInfo->NotBefore & NotAfter?

        if (CertCompareCertificateName(
            X509_ASN_ENCODING,
            &cert->pCertInfo->Subject,
            &cert->pCertInfo->Issuer
        )) {
            // self-signed, keep searching for better
            if (!selfsigned) {
                // keep a copy (aka incref)
                selfsigned.reset(CertDuplicateCertificateContext(cert));
            }
            continue;
        }

        // add copy to list of matches
        certs.push_back({});
        certs.back().reset(CertDuplicateCertificateContext(cert));
    }

    if (certs.empty() && selfsigned) 
        certs.push_back(move(selfsigned));

    // no cert found, try to make a new one?
    if (certs.empty()) {
    }
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::winTlsInitialize() {
    shutdownMonitor(&s_cleanup);

    WinError err{0};

    vector<unique_ptr<const CERT_CONTEXT>> certs;
    vector<const CERT_CONTEXT *> ptrs;
    if (kMakeCert) {
        // cert = 
        makeCert("wintls.dimapp");
    } else {
        getCerts(certs, "", false);
    }

    SCHANNEL_CRED cred = {};
    cred.dwVersion = SCHANNEL_CRED_VERSION;
    cred.dwFlags = SCH_USE_STRONG_CRYPTO;   // optional
    if (!certs.empty()) {
        ptrs.reserve(certs.size());
        for (auto && cert : certs)
            ptrs.push_back(cert.get());
        cred.cCreds = (DWORD) size(ptrs);
        cred.paCred = data(ptrs);
    }
    TimeStamp expiry;
    err = (SecStatus) AcquireCredentialsHandle(
        NULL, // principal - null for schannel
        const_cast<LPSTR>(UNISP_NAME), // package
        SECPKG_CRED_INBOUND,
        NULL, // logon id - null for schannel
        &cred,
        NULL, // get key fn
        NULL, // get key arg
        &s_srvCred,
        &expiry // expiration time of credentials
    );
    if (err)
        logMsgCrash() << "AcquireCredentialsHandle: " << err;

    TimePoint expires(Duration(expiry.QuadPart));
    Time8601Str expiryStr(expires, 3, timeZoneMinutes(expires));
    logMsgInfo() << "Credentials expiration: " << expiryStr.c_str();

    // TODO: log warning if the cred is about to expire?
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
