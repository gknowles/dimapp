// main.cpp - dimapp test file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public ITaskNotify,
                    public IFileWriteNotify,
                    public IFileReadNotify,
                    public ILogNotify {
    int m_argc;
    char ** m_argv;

public:
    Application(int argc, char * argv[]);

    // ITaskNotify
    void onTask() override;

    // IFileWriteNotify
    void onFileWrite(
        int written,
        const char data[],
        int bytes,
        int64_t offset,
        IFile * file) override;

    // IFileReadNotify
    void onFileEnd(int64_t offset, IFile * file) override;

    // ILogNotify
    void onLog(LogType type, const string & msg) override;

    int m_errors{0};
};
} // namespace

//===========================================================================
Application::Application(int argc, char * argv[])
    : m_argc(argc)
    , m_argv(argv) {}

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
    Cli cli;
    auto & fn = cli.opt<string>("[dat file]", "metrics.dat");
    if (!cli.parse(m_argc, m_argv))
        return appSignalUsageError();

    size_t psize = filePageSize();
    auto file =
        fileOpen(*fn, IFile::kCreat | IFile::kTrunc | IFile::kReadWrite 
            | IFile::kBlocking);
    fileWriteSync(file.get(), 0, "aaaa", 4);
    const char * base;
    if (!fileOpenView(base, file.get(), 1001 * psize))
        return appSignalShutdown(EX_DATAERR);
    fileExtendView(file.get(), 1001 * psize);
    unsigned num = 0;
    static char v;
    for (size_t i = 1; i < 1000; ++i) {
        v = 0;
        fileWriteSync(file.get(), i * psize, "bbbb", 4);
        v = base[i * psize];
        if (v == 'b')
            num += 1;
    }
    fileExtendView(file.get(), psize);
    static char buf[5] = {};
    for (unsigned i = 0; i < 100; ++i) {
        fileReadSync(buf, 4, file.get(), psize);
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

//===========================================================================
void Application::onFileWrite(
    int written,
    const char data[],
    int bytes,
    int64_t offset,
    IFile * file) {}

//===========================================================================
void Application::onFileEnd(int64_t offset, IFile * file) {}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app(argc, argv);
    int code = appRun(app);
    return code;
}
