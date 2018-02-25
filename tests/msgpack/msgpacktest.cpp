// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// msgpacktest.cpp - dim test msgpack
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(...)                                                         \
    if (!bool(__VA_ARGS__)) {                                               \
        logMsgError() << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
                      << #__VA_ARGS__ << ") failed";                        \
    }


/****************************************************************************
*
*   MsgPack2Json
*
***/

namespace {

class MsgPack2Json : public MsgPack::IStreamParserNotify {
public:
    MsgPack2Json(CharBuf * out);

private:
    // Inherited via IStreamParserNotify
    bool startDoc() override;
    bool endDoc() override;
    bool startArray(size_t length) override;
    bool startMap(size_t length) override;
    bool valuePrefix(std::string_view val, bool first) override;
    bool value(std::string_view val) override;
    bool value(bool val) override;
    bool value(double val) override;
    bool value(int64_t val) override;
    bool value(uint64_t val) override;
    bool value(std::nullptr_t) override;

    template<typename T>
    bool addValue(T val);

    CharBuf & m_buf;
    JBuilder m_bld;
    vector<pair<bool, size_t>> m_stack;
    string m_tmp;
};

} // namespace

//===========================================================================
MsgPack2Json::MsgPack2Json(CharBuf * out)
    : m_buf{*out}
    , m_bld{m_buf}
{}

//===========================================================================
bool MsgPack2Json::startDoc() {
    m_buf.clear();
    return true;
}

//===========================================================================
bool MsgPack2Json::endDoc() {
    return true;
}

//===========================================================================
bool MsgPack2Json::startArray(size_t length) {
    m_stack.push_back({false, length});
    m_bld.array();
    return true;
}

//===========================================================================
bool MsgPack2Json::startMap(size_t length) {
    m_stack.push_back({true, 2 * length});
    m_bld.object();
    return true;
}

//===========================================================================
bool MsgPack2Json::valuePrefix(std::string_view val, bool first) {
    m_tmp.append(val);
    return true;
}

//===========================================================================
bool MsgPack2Json::value(std::string_view val) {
    if (!m_tmp.empty()) {
        m_tmp.append(val);
        val = string_view{m_tmp};
    }
    if (m_stack.empty()) {
        m_bld.value(val);
    } else {
        auto & [map, count] = m_stack.back();
        if (map && count % 2 == 0) {
            m_bld.member(val);
            count -= 1;
            return true;
        }

        m_bld.value(val);
        if (!--count) {
            m_stack.pop_back();
            m_bld.end();
        }
    }
    return true;
}

//===========================================================================
template<typename T>
bool MsgPack2Json::addValue(T val) {
    if (m_stack.empty()) {
        m_bld.value(val);
        return true;
    }

    auto & [map, count] = m_stack.back();
    if (map && count % 2 == 0)
        return false; // object keys must be strings

    m_bld.value(val);
    if (!--count) {
        m_stack.pop_back();
        m_bld.end();
    }
    return true;
}

//===========================================================================
bool MsgPack2Json::value(bool val) {
    return addValue(val);
}

//===========================================================================
bool MsgPack2Json::value(double val) {
    return addValue(val);
}

//===========================================================================
bool MsgPack2Json::value(int64_t val) {
    return addValue(val);
}

//===========================================================================
bool MsgPack2Json::value(uint64_t val) {
    return addValue(val);
}

//===========================================================================
bool MsgPack2Json::value(std::nullptr_t) {
    return addValue(nullptr);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    int line = 0;

    CharBuf buf;
    MsgPack::Builder bld(&buf);
    bld.map(2);
    bld.element("compact", true);
    bld.element("schema", 0);

    const char kExpect1[] = "\x82\xa7" "compact\xc3\xa6schema\x0";
    auto ev = string_view(kExpect1, size(kExpect1) - 1);
    EXPECT(buf.view() == ev);

    CharBuf buf2;
    MsgPack2Json m2j(&buf2);
    MsgPack::StreamParser parser(&m2j);
    unsigned used;
    parser.parse(&used, buf.view());
    EXPECT(buf2.view() == "{\"compact\":true,\n\"schema\":0\n}\n");

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
