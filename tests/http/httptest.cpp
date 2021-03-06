// Copyright Glen Knowles 2016 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// httptest.cpp - dim test http
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

struct NameValue {
    const char * name;
    const char * value;
    unsigned flags;

    bool operator==(const NameValue & right) const;
};

struct TestMsg {
    vector<NameValue> headers;
    const char * body;
};

struct Test {
    const char * name;
    bool reset;
    string input;
    bool result;
    string output;
    vector<TestMsg> msgs;
};

} // namespace


/****************************************************************************
*
*   Test vectors
*
***/

// clang-format off
const Test s_tests[] = {
    {
        "/a", true,
        {
            'P','R','I',' ','*',' ','H','T','T','P','/','2','.','0','\r','\n',
            '\r','\n',
            'S', 'M', '\r', '\n', '\r', '\n',
            0, 0, 0, 4, 0, 0, 0, 0, 0, // settings
            0, 0, 38, 1, 5, 0, 0, 0, 1, // headers (38 bytes) + eoh + eos
            '\x82', // :method: GET
            '\x87', // :scheme: https
            0x44, 0x09, '/','r','e','s','o','u','r','c','e',
            0x66, 0x0b, 'e','x','a','m','p','l','e','.','o','r','g',
            0x53, 0x0a, 'i','m','a','g','e','/','j','p','e','g',
        },
        true,
        {
            0, 0, 0, 4, 0, 0, 0, 0, 0, // settings
            0, 0, 0, 4, 1, 0, 0, 0, 0, // settings + ack
        },
        {
            {
                {
                    {":method", "GET"},
                    {":scheme", "https"},
                    {":path", "/resource"},
                    {"host", "example.org"},
                    {"accept", "image/jpeg"},
                },
                ""
            },
        }
    },
};
// clang-format on


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
void oldTest() {
    CharBuf output;
    HttpConnHandle conn{};
    bool result;
    vector<unique_ptr<HttpMsg>> msgs;
    for (auto && test : s_tests) {
        cout << "Test - " << test.name << endl;
        if (test.reset && conn)
            httpClose(conn);
        if (!conn)
            conn = httpAccept();
        result =
            httpRecv(&output, &msgs, conn, data(test.input), size(test.input));
        if (result != test.result) {
            logMsgError() << "result: " << result << " != " << test.result
                          << " (FAILED)";
        }
        if (output.compare(test.output) != 0)
            logMsgError() << "headers mismatch (FAILED)";
        auto tmi = test.msgs.begin();
        for (auto && msg : msgs) {
            if (tmi == test.msgs.end()) {
                logMsgError() << "too many messages (FAILED)";
                break;
            }
            if (msg->body().compare(tmi->body) != 0)
                logMsgError() << "body mismatch (FAILED)";
            auto thi = tmi->headers.begin(), ethi = tmi->headers.end();
            for (auto && hdr : msg->headers()) {
                for (auto && hv : hdr) {
                    if (thi == ethi) {
                        logMsgError() << "expected fewer headers";
                        goto FINISHED_HEADERS;
                    }
                    if (strcmp(thi->name, hdr.m_name) != 0
                        || strcmp(thi->value, hv.m_value) != 0) {
                        logMsgError() << "header mismatch, '" << hdr.m_name
                                      << ": " << hv.m_value << "', expected '"
                                      << thi->name << ": " << thi->value
                                      << "' (FAILED)";
                    }
                    ++thi;
                }
            }
        FINISHED_HEADERS:
            if (thi != ethi)
                logMsgError() << "expected more headers (FAILED)";
        }
        msgs.clear();
    }
    httpClose(conn);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    oldTest();

    bool result;
    vector<unique_ptr<HttpMsg>> msgs;
    CharBuf cbuf;
    CharBuf sbuf;
    auto hsrv = httpAccept();
    auto hcli = httpConnect(&cbuf);
    HttpRequest msg;
    msg.addHeaderRef(kHttp_Method, "get");
    msg.addHeaderRef(kHttp_Scheme, "https");
    msg.addHeaderRef(kHttp_Path, "/");
    int streamId = httpRequest(&cbuf, hcli, msg);
    ignore = streamId;

    while (cbuf.size()) {
        result = httpRecv(&sbuf, &msgs, hsrv, cbuf.data(), cbuf.size());
        cbuf.clear();
        assert(result);
        msgs.clear();
        if (sbuf.empty())
            break;
        result = httpRecv(&cbuf, &msgs, hcli, sbuf.data(), sbuf.size());
        sbuf.clear();
        assert(result);
        msgs.clear();
    }
    httpClose(hsrv);
    httpClose(hcli);

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
    return appRun(app, argc, argv);
}
