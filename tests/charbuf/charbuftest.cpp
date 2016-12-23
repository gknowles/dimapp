// charbuftest.cpp - dim test charbuf
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#define EXPECT(e) \
    if (!bool(e)) { \
    logMsgError() << "Line " \
        << (line ? line : __LINE__) \
        << ": EXPECT(" << #e << ") failed"; \
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

class Application : public ITaskNotify, public ILogNotify {
    // ITaskNotify
    void onTask() override;

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
void Application::onTask() {
    int line = 0;
    CharBuf buf;
    buf.assign("abcdefgh");
    EXPECT(to_string(buf) == "abcdefgh"); // to_string

    // replace in the middle
    buf.replace(3, 3, "DEF"); // same size
    EXPECT(buf == "abcDEFgh"s);
    buf.replace(3, 3, "MN"); // shrink
    EXPECT(buf == "abcMNgh"s);
    buf.replace(3, 2, "def"); // expand
    EXPECT(buf == "abcdefgh"s);

    // replace at the end
    buf.replace(5, 3, "FGH"); // same size
    EXPECT(buf == "abcdeFGH"s);
    buf.replace(5, 3, "fg"); // shrink
    EXPECT(buf == "abcdefg"s);
    buf.replace(5, 2, "FGH"); // expand
    EXPECT(buf == "abcdeFGH"s);

    buf.replace(3, 3, "XYZ", 3);
    EXPECT(buf == "abcXYZGH"s);

    if (m_errors) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "*** " << m_errors << " FAILURES" << endl;
        appSignalShutdown(1);
    } else {
        cout << "All tests passed" << endl;
        appSignalShutdown(0);
    }
}


/****************************************************************************
*
*   External
*
***/

int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);
    Application app;
    logAddNotify(&app);
    return appRun(app);
}
