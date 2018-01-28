// Copyright Glen Knowles 2017 - 2018.
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
template<typename T>
static void encodeBlob(
    T & out,
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
        logMsgCrash() << "CryptEncodeObject(" << structType << "): "
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

    CERT_ALT_NAME_ENTRY altNames[2];
    altNames[0].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    altNames[0].pwszDNSName = const_cast<LPWSTR>(L"kcollege");
    altNames[1].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    altNames[1].pwszDNSName = const_cast<LPWSTR>(L"kpower");
    CERT_ALT_NAME_INFO ni;
    ni.cAltEntry = (DWORD) size(altNames);
    ni.rgAltEntry = altNames;

    string extSanData;
    CERT_EXTENSION extSan;
    extSan.pszObjId = (char *) szOID_SUBJECT_ALT_NAME2;
    extSan.fCritical = false;
    encodeBlob(extSan.Value, extSanData, X509_ALTERNATE_NAME, &ni);

    CERT_EXTENSIONS exts;
    exts.cExtension = 1;
    exts.rgExtension = &extSan;

    const CERT_CONTEXT * cert = CertCreateSelfSignCertificate(
        nkey,
        issuer,
        0, // flags
        NULL, // key provider info
        &sigalgo,
        &startTime,
        &endTime, // end time - defaults to 1 year
        &exts // extensions
    );
    if (!cert) {
        err.set();
        logMsgCrash() << "CertCreateSelfSignCertificate: " << err;
        abort();
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
static bool isSelfSigned(const CERT_CONTEXT * cert) {
    return CertCompareCertificateName(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        &cert->pCertInfo->Subject,
        &cert->pCertInfo->Issuer
    );
}


//===========================================================================
static bool hasEnhancedUsage(const CERT_CONTEXT * cert, string_view oid) {
    DWORD cb;
    if (!CertGetEnhancedKeyUsage(
        cert,
        0, // flags
        nullptr,
        &cb
    )) {
        logMsgCrash()
            << "CertGetCertificateContextProperty(ENHKEY_USAGE, NULL): "
            << WinError{};
    }
    string eudata(cb, '\0');
    auto eu = (CERT_ENHKEY_USAGE *) eudata.data();
    if (!CertGetEnhancedKeyUsage(cert, 0, eu, &cb)) {
        WinError err;
        if (err == CRYPT_E_NOT_FOUND) {
            // good for all uses
            return true;
        }
        logMsgCrash()
            << "CertGetCertificateContextProperty(ENHKEY_USAGE): "
            << WinError{};
    }
    for (unsigned i = 0; i < eu->cUsageIdentifier; ++i) {
        if (eu->rgpszUsageIdentifier[i] == oid)
            return true;
    }
    return false;
}

//===========================================================================
static void addCerts(
    vector<unique_ptr<const CERT_CONTEXT>> & certs,
    string_view storeName,
    CertLocation storeLoc,
    string_view subjectKeyId
) {
    string subjectKeyBytes;
    if (!hexToBytes(subjectKeyBytes, subjectKeyId, false)) {
        logMsgError() << "findCert, malformed subject key identifier, {"
            << subjectKeyId << "}";
        return;
    }
    CRYPT_DATA_BLOB subKeyBlob;
    subKeyBlob.cbData = (DWORD) subjectKeyBytes.size();
    subKeyBlob.pbData = (BYTE *) subjectKeyBytes.data();

    auto wstore = toWstring(storeName);
    auto hstore = HCERTSTORE{};
    DWORD flags = (storeLoc << CERT_SYSTEM_STORE_LOCATION_SHIFT);
    hstore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        NULL,
        NULL,
        flags | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
        wstore.data()
    );
    if (!hstore) {
        WinError err;
        logMsgError() << "CertOpenStore('" << storeName << "',"
            << storeLoc.view() << "): " << err;
        return;
    }

    const CERT_CONTEXT * cert{nullptr};
    for (;;) {
        cert = CertFindCertificateInStore(
            hstore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0, // flags
            CERT_FIND_KEY_IDENTIFIER,
            &subKeyBlob,
            cert
        );
        if (!cert) {
            WinError err;
            if (err != CRYPT_E_NOT_FOUND)
                logMsgError() << "CertFindCertificateInStore: " << err;
            break;
        }

        if (!hasEnhancedUsage(cert, szOID_PKIX_KP_SERVER_AUTH))
            continue;

        // has usable private key?
        NCRYPT_KEY_HANDLE nkey;
        DWORD keySpec;
        BOOL mustFree;
        if (!CryptAcquireCertificatePrivateKey(
            cert,
            CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_SILENT_FLAG
                | CRYPT_ACQUIRE_COMPARE_KEY_FLAG
                | CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG,
            NULL,
            &nkey,
            &keySpec,
            &mustFree
        )) {
            WinError err;
            continue;
            // NTE_BAD_PUBLIC_KEY - 8009'0015
            // NTE_SILENT_CONTEXT - 8009'0022
        }
        assert(!mustFree);

        // add copy to list of matches
        certs.push_back({});
        certs.back().reset(CertDuplicateCertificateContext(cert));
    }

    // closing the store (decref) always succeeds
    CertCloseStore(hstore, 0);
}

//===========================================================================
static void getCerts(
    vector<unique_ptr<const CERT_CONTEXT>> & certs,
    const CertKey keys[],
    size_t numKeys
) {
    certs.clear();
    auto ekeys = keys + numKeys;
    for (auto ptr = keys; ptr != ekeys; ++ptr) {
        addCerts(
            certs,
            ptr->storeName,
            ptr->storeLoc,
            ptr->subjectKeyIdentifier
        );
    }

    // no certs found? try to make a new one
    if (certs.empty()) {
        logMsgError() << "No valid Certificate found, creating temporary "
            "self-signed cert.";
        auto cert = makeCert("wintls.dimapp");
        certs.push_back(unique_ptr<const CERT_CONTEXT>(cert));
    }
}


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
    delete ptr;
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
        assert(m_blob.pbData);
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
        logMsgDebug() << "CertStrToName: " << WinError{};
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
*   CredLocation
*
***/

constexpr TokenTable::Token kStoreLocs[] = {
    { CertLocation::kCurrentService,          "Current Service" },
    { CertLocation::kCurrentUser,             "Current User" },
    { CertLocation::kCurrentUserGroupPolicy,  "Current User Group Policy" },
    { CertLocation::kLocalMachine,            "Local Machine" },
    { CertLocation::kLocalMachineEnterprise,  "Local Machine Enterprise" },
    { CertLocation::kLocalMachineGroupPolicy, "Local Machine Group Policy" },
};
const TokenTable s_storeLocTbl(kStoreLocs, size(kStoreLocs));

//===========================================================================
CertLocation & CertLocation::operator=(string_view name) {
    m_value = tokenTableGetEnum(s_storeLocTbl, name, kInvalid);
    return *this;
}

//===========================================================================
string_view CertLocation::view() const {
    return tokenTableGetName(s_storeLocTbl, m_value, "");
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
unique_ptr<CredHandle> Dim::iWinTlsCreateCred(
    const CertKey keys[],
    size_t numKeys
) {
    WinError err{0};

    vector<unique_ptr<const CERT_CONTEXT>> certs;
    getCerts(certs, keys, numKeys);

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
        NULL, // principal - null for SChannel
        const_cast<LPSTR>(UNISP_NAME), // package
        SECPKG_CRED_INBOUND,
        NULL, // login id - null for SChannel
        &cred,
        NULL, // get key function
        NULL, // get key argument
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

    return handle;
}

//===========================================================================
bool Dim::iWinTlsIsSelfSigned(const CERT_CONTEXT * cert) {
    return isSelfSigned(cert);
}

//===========================================================================
bool Dim::iWinTlsMatchHost(const CERT_CONTEXT * cert, string_view host) {
    return matchHost(cert, host);
}
