// Copyright Glen Knowles 2017.
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
    void operator()(const CERT_CONTEXT * ptr) const;
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

    CertLocation & operator=(const CertLocation & right) = default;
    CertLocation & operator=(std::string_view name);

    explicit operator bool() const { return m_value != kInvalid; }
    operator Type() const { return m_value; }
    std::string_view view() const;

private:
    Type m_value{kInvalid};
};

struct CertKey {
    std::string_view storeName;
    CertLocation storeLoc;
    std::string_view subjectKeyIdentifier;
};

std::unique_ptr<CredHandle> iWinTlsCreateCred(
    const CertKey keys[],
    size_t numKeys
);

bool iWinTlsIsSelfSigned(const CERT_CONTEXT * cert);
bool iWinTlsMatchHost(const CERT_CONTEXT * cert, std::string_view host);

} // namespace
