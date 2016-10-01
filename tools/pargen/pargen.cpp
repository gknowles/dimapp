// pargen.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static RunOptions s_cmdopts;


/****************************************************************************
*
*   Helpers
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
        writeParser(oh, ocpp, rules, s_cmdopts);
        oh.close();
        ocpp.close();
    }
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    cout << "Elapsed time: " << elapsed.count() << " seconds" << endl;
}

//===========================================================================
static bool internalTest () {
    bool valid;
    Grammar rules;
    getCoreRules(rules);
    rules.addOption("%root", "LWSP");
    rules.addOption("%api.prefix", "Test");
    valid = processOptions(rules);
    cout << "Process options: " << valid << endl;

    ostringstream abnf;
    for (auto && rule : rules.rules()) {
        writeRule(abnf, rule, 79, "");
    }
    rules.clear();
    valid = parseAbnf(rules, abnf.str());
    cout << "Valid: " << valid << endl;
    if (!valid)
        return false;

    ostringstream o1;
    for (auto && rule : rules.rules()) {
        writeRule(o1, rule, 79, "");
    }

    ostringstream o2;
    rules.clear();
    valid = parseAbnf(rules, o1.str());
    for (auto && rule : rules.rules()) {
        writeRule(o2, rule, 79, "");
    }
    valid = valid && (o1.str() == o2.str());
    cout << "Round trip: " << valid << endl;
    return valid;
}

//===========================================================================
static void printUsage() {
    cout << "usage: pargen [<options>] <source file[.abnf]>\n";
}

//===========================================================================
static void printSyntax() {
    cout << "pargen v0.1.0 (" __DATE__ ") - simplistic parser generator\n";
    printUsage();
    cout << R"(
Options:
    -f#, --mark-functions=#
    -C, --no-callbacks
    -B, --no-build-tree
    -D, --no-dedup-tree
    --[no-]state-detail
    --[no-]write-functions

   --test            runs internal test of ABNF parsing logic

   -?, -h, --help    print this message
)";
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

enum {
    kExitTestFailure = kExitFirstAvailable
};

//===========================================================================
void Application::onTask() {
    CmdLine::Parser cmd;
    auto& srcfile = cmd.addArg<string>("source file");
    auto& help = cmd.addOpt<bool>("? h help");
    auto& test = cmd.addOpt<bool>("test");
    cmd.addOpt(&s_cmdopts.markRecursion, "f mark-functions", 1);
    cmd.addOpt(&s_cmdopts.includeCallbacks, "!C callbacks", true);
    cmd.addOpt(&s_cmdopts.buildStateTree, "!B build-tree", true);
    cmd.addOpt(&s_cmdopts.dedupStateTree, "!D dedup-tree", true);
    cmd.addOpt(&s_cmdopts.writeStatePositions, "state-detail");
    cmd.addOpt(&s_cmdopts.writeFunctions, "write-functions", true);
    if (!cmd.parse(m_argc, m_argv)) {
        printUsage();
        return appSignalShutdown(kExitBadArgs);
    }
    if (*help || m_argc == 1) {
        printSyntax();
        return appSignalShutdown(kExitSuccess);
    }
    if (*test) {
        if (!internalTest()) {
            appSignalShutdown(kExitTestFailure);
        } else {
            appSignalShutdown(kExitSuccess);
        }
        return;
    }
    if (!srcfile) {
        logMsgError() << "No value given for " << "source file";
        printUsage();
        return appSignalShutdown(kExitBadArgs);
    }

    fileReadBinary(this, m_source, *srcfile);
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
