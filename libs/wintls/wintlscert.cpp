// Copyright Glen Knowles 2017 - 2025.
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
    CertName(CertName && from) noexcept;
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
        logMsgFatal() << "NCryptOpenStorageProvider: " << err;

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
        logMsgFatal() << "NCryptCreatePersistedKey: " << err;

    err = (SecStatus) NCryptFreeObject(hProv);
    if (err)
        logMsgFatal() << "NCryptFreeObject(hProv): " << err;

    DWORD val = 2048;
    err = (SecStatus) NCryptSetProperty(
        nkey,
        NCRYPT_LENGTH_PROPERTY,
        (BYTE *) &val,
        sizeof val,
        NCRYPT_SILENT_FLAG
    );
    if (err)
        logMsgFatal() << "NCryptSetProperty(LENGTH): " << err;

    val = NCRYPT_ALLOW_EXPORT_FLAG | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;
    err = (SecStatus) NCryptSetProperty(
        nkey,
        NCRYPT_EXPORT_POLICY_PROPERTY,
        (BYTE *) &val,
        sizeof val,
        NCRYPT_SILENT_FLAG
    );
    if (err)
        logMsgFatal() << "NCryptSetProperty(EXPORT_POLICY): " << err;

    err = (SecStatus) NCryptFinalizeKey(nkey, NCRYPT_SILENT_FLAG);
    if (err)
        logMsgFatal() << "NCryptFinalizeKey: " << err;

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
        logMsgFatal() << "CryptEncodeObject(" << structType << ", NULL): "
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
        logMsgFatal() << "CryptEncodeObject(" << structType << "): "
            << WinError{};
    }
    assert(out.cbData == outData.size());
    out.pbData = (BYTE *) outData.data();
}

//===========================================================================
static bool hasEnhancedUsage(const CERT_CONTEXT * cert, string_view oid) {
    DWORD cb{0};
    if (!CertGetEnhancedKeyUsage(
        cert,
        0, // flags
        nullptr,
        &cb
    )) {
        logMsgFatal()
            << "CertGetEnhancedKeyUsage(NULL): "
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
        logMsgFatal()
            << "CertGetEnhancedKeyUsage: "
            << WinError{};
    }
    for (unsigned i = 0; i < eu->cUsageIdentifier; ++i) {
        if (eu->rgpszUsageIdentifier[i] == oid)
            return true;
    }
    return false;
}

//===========================================================================
static void addAltNameExt(
    vector<CERT_EXTENSION> * extVec,
    vector<string> * blobBufs,
    const vector<string_view> & dnsNames,
    const vector<string_view> & ipAddrs
) {
    if (dnsNames.empty() && ipAddrs.empty())
        return;

    vector<CERT_ALT_NAME_ENTRY> anes;
    vector<wstring> dnames;
    for (auto && n : dnsNames) {
        dnames.push_back(toWstring(n));
        auto & ane = anes.emplace_back();
        ane.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
        ane.pwszDNSName = dnames.back().data();
    }
    vector<string> ipBytes;
    for (auto && n : ipAddrs) {
        HostAddr ip;
        if (!parse(&ip, n)) {
            logMsgError() << "makeCert: invalid IP address: " << n;
            continue;
        }
        auto & bytes = ipBytes.emplace_back();
        if (ip.family() == HostAddr::kIpv4) {
            bytes.resize(4);
            hton32(bytes.data(), ip.ipv4());
        } else {
            bytes.resize(16);
            memcpy(bytes.data(), ip.data, sizeof ip.data);
        }
        auto & ane = anes.emplace_back();
        ane.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
        ane.IPAddress.cbData = (DWORD) bytes.size();
        ane.IPAddress.pbData = (BYTE *) bytes.data();
    }
    CERT_ALT_NAME_INFO ani = {};
    ani.cAltEntry = (DWORD) anes.size();
    ani.rgAltEntry = anes.data();

    auto & ce = extVec->emplace_back();
    ce.pszObjId = (char *) szOID_SUBJECT_ALT_NAME2;
    ce.fCritical = false;
    encodeBlob(
        ce.Value,
        blobBufs->emplace_back(),
        X509_ALTERNATE_NAME,
        &ani
    );
}

