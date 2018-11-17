// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// wintls.h - dim windows platform tls
//
// Uses windows SChannel to secure appsocket connections
#pragma once

#include "cppconf/cppconf.h"

#include <string_view>


/****************************************************************************
*
*   default_delete (for unique_ptr)
*
***/

namespace std {

template<>
struct default_delete<const CERT_CONTEXT> {
    void operator()(CERT_CONTEXT const * ptr) const;
};

template<>
struct default_delete<CredHandle> {
    void operator()(CredHandle * ptr) const;
};

} // namespace


namespace Dim {


/****************************************************************************
*
*   Internal APIs
*
***/

struct CertLocation {
    enum Type {
        kInvalid,
        kCurrentService = CERT_SYSTEM_STORE_CURRENT_SERVICE_ID,
        kCurrentUser = CERT_SYSTEM_STORE_CURRENT_USER_ID,
        kCurrentUserGroupPolicy =
            CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID,
        kLocalMachine = CERT_SYSTEM_STORE_LOCAL_MACHINE_ID,
        kLocalMachineEnterprise = CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID,
        kLocalMachineGroupPolicy =
            CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID,
    };

    CertLocation & operator=(CertLocation const & right) = default;
    CertLocation & operator=(std::string_view name);

    explicit operator bool() const { return m_value != kInvalid; }
    operator Type() const { return m_value; }
    std::string_view view() const;

private:
    Type m_value{kInvalid};
};

struct CertKey {
    enum Type {
        kInvalid,
        kSubjectKeyIdentifier,
        kSerialNumber,
        kThumbprint,
    };

    std::string_view storeName;
    CertLocation storeLoc;
    Type type{kInvalid};
    std::string_view value;
    std::string_view issuer; // used with kSerialNumber
};
char const * toString(CertKey::Type type, char const def[] = nullptr);
CertKey::Type fromString(std::string_view src, CertKey::Type def);

std::unique_ptr<CredHandle> iWinTlsCreateCred(
    CertKey const keys[],
    size_t numKeys,
    std::vector<std::string_view> const & dnsNamesForSelfSigned = {},
    std::vector<std::string_view> const & ipAddrsForSelfSigned = {}
);

bool iWinTlsIsSelfSigned(CERT_CONTEXT const * cert);
bool iWinTlsMatchHost(CERT_CONTEXT const * cert, std::string_view host);

} // namespace
