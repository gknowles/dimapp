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
    void accept();
    bool recv(CharBuf * out, CharBuf * data, string_view src);
    void send(CharBuf * out, string_view src);
private:
    CtxtHandle m_hctxt;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<WinTlsConnHandle, ServerConn> s_conns;
static CredHandle s_srvCred;


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
void ServerConn::accept() {
    WinError err{0};
    //err = (SecStatus) AcceptSecurityContext(
    //    &s_srvCred,
    //    NULL, // current context
    //    NULL, // input buffer
    //    ASC_REQ_STREAM
    //    ASC_REQ_CONFIDENTIALITY
    //    CONNECTION
    //SECBUFFER_APPLICATION_PROTOCOLS
}

//===========================================================================
bool ServerConn::recv(CharBuf * out, CharBuf * data, string_view src) {
    return false;
}

//===========================================================================
void ServerConn::send(CharBuf * out, string_view src) {
}


/****************************************************************************
*
*   Private API
*
***/


/****************************************************************************
*
*   Public API
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
static bool match(string_view authority, string_view host) {
    if (!host.size())
        return true;
    if (DnsNameCompare(authority.data(), host.data()))
        return true;

    // TODO: match against wildcard authority

    return false;
}

//===========================================================================
static bool match(const CERT_CONTEXT * cert, string_view host) {
#if 0
    WinError err{0};

    const char * oid = szOID_SUBJECT_ALT_NAME2;
    auto ext = CertFindExtension(
        oid,
        cert->pCertInfo->cExtension,
        cert->pCertInfo->rgExtension
    );
    if (!ext) {
        oid = szOID_SUBJECT_ALT_NAME;
        ext = CertFindExtension(
            oid,
            cert->pCertInfo->cExtension,
            cert->pCertInfo->rgExtension
        );
    }
    if (ext) {
        CERT_ALT_NAME_INFO * info;
        DWORD infoLen;
        if (!CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            ext->pszObjId,
            ext->Value.pbData,
            ext->Value.cbData,
            CRYPT_DECODE_ALLOC_FLAG,
            nullptr,
            &info, 
            &infoLen
        )) {
            err.set();
            logMsgCrash() << "CryptDecodeObjectEx: " << err;
        }
        int i = 0;
        for (; i < info->cAltEntry; ++i) {
            auto & alt = info->rgAltEntry[i];
            if (alt.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME
                && match(alt.pwszDNSName, host)
            ) {
                break;
            }
        }
        LocalFree(info);
        return i != info->cAltEntry;
    }
#endif

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
        if (match(authority, host))
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
static unique_ptr<const CERT_CONTEXT> getCert(
    string_view host, 
    bool userStore
) {
    unique_ptr<const CERT_CONTEXT> cert;

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
    
    unique_ptr<const CERT_CONTEXT> selfsigned;
    for (;;) {
        cert.reset(CertFindCertificateInStore(
            hstore,
            X509_ASN_ENCODING,
            CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG,
            CERT_FIND_ENHKEY_USAGE,
            &eu,
            cert.release()
        ));
        if (!cert)
            break;
        if (!match(cert.get(), host))
            continue;

        // TODO: check cert->pCertInfo->NotBefore & NotAfter
        // TODO: prefer certs with server auth usage

        if (CertCompareCertificateName(
            X509_ASN_ENCODING,
            &cert->pCertInfo->Subject,
            &cert->pCertInfo->Issuer
        )) {
            // self-signed, keep searching for better
            if (!selfsigned) {
                // keep a copy (aka incref)
                selfsigned.reset(CertDuplicateCertificateContext(cert.get()));
            }
            continue;
        }

        break;
    }

    if (!cert && selfsigned)
        return selfsigned;

    // no cert found, try to make a new one?
    if (!cert) {
    }
    return cert;
}

//===========================================================================
void Dim::winTlsInitialize() {
    WinError err{0};

    unique_ptr<const CERT_CONTEXT> cert;

    SCHANNEL_CRED cred = {};
    cred.dwVersion = SCHANNEL_CRED_VERSION;
    if (kMakeCert) {
        // cert = 
        makeCert("wintls.dimapp");
    } else {
        cert = getCert("kpower", false);
    }
    if (cert) {
        const CERT_CONTEXT * certs[] = { cert.get() };
        cred.cCreds = (DWORD) size(certs);
        cred.paCred = certs;
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

    // TODO: log warning if the cred will expire soon
}

//===========================================================================
WinTlsConnHandle Dim::winTlsAccept() {
    auto conn = new ServerConn;
    conn->accept();
    return s_conns.insert(conn);
}

//===========================================================================
void Dim::winTlsClose(WinTlsConnHandle h) {
    s_conns.erase(h);
}

//===========================================================================
bool Dim::winTlsRecv(
    WinTlsConnHandle h,
    CharBuf * out,
    CharBuf * data,
    string_view src) {
    auto conn = s_conns.find(h);
    return conn->recv(out, data, src);
}

//===========================================================================
void Dim::winTlsSend(
    WinTlsConnHandle h,
    CharBuf * out,
    string_view src
) {
    auto conn = s_conns.find(h);
    conn->send(out, src);
}