//===========================================================================
static const CERT_CONTEXT * makeCert(
    string_view issuerName,
    const vector<string_view> & dnsNames,
    const vector<string_view> & ipAddrs
) {
    WinError err{0};
    auto nkey = createKey();

    string name = "CN=";
    name += issuerName;
    CertName issuer;
    if (!issuer.parse(name.c_str()))
        logMsgFatal() << "CertName.parse failed";

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

    vector<CERT_EXTENSION> extVec;
    vector<string> blobBufs;
    addAltNameExt(&extVec, &blobBufs, dnsNames, ipAddrs);

    CERT_EXTENSIONS exts;
    exts.cExtension = (DWORD) extVec.size();
    exts.rgExtension = extVec.data();

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
        logMsgFatal() << "CertCreateSelfSignCertificate: " << err;
        abort();
    }

    err = (SecStatus) NCryptFreeObject(nkey);
    if (err)
        logMsgFatal() << "NCryptFreeObject: " << err;

    if (!CertAddEnhancedKeyUsageIdentifier(cert, szOID_PKIX_KP_SERVER_AUTH)) {
        err.set();
        logMsgFatal() << "CertAddEnhancedKeyUsageIdentifier: " << err;
    }

    CRYPT_DATA_BLOB data;
    const wchar_t s_friendlyName[] = L"dimapp-wintls";
    data.pbData = (BYTE *) s_friendlyName;
    data.cbData = sizeof s_friendlyName;
    if (!CertSetCertificateContextProperty(
        cert,
        CERT_FRIENDLY_NAME_PROP_ID,
        0,
        &data
    )) {
        logMsgFatal() << "CertSetCertificateContextProperty(FRIENDLY_NAME): "
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
        logMsgFatal() << "CryptAcquireCertificatePrivateKey: " << WinError{};
    }
    if (mustFreeNkey) {
        err = (SecStatus) NCryptFreeObject(nkey);
        if (err)
            logMsgFatal() << "NCryptFreeObject: " << err;
    }

    return cert;
}

//===========================================================================
static bool matchHost(string_view authority, string_view host) {
    if (!host.size())
        return true;
    if (DnsNameCompare_W(toWstring(authority).data(), toWstring(host).data()))
        return true;

    // TODO: match against wildcard authority

    return false;
}

