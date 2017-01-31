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
class Application 
    : public ITaskNotify 
    , public IFileWriteNotify
{
    int m_argc;
    char ** m_argv;

public:
    Application(int argc, char * argv[]);
    void onTask() override;
    void onFileWrite(int written, const char data[], int bytes,
        int64_t offset, IFile * file) override;
};
} // namespace

//===========================================================================
Application::Application(int argc, char * argv[])
    : m_argc(argc)
    , m_argv(argv) {}

//===========================================================================
void Application::onTask () {
    Cli cli;
    auto & fn = cli.opt<string>("[dat file]", "metrics.dat");
    if (!cli.parse(m_argc, m_argv))
        return appSignalUsageError();

    size_t psize = filePageSize();
    auto file = fileOpen(*fn, IFile::kCreat | IFile::kTrunc | IFile::kReadWrite);
    fileWrite(this, file.get(), 0, "aaaa", 4);
    const char * base;
    if (!fileOpenView(base, file.get(), 3 * psize))
        return appSignalShutdown(EX_DATAERR);
    fileWrite(this, file.get(), psize, "bbbb", 4);
    fileExtendView(file.get(), psize);

    appSignalShutdown(EX_OK);
}

//===========================================================================
void Application::onFileWrite(
    int written, 
    const char data[], 
    int bytes,
    int64_t offset, 
    IFile * file) {
}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char *argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app(argc, argv);
    int code = appRun(app);
    return code;
}
