// tokentabletest.cpp - dim test tokentable
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
*   Helpers
*
***/


/****************************************************************************
*
*   Application
*
***/

namespace {

class Application : public IAppNotify, public ILogNotify {
    // IAppNotify
    void onAppRun() override;

    // ILogNotify
    void onLog(LogType type, const string & msg) override;

    int m_errors{0};
};

} // namespace

//===========================================================================
void Application::onLog(LogType type, const string & msg) {
    if (type >= kLogError) {
        ConsoleScopedAttr attr(kConsoleError);
        m_errors += 1;
        cout << "ERROR: " << msg << endl;
    } else {
        cout << msg << endl;
    }
}

//===========================================================================
void Application::onAppRun() {
    int line = 0;

    const TokenTable::Token numbers[] = {
        {1, "one"},
        {2, "two"},
        {3, "three"},
        {4, "four"},
        {5, "five"},
        {21, "twenty-one"},
        {22, "twenty-two"},
    };
    const TokenTable numberTbl(numbers, size(numbers));

    EXPECT(tokenTableGetEnum(numberTbl, "invalid", 0) == 0);
    for (auto && tok : numbers) {
        EXPECT(tokenTableGetEnum(numberTbl, tok.name, 0) == tok.id);
    }

    if (m_errors) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** " << m_errors << " FAILURES" << endl;
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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    Application app;
    logAddNotify(&app);
    return appRun(app, argc, argv);
}