//===========================================================================
static bool matchHost(const CERT_CONTEXT * cert, string_view host) {
    // get length of Common Name
    auto nameLen = CertGetNameStringW(
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
    wstring wname(nameLen, 0);
    CertGetNameStringW(
        cert,
        CERT_NAME_DNS_TYPE,
        CERT_NAME_SEARCH_ALL_NAMES_FLAG,
        (void *) szOID_SUBJECT_ALT_NAME2,
        wname.data(),
        (DWORD) wname.size()
    );
    auto name = toString(wname);
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
static void addCerts(
    vector<unique_ptr<const CERT_CONTEXT>> & certs,
    const CertKey & key
) {
    string idBytes;
    if (!hexToBytes(&idBytes, key.value, false)) {
        logMsgError() << "findCert, malformed search key, {"
            << key.value << "}";
        return;
    }
    string issuerBytes;
    CertName issuer;
    CERT_ID certId;
    switch (key.type) {
    case CertKey::kSubjectKeyIdentifier:
        certId.dwIdChoice = CERT_ID_KEY_IDENTIFIER;
        certId.KeyId.cbData = (DWORD) idBytes.size();
        certId.KeyId.pbData = (BYTE *) idBytes.data();
        break;
    case CertKey::kThumbprint:
        certId.dwIdChoice = CERT_ID_SHA1_HASH;
        certId.HashId.cbData = (DWORD) idBytes.size();
        certId.HashId.pbData = (BYTE *) idBytes.data();
        break;
    case CertKey::kSerialNumber:
        certId.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
        reverse(idBytes.begin(), idBytes.end());
        certId.IssuerSerialNumber.SerialNumber.cbData = (DWORD) idBytes.size();
        certId.IssuerSerialNumber.SerialNumber.pbData = (BYTE *) idBytes.data();
        if (!issuer.parse(key.issuer.data())) {
            logMsgError() << "findCert, malformed issuer, {"
                << key.issuer << "}";
            return;
        }
        certId.IssuerSerialNumber.Issuer = *issuer;
        break;
    default:
        return;
    }

    auto wstore = toWstring(key.storeName);
    auto hstore = HCERTSTORE{};
    DWORD flags = (key.storeLoc << CERT_SYSTEM_STORE_LOCATION_SHIFT);
    hstore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        NULL,
        NULL,
        flags | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
        wstore.data()
    );
    if (!hstore) {
        WinError err;
        logMsgError() << "CertOpenStore('" << key.storeName << "',"
            << key.storeLoc.view() << "): " << err;
        return;
    }

    const CERT_CONTEXT * cert = nullptr;
    for (;;) {
        cert = CertFindCertificateInStore(
            hstore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0, // flags
            CERT_FIND_CERT_ID,
            &certId,
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
            // NTE_BAD_KEYSET - 8009'0016
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
    span<const CertKey> keys,
    const vector<string_view> & dnsNames,
    const vector<string_view> & ipAddrs
) {
    certs.clear();
    for (auto&& val : keys)
        addCerts(certs, val);

    // no certs found? try to make a new one
    if (certs.empty()) {
        if (dnsNames.empty() && ipAddrs.empty()) {
            logMsgWarn() << "No valid Certificate found, creating temporary "
                "self-signed cert.";
        } else {
            // Don't warn if there are explicit parameters for self-signing
            // under the assumption that it's intentional.
            logMsgInfo() << "Creating temporary self-signed certificate.";
        }
        auto cert = makeCert("wintls.dimapp", dnsNames, ipAddrs);
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
            logMsgFatal() << "FreeCredentialsHandle: " << err;
        SecInvalidateHandle(ptr);
    }
    delete ptr;
}


/****************************************************************************
*
*   CertKey
*
***/

namespace {

constexpr TokenTable::Token s_certKeyTypes[] = {
    { CertKey::kSubjectKeyIdentifier, "subjectKeyId" },
    { CertKey::kThumbprint, "thumbprint" },
    { CertKey::kSerialNumber, "serialNumber" },
};
const TokenTable s_certKeyTbl(s_certKeyTypes);

} // namespace

//===========================================================================
const char * Dim::toString(CertKey::Type type, const char def[]) {
    return s_certKeyTbl.findName(type, def);
}

//===========================================================================
CertKey::Type Dim::fromString(std::string_view src, CertKey::Type def) {
    return s_certKeyTbl.find(src, def);
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
CertName::CertName(CertName && from) noexcept {
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
    const wchar_t * errstr;
    auto wsrc = toWstring(src);

    // get length of blob
    if (!CertStrToNameW(
        X509_ASN_ENCODING,
        wsrc.data(),
        CERT_X500_NAME_STR | CERT_NAME_STR_FORCE_UTF8_DIR_STR_FLAG,
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
    if (!CertStrToNameW(
        X509_ASN_ENCODING,
        wsrc.data(),
        CERT_X500_NAME_STR | CERT_NAME_STR_FORCE_UTF8_DIR_STR_FLAG,
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
    wstring wname;
    DWORD len = CertNameToStrW(
        X509_ASN_ENCODING,
        const_cast<CERT_NAME_BLOB *>(&m_blob),
        CERT_SIMPLE_NAME_STR,
        NULL,
        0
    );
    wname.resize(len);
    CertNameToStrW(
        X509_ASN_ENCODING,
        const_cast<CERT_NAME_BLOB *>(&m_blob),
        CERT_SIMPLE_NAME_STR,
        wname.data(),
        len
    );
    assert(len == wname.size());
    wname.pop_back();
    return toString(wname);
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
const TokenTable s_storeLocTbl(kStoreLocs);

//===========================================================================
CertLocation & CertLocation::operator=(string_view name) {
    m_value = s_storeLocTbl.find(name, kInvalid);
    return *this;
}

//===========================================================================
string_view CertLocation::view() const {
    return s_storeLocTbl.findName(m_value, "");
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
unique_ptr<CredHandle> Dim::iWinTlsCreateCred(
    span<const CertKey> keys,
    const vector<string_view> & dnsNamesForSelfSigned,
    const vector<string_view> & ipAddrsForSelfSigned
) {
    WinError err{0};

    vector<unique_ptr<const CERT_CONTEXT>> certs;
    getCerts(certs, keys, dnsNamesForSelfSigned, ipAddrsForSelfSigned);

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
    err = (SecStatus) AcquireCredentialsHandleW(
        NULL, // principal - null for SChannel
        const_cast<LPWSTR>(UNISP_NAME), // package
        SECPKG_CRED_INBOUND,
        NULL, // login id - null for SChannel
        &cred,
        NULL, // get key function
        NULL, // get key argument
        handle.get(),
        &expiry // expiration time of credentials
    );
    if (err)
        logMsgFatal() << "AcquireCredentialsHandle: " << err;
    // SEC_E_UNKNOWN_CREDENTIALS - 8009'030d
    // SEC_E_NO_CREDENTIALS - 8009'030e
    // NTE_BAD_KEYSET - 8009'0016

    TimePoint expires(Duration(expiry.QuadPart));
    Time8601Str expiryStr(expires, 3);
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
