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

#define EXPECT(e)                                                           \
    if (!bool(e)) {                                                         \
        logMsgError() << "Line " << (line ? line : __LINE__) << ": EXPECT(" \
                      << #e << ") failed";                                  \
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

    // replace in the middle with sz
    buf.replace(3, 3, "DEF"); // same size
    EXPECT(buf == "abcDEFgh");
    buf.replace(3, 3, "MN"); // shrink
    EXPECT(buf == "abcMNgh");
    buf.replace(3, 2, "def"); // expand
    EXPECT(buf == "abcdefgh");

    // replace at the end with sz
    buf.replace(5, 3, "FGH"); // same size
    EXPECT(buf == "abcdeFGH");
    buf.replace(5, 3, "fg"); // shrink
    EXPECT(buf == "abcdefg");
    buf.replace(5, 2, "FGH"); // expand
    EXPECT(buf == "abcdeFGH");

    // replace in the middle with len
    buf.replace(3, 3, "XYZ", 3); // same size
    EXPECT(buf == "abcXYZGH");
    buf.replace(3, 3, "DE", 2); // shrink
    EXPECT(buf == "abcDEGH");
    buf.replace(3, 2, "def", 3); // expand
    EXPECT(buf == "abcdefGH");

    // replace at the end with len
    buf.replace(6, 2, "gh", 2); // same size
    EXPECT(buf == "abcdefgh");
    buf.replace(5, 3, "FG", 2); // shrink
    EXPECT(buf == "abcdeFG");
    buf.replace(5, 2, "fgh", 3); // expand
    EXPECT(buf == "abcdefgh");

    // insert amount smaller than tail
    buf.replace(4, 0, "x"); // insert one char
    EXPECT(buf == "abcdxefgh");
    buf.replace(4, 1, ""); // erase one char
    EXPECT(buf == "abcdefgh");

    buf.assign(5000, 'a');
    buf.insert(4094, "x");
    EXPECT(buf.compare(4092, 5, "aaxaa") == 0);

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
