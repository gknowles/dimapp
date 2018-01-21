// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// tlstest.cpp - dim test tls
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

enum TestType : unsigned {
    fTestReset = 0x01,
    fTestClient = 0x02,
};

struct Test {
    const char * name;
    TestType flags;
    string input;
    bool result;
    string output;
    string appdata;
};

} // namespace


/****************************************************************************
*
*   Test vectors
*
***/

// clang-format off
const Test s_tests[] = {
    {"connect", fTestReset | fTestClient, {}, true, {}, {}},
    {"simple client hello",
     fTestReset,
     {
         kClientHello, // msg_type
         0, 0, 10, // length
         // ClientHello
         3, 4, // client_version
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, // random[32]
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         0, // legacy_session_id<0..32>
         0, 2, '\xcc', '\xa9', // cipher_suites<2..2^16-2>
         0, 0, // legacy_compression_methods<1..2*8-1> (null compression)
         0, 49, // extensions<0..2^16-1>
         // KeyShare
         '\x88', 2, // extension type
         0, 37, // extension_data<0..2^16-1>
         0, 29, // group (kEcdhX25519)
         0, 33, // key_exchange<1..2^16-1>
         32, // point<1..2^8-1>
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
         // SignatureAndHashAlgorithm
         0, 13, // extension type
         0, 4, // extension_data<0..2^16-1>
         0, 2, // supported_signature_algorithms<2..2^16-2>
         4, 5, // (sha256, eddsa)
     },
     true,
     {
         // ServerHello
         // EncryptedExtensions
         // CertificateRequest
         // ServerConfiguration
     },
     {}},
};
// clang-format on


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    const TlsCipherSuite kCiphers[] = {
        TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
    };
    const char kHost[] = "example.com";

    CharBuf output;
    CharBuf plain;
    TlsConnHandle client{};
    TlsConnHandle server{};
    bool result = 0;
    for (auto && test : s_tests) {
        cout << "Test - " << test.name << endl;
        TlsConnHandle & conn = (test.flags & fTestClient) ? client : server;
        if ((test.flags & fTestReset) && conn)
            tlsClose(conn);
        if (!conn) {
            if (test.flags & fTestClient) {
                conn = tlsConnect(&output, kHost, kCiphers, size(kCiphers));
            } else {
                conn = tlsAccept(kCiphers, size(kCiphers));
            }
        }
        result = tlsRecv(
            &output,
            &plain,
            conn,
            data(test.input),
            size(test.input)
        );
        if (result != test.result) {
            logMsgError() << "result: " << result << " != " << test.result
                << " (FAILED)";
        }
        if (output.compare(test.output) != 0)
            logMsgError() << "headers mismatch (FAILED)";
    }
    tlsClose(client);
    tlsClose(server);

    if (int errs = logGetMsgCount(kLogTypeError)) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** TEST FAILURES: " << errs << endl;
        appSignalShutdown(EX_SOFTWARE);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(EX_OK);
    }
}


/****************************************************************************
*
*   External
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_DELAY_FREE_MEM_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    return appRun(app, argc, argv);
}
