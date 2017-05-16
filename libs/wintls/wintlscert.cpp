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

class CertName {
public:
    CertName() {}
    CertName(const CertName & from);
    CertName(CertName && from);
    ~CertName();

    void reset();
    void reset(CERT_NAME_BLOB blob);
    CERT_NAME_BLOB release();

    bool parse(const char src[]);
    string str() const;

    operator CERT_NAME_BLOB * () { return &m_blob; }
    operator const CERT_NAME_BLOB * () const { return &m_blob; }

    explicit operator bool() const { return m_blob.cbData; }

private:
    CERT_NAME_BLOB m_blob{};
};

} // namespace



/****************************************************************************
*
*   default_delete
*
***/

//===========================================================================
void std::default_delete<const CERT_CONTEXT>::operator()(
    const CERT_CONTEXT * ptr
) const {
    if (ptr) {
        // free (aka decref) always succeeds
        CertFreeCertificateContext(ptr);
    }
}

//===========================================================================
void std::default_delete<CredHandle>::operator()(CredHandle * ptr) const {
    if (SecIsValidHandle(ptr)) {
        WinError err = (SecStatus) FreeCredentialsHandle(ptr);
        if (err)
            logMsgCrash() << "FreeCredentialsHandle: " << err;
        SecInvalidateHandle(ptr);
    }
}


/****************************************************************************
*
*   Helpers
*
***/

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
        L"wintls-dimapp",
        AT_SIGNATURE,
        NCRYPT_OVERWRITE_KEY_FLAG
    );
    if (err)
        logMsgCrash() << "NCryptCreatePersistedKey: " << err;

    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgCrash() << "NCryptFreeObject(hProv): " << err;

    DWORD val = 2048;
    err = (SecStatus) NCryptSetProperty(
        nkey, 
        NCRYPT_LENGTH_PROPERTY,
        (BYTE *) &val,
        sizeof(val),
        NCRYPT_SILENT_FLAG
    );
    if (err)
        logMsgCrash() << "NCryptSetProperty(LENGTH): " << err;

    val = NCRYPT_ALLOW_EXPORT_FLAG | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;
    err = (SecStatus) NCryptSetProperty(
        nkey, 
        NCRYPT_EXPORT_POLICY_PROPERTY,
        (BYTE *) &val,
        sizeof(val),
        NCRYPT_SILENT_FLAG
    );
    if (err)
        logMsgCrash() << "NCryptSetProperty(EXPORT_POLICY): " << err;

    err = (SecStatus) NCryptFinalizeKey(nkey, NCRYPT_SILENT_FLAG);
    if (err)
        logMsgCrash() << "NCryptFinalizeKey: " << err;

    return nkey;
}

//===========================================================================
static void encodeObject(
    CRYPT_OBJID_BLOB & out,
    string & outData,
    const char structType[],
    const void * structInfo
) {
    if (!CryptEncodeObject(
        X509_ASN_ENCODING,
        structType,
        structInfo,
        NULL,
        &out.cbData
    )) {
        logMsgCrash() << "CryptEncodeObject(" << structType << ", NULL): " 
            << WinError{};
    }
    outData.resize(out.cbData);
    if (!CryptEncodeObject(
        X509_ASN_ENCODING,
        structType,
        structInfo,
        (BYTE *) outData.data(),
        &out.cbData
    )) {
        logMsgCrash() << "CryptEncodeObject(" << structType << ", NULL): " 
            << WinError{};
    }
    assert(out.cbData == outData.size());
    out.pbData = (BYTE *) outData.data();
}

