// pargen.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

#ifdef NDEBUG
static bool s_allRules = true;
#else
static bool s_allRules = false;
#endif

//===========================================================================
static void getCoreRules(Grammar & rules) {
	const char * coreRules = R"(
ALPHA   =  %x41-5A / %x61-7A    ; A-Z / a-z
BIT     =  "0" / "1"
CHAR    =  %x01-7F
CR      =  %x0D
CRLF    =  CR LF
CTL     =  %x00-1F / %x7F
DIGIT   =  %x30-39				; 0-9
DQUOTE  =  %x22
HEXDIG  =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
HTAB    =  %x09
LF      =  %x0A
LWSP    =  *(WSP / NEWLINE WSP)
NEWLINE =  LF / CRLF
OCTET   =  %x00-FF
SP      =  %x20
VCHAR   =  %x21-7E
WSP     =  SP / HTAB
)";
    if (!s_allRules) {
        coreRules = R"(
ALPHA   =  %x5A
BIT     =  "0" / "1"
CHAR    =  %x01-7F
CR      =  %x0D
CRLF    =  CR
CTL     =  %x00-1F / %x7F
DIGIT   =  %x39
DQUOTE  =  %x22
HEXDIG  =  "C"
HTAB    =  %x09
LF      =  %x0A
LWSP    =  *(WSP / NEWLINE WSP)
NEWLINE =  CR
OCTET   =  %x00-FF
SP      =  %x20
VCHAR   =  %x56
WSP     =  SP
)";
    }
    [[maybe_unused]] bool valid = parseAbnf(rules, coreRules);
    assert(valid);
    (void) valid;
}

//===========================================================================
static void writeParserFiles(Grammar & rules) {
    TimePoint start = Clock::now();
    if (processOptions(rules)) {
        ofstream oh(rules.optionString(kOptionApiHeaderFile));
        ofstream ocpp(rules.optionString(kOptionApiCppFile));
        writeParser(oh, ocpp, rules);
        oh.close();
        ocpp.close();
    }
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    cout << "Elapsed time: " << elapsed.count() << " seconds" << endl;
}


//===========================================================================
static void printUsage() {
    cout << "usage: pargen [<options>] [<source file[.abnf]>]\n";
}

//===========================================================================
static void printSyntax() {
    cout << "pargen v0.1.0 (" __DATE__ ") - simplistic parser generator\n";
    printUsage();
    cout << "\n"
        << "Options:\n"
        << "  -?, -h, --help    print this message\n"
        ;
}

/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public ITaskNotify, public IFileReadNotify {
    int m_argc;
    char ** m_argv;
    string m_source;

public:
    Application(int argc, char * argv[]);
    void onTask() override;

    // IFileReadNotify
    void onFileEnd(int64_t offset, IFile * file) override;
};
} // namespace

//===========================================================================
Application::Application(int argc, char * argv[])
    : m_argc(argc)
    , m_argv(argv) {}

//===========================================================================
void Application::onTask() {
    CmdLine::Argument<string> srcfile("source file");
    CmdLine::Option<bool> help("? h help");
    if (!CmdLine::parseOptions(m_argc, m_argv)) {
        printUsage();
        return appSignalShutdown(kExitBadArgs);
    }
    if (help) {
        printSyntax();
        return appSignalShutdown();
    }

    if (srcfile) {
        fileReadBinary(this, m_source, *srcfile);
        return;
    }

    Grammar rules;
    getCoreRules(rules);

    writeParserFiles(rules);

    ostringstream abnf;
    for (auto && rule : rules.rules()) {
        abnf << rule.name << " = " << rule << '\n';
    }
    rules.clear();
    bool valid = parseAbnf(rules, abnf.str());
    cout << "Valid: " << valid << endl;

    ostringstream o1;
    for (auto && rule : rules.rules()) {
        o1 << rule.name << " = " << rule << '\n';
    }

    ostringstream o2;
    rules.clear();
    valid = parseAbnf(rules, o1.str());
    for (auto && rule : rules.rules()) {
        o2 << rule.name << " = " << rule << '\n';
    }
    cout << "Round trip: " << (o1.str() == o2.str()) << endl;

    appSignalShutdown(kExitSuccess);
}

//===========================================================================
void Application::onFileEnd(int64_t offset, IFile * file) {
    Grammar rules;
    getCoreRules(rules);
    if (parseAbnf(rules, m_source)) {
        writeParserFiles(rules);
    }
    appSignalShutdown(kExitSuccess);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    consoleEnableCtrlC(false);
    Application app(argc, argv);
    return appRun(app);
}
