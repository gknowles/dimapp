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
static CredHandle s_hcred;


/****************************************************************************
*
*   ServerConn
*
***/

//===========================================================================
void ServerConn::accept() {
    WinError err{0};
    //err = (SecStatus) AcceptSecurityContext(
    //    &s_hcred,
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
static NCRYPT_KEY_HANDLE NCryptFromBCrypt (BCRYPT_KEY_HANDLE bkey) {
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
        logMsgCrash() << "BCryptExportKey(NULL), " << err;
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
        logMsgCrash() << "BCryptExportKey(), " << err;
    NCRYPT_PROV_HANDLE hProv;
    err = (SecStatus) NCryptOpenStorageProvider(
        &hProv, 
        MS_KEY_STORAGE_PROVIDER, 
        0 // flags
    );
    if (err)
        logMsgCrash() << "NCryptOpenStorageProvider(), " << err;
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
        logMsgCrash() << "NCryptImportKey(), " << err;
    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgCrash() << "NCryptFreeObject(), " << err;
    memset(blob, 0, count);
    return nkey;
}

//===========================================================================
static NCRYPT_KEY_HANDLE CreateKeyViaBCrypt() {
    WinError err{0};
    BCRYPT_ALG_HANDLE bAlgo;
    err = (NtStatus) BCryptOpenAlgorithmProvider(
        &bAlgo,
        BCRYPT_RSA_ALGORITHM,
        NULL, // implementation
        0 // flags
    );
    if (err) 
        logMsgCrash() << "BCryptOpenAlgorithmProvider(), " << err;

    BCRYPT_KEY_HANDLE bKey;
    err = (NtStatus) BCryptGenerateKeyPair(bAlgo, &bKey, 2048, 0);
    if (err)
        logMsgCrash() << "BCryptGenerateKeyPair(), " << err;
    err = (NtStatus) BCryptFinalizeKeyPair(bKey, 0);
    if (err)
        logMsgCrash() << "BCryptFinalizeKeyPair(), " << err;

    // Convert to from BCRYPT to NCRYPT
    NCRYPT_KEY_HANDLE nKey = NCryptFromBCrypt(bKey);

    err = (NtStatus) BCryptCloseAlgorithmProvider(bAlgo, 0);
    if (err)
        logMsgCrash() << "BCryptCloseAlgorithmProvider(), " << err;
    err = (NtStatus) BCryptDestroyKey(bKey);
    if (err)
        logMsgCrash() << "BCryptDestroykey(), " << err;

    return nKey;
}

//===========================================================================
static NCRYPT_KEY_HANDLE CreateKey() {
    WinError err{0};
    NCRYPT_PROV_HANDLE hProv;
    err = (SecStatus) NCryptOpenStorageProvider(
        &hProv, 
        MS_KEY_STORAGE_PROVIDER, 
        0 // flags
    );
    if (err)
        logMsgCrash() << "NCryptOpenStorageProvider(), " << err;

    NCRYPT_KEY_HANDLE hKey;
    err = (SecStatus) NCryptCreatePersistedKey(
        hProv,
        &hKey,
        BCRYPT_RSA_ALGORITHM,
        NULL, // L"wintls-dimapp",
        AT_SIGNATURE,
        NCRYPT_OVERWRITE_KEY_FLAG // | NCRYPT_MACHINE_KEY_FLAG
    );
    if (err)
        logMsgCrash() << "NCryptCreatePersistedKey(), " << err;

    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgCrash() << "NCryptFreeObject(hProv), " << err;

    err = (SecStatus) NCryptFinalizeKey(hKey, NCRYPT_SILENT_FLAG);
    if (err)
        logMsgCrash() << "NCryptFinalizeKey(), " << err;

    return hKey;
}

//===========================================================================
static const CERT_CONTEXT * MakeCert(string_view issuerName) {
    WinError err{0};
    auto nKey = CreateKey();
    if (err)
        nKey = CreateKeyViaBCrypt();

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
        logMsgCrash() << "CertStrToName(), " << WinError{};
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
        logMsgCrash() << "CertStrToName(), " << WinError{};
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
        nKey,
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
        logMsgCrash() << "CertCreateSelfSignCertificate(), " << err;
    }

    err = (SecStatus) NCryptFreeObject(nKey);
    if (err)
        logMsgCrash() << "NCryptFreeObject(), " << err;

    if (!CertAddEnhancedKeyUsageIdentifier(cert, szOID_PKIX_KP_SERVER_AUTH)) {
        err.set();
        logMsgCrash() << "CertAddEnhancedKeyUsageIdentifier(), " << err;
    }

    return cert;
}

//===========================================================================
void Dim::winTlsInitialize() {
    WinError err{0};

    const CERT_CONTEXT * cert = nullptr;

    SCHANNEL_CRED cred = {};
    cred.dwVersion = SCHANNEL_CRED_VERSION;
    if (!kMakeCert) {
        cert = MakeCert("wintls.dimapp");
        cred.cCreds = 1;
        cred.paCred = &cert;
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
        &s_hcred,
        &expiry // expiration time of credentials
    );
    if (err)
        logMsgCrash() << "AcquireCredentialsHandle(), " << err;

    if (cert)
        CertFreeCertificateContext(cert);
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