//===========================================================================
static const CERT_CONTEXT * makeCert(string_view issuerName) {
    WinError err{0};
    auto nkey = createKey();

    string name = "CN=";
    name += issuerName;
    CertName issuer;
    if (!issuer.parse(name.c_str()))
        logMsgCrash() << "CertName.parse failed";

    CRYPT_KEY_PROV_INFO kpinfo = {};
    kpinfo.pwszContainerName = (wchar_t *) L"wintls-dimapp";
    kpinfo.pwszProvName = (wchar_t *) MS_KEY_STORAGE_PROVIDER;
    kpinfo.dwProvType = 0;
    kpinfo.dwFlags = NCRYPT_SILENT_FLAG;
    kpinfo.dwKeySpec = AT_SIGNATURE;

    CRYPT_ALGORITHM_IDENTIFIER sigalgo = {};
    sigalgo.pszObjId = const_cast<char *>(szOID_RSA_SHA256RSA);
    
    SYSTEMTIME startTime;
    SYSTEMTIME endTime;
    {
        // set startTime to 1 week ago
        static const int64_t kStartDelta = 
            7 * 24 * 60 * 60 * kClockTicksPerSecond;
        LARGE_INTEGER tmp;
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        // set endTime to 1 year from now
        FileTimeToSystemTime(&ft, &endTime);
        endTime.wYear += 1;
        // set startTime to 1 week ago
        tmp.HighPart = ft.dwHighDateTime;
        tmp.LowPart = ft.dwLowDateTime;
        tmp.QuadPart -= kStartDelta;
        ft.dwHighDateTime = tmp.HighPart;
        ft.dwLowDateTime = tmp.LowPart;
        FileTimeToSystemTime(&ft, &startTime);
    }

    CERT_ALT_NAME_ENTRY neIssuer;
    neIssuer.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    neIssuer.pwszDNSName = const_cast<LPWSTR>(L"kcollege");
    CERT_ALT_NAME_INFO ni;
    ni.cAltEntry = 1;
    ni.rgAltEntry = &neIssuer;

    string extSanData;
    CERT_EXTENSION extSan;
    extSan.pszObjId = (char *) szOID_SUBJECT_ALT_NAME2;
    extSan.fCritical = false;
    encodeObject(extSan.Value, extSanData, X509_ALTERNATE_NAME, &ni);
    CERT_EXTENSIONS exts;
    exts.cExtension = 1;
    exts.rgExtension = &extSan;

    const CERT_CONTEXT * cert = CertCreateSelfSignCertificate(
        nkey,
        issuer,
        0, // flags
        NULL, // &kpinfo, // key provider info
        &sigalgo,
        &startTime,
        &endTime, // end time - defaults to 1 year
        &exts // extensions
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

    CRYPT_DATA_BLOB data;
    const wchar_t s_friendlyName[] = L"dimapp-wintls";
    data.pbData = (BYTE *) s_friendlyName;
    data.cbData = sizeof(s_friendlyName);
    if (!CertSetCertificateContextProperty(
        cert,
        CERT_FRIENDLY_NAME_PROP_ID,
        0,
        &data
    )) {
        logMsgCrash() << "CertSetCertificateContextProperty(FRIENDLY_NAME): "
            << WinError{};
    }

    // verify access to cert's private key
    DWORD keySpec;
    BOOL mustFreeNkey;
    if (!CryptAcquireCertificatePrivateKey(
        cert,
        CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG, // flags
        NULL, // parameters
        &nkey,
        &keySpec,
        &mustFreeNkey
    )) {
        logMsgCrash() << "CryptAcquireCertificatePrivateKey: " << WinError{};
    }
    if (mustFreeNkey) {
        err = (SecStatus) NCryptFreeObject(nkey);
        if (err)
            logMsgCrash() << "NCryptFreeObject: " << err;
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

//===========================================================================
static void getCerts(
    vector<unique_ptr<const CERT_CONTEXT>> & certs,
    string_view host, 
    bool serviceStore
) {
    auto hstore = HCERTSTORE{};
    DWORD flags = serviceStore
        ? CERT_SYSTEM_STORE_CURRENT_SERVICE
        //: CERT_SYSTEM_STORE_CURRENT_USER;
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
            << (serviceStore ? "CURRENT_SERVICE" : "CURRENT_USER")
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

    // closing the store (decref) always succeeds
    CertCloseStore(hstore, 0);

    if (certs.empty() && selfsigned) {
        CertName sub;
        sub.reset(selfsigned->pCertInfo->Subject);
        logMsgDebug() << "Using cert selfsigned by '" << sub.str() << "'";
        sub.release();
        certs.push_back(move(selfsigned));
    }

    // no cert found, try to make a new one?
    if (certs.empty()) {
        logMsgCrash() << "No certificates found";
    }
}


/****************************************************************************
*
*   CertName
*
***/

//===========================================================================
CertName::CertName(const CertName & from) {
    if (from) {
        m_blob.pbData = (BYTE *) malloc(from.m_blob.cbData);
        memcpy(m_blob.pbData, from.m_blob.pbData, from.m_blob.cbData);
    }
}

//===========================================================================
CertName::CertName(CertName && from) {
    swap(m_blob, from.m_blob);
}

//===========================================================================
CertName::~CertName() {
    reset();
}

//===========================================================================
void CertName::reset() {
    if (m_blob.cbData) {
        free(m_blob.pbData);
        m_blob.pbData = nullptr;
        m_blob.cbData = 0;
    }
}

//===========================================================================
void CertName::reset(CERT_NAME_BLOB blob) {
    reset();
    m_blob = blob;
}

//===========================================================================
CERT_NAME_BLOB CertName::release() {
    auto tmp = m_blob;
    m_blob.pbData = nullptr;
    m_blob.cbData = 0;
    return tmp;
}

//===========================================================================
bool CertName::parse(const char src[]) {
    const char * errstr;

    // get length of blob
    if (!CertStrToName(
        X509_ASN_ENCODING,
        src,
        CERT_X500_NAME_STR,
        NULL, // reserved
        NULL, // pbData
        &m_blob.cbData,
        &errstr
    )) {
        logMsgDebug() << "CertStrToName(NULL): " << WinError{};
        return false;
    }
    // get blob
    m_blob.pbData = (BYTE *) malloc(m_blob.cbData);
    if (!CertStrToName(
        X509_ASN_ENCODING,
        src,
        CERT_X500_NAME_STR,
        NULL, // reserved
        m_blob.pbData,
        &m_blob.cbData,
        &errstr
    )) {
        logMsgDebug() << "CertStrToName(issuer): " << WinError{};
        return false;
    }

    return true;
}

//===========================================================================
string CertName::str() const {
    string name;
    DWORD len = CertNameToStr(
        X509_ASN_ENCODING, 
        const_cast<CERT_NAME_BLOB *>(&m_blob), 
        CERT_SIMPLE_NAME_STR, 
        NULL, 
        0
    );
    name.resize(len);
    CertNameToStr(
        X509_ASN_ENCODING, 
        const_cast<CERT_NAME_BLOB *>(&m_blob), 
        CERT_SIMPLE_NAME_STR, 
        name.data(), 
        len
    );
    assert(len == name.size());
    name.pop_back();
    return name;
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
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
unique_ptr<CredHandle> Dim::iWinTlsCreateCred() {
    WinError err{0};

    vector<unique_ptr<const CERT_CONTEXT>> certs;
    if (!kMakeCert) {
        auto cert = makeCert("wintls.dimapp");
        certs.push_back(unique_ptr<const CERT_CONTEXT>(cert));
    } else {
        getCerts(certs, "", appFlags() & fAppIsService);
    }

    vector<const CERT_CONTEXT *> ptrs;
    SCHANNEL_CRED cred = {};
    cred.dwVersion = SCHANNEL_CRED_VERSION;
    cred.dwFlags = SCH_CRED_NO_SYSTEM_MAPPER 
        | SCH_USE_STRONG_CRYPTO;
    if (!certs.empty()) {
        for (auto && cert : certs)
            ptrs.push_back(cert.get());
        cred.cCreds = (DWORD) size(ptrs);
        cred.paCred = data(ptrs);
    }

    auto handle = make_unique<CredHandle>();
    TimeStamp expiry;
    err = (SecStatus) AcquireCredentialsHandle(
        NULL, // principal - null for schannel
        const_cast<LPSTR>(UNISP_NAME), // package
        SECPKG_CRED_INBOUND,
        NULL, // logon id - null for schannel
        &cred,
        NULL, // get key fn
        NULL, // get key arg
        handle.get(),
        &expiry // expiration time of credentials
    );
    if (err)
        logMsgCrash() << "AcquireCredentialsHandle: " << err;
    // SEC_E_UNKNOWN_CREDENTIALS - 8009'030d
    // NTE_BAD_KEYSET - 8009'0016

    TimePoint expires(Duration(expiry.QuadPart));
    Time8601Str expiryStr(expires, 3, timeZoneMinutes(expires));
    logMsgInfo() << "Credentials expiration: " << expiryStr.c_str();

    // TODO: log warning if the cred is about to expire?

    return handle;
}
