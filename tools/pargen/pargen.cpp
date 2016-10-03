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
    (void)valid;
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
static bool internalTest() {
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
static void usageError(ostream & os) {
    os << 1 + R"(
usage: pargen [-?|-h|--help] [-f|--mark-functions=#] [-C|--no-callbacks] 
              [--min-core] [-B|--no-build] [-D|--no-dedup] [--state-detail]
              [--no-write-functions]
              <source file[.abnf]>
)";
    appSignalShutdown(EX_USAGE);
}

//===========================================================================
static void printSyntax(ostream & os) {
    os << "pargen v0.1.0 (" __DATE__ ") - simplistic parser generator";
    os << R"(
usage: pargen [<options>] <source file[.abnf]

Options:
    -?, -h, --help    
        Print this message.
    -f, --mark-functions <0-2>
        Function tag preprocessing:
            0 - no change to function tags
            1 - add function tags to break rule recursion (default)
            2 - same as #1, but clear existing tags first
    -C, --no-callbacks
        Suppress all callback events, reduces generated parser down to 
        pass/fail syntax check.

Testing options:
    --[no-]min-core
        Use reduced core rules: ALPHA, DIGIT, CRLF, HEXDIG, NEWLINE, 
        VCHAR, and WSP are shortened to fewer (usually 1) characters.
    -B, --no-build
        Skip building the state tree, only stub versions of parser are
        generated.
    -D, --no-dedup
        Skip purging duplicate entries from the state tree, duplicates
        occur when multiple paths through the rules end with the same
        series of transitions.
    --[no-]state-detail
        Include details of the states as comments in the generated parser
        code - may be extremely verbose.
    --no-write-functions
        Skip generation of recursion breaking dependent functions (enabled
        by default).
        NOTE: generated files may not be compilable
    --test
        runs internal test of ABNF parsing logic

For additional information, see:
    https://github.com/gknowles/dimapp/tree/master/tools/pargen
)";
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public ITaskNotify,
                    public ILogNotify,
                    public IFileReadNotify {
    int m_argc;
    char ** m_argv;
    string m_source;

public:
    Application(int argc, char * argv[]);
    void onTask() override;

    // ILogNotify
    void onLog(LogType type, const std::string & msg) override;

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
    CmdParser cmd;
    auto & srcfile = cmd.addArg<experimental::filesystem::path>("source file");
    auto & help = cmd.addOpt<bool>("? h help");
    auto & test = cmd.addOpt<bool>("test");
    cmd.addOpt(&s_allRules, "min-core", s_allRules);
    cmd.addOpt(&s_cmdopts.markRecursion, "f mark-functions", 1);
    cmd.addOpt(&s_cmdopts.includeCallbacks, "!C callbacks", true);
    cmd.addOpt(&s_cmdopts.buildStateTree, "!B build", true);
    cmd.addOpt(&s_cmdopts.dedupStateTree, "!D dedup", true);
    cmd.addOpt(&s_cmdopts.writeStatePositions, "state-detail");
    cmd.addOpt(&s_cmdopts.writeFunctions, "write-functions", true);
    if (!cmd.parse(cerr, m_argc, m_argv)) {
        return usageError(cerr);
    }
    if (*help || m_argc == 1) {
        printSyntax(cout);
        return appSignalShutdown(EX_OK);
    }
    if (*test) {
        if (!internalTest()) {
            appSignalShutdown(EX_SOFTWARE);
        } else {
            appSignalShutdown(EX_OK);
        }
        return;
    }
    if (!srcfile) {
        cerr << "No value given for " << srcfile << endl;
        return usageError(cerr);
    }

    assert(srcfile);
    if (!srcfile->has_extension())
        srcfile->replace_extension("abnf");
    fileReadBinary(this, m_source, *srcfile);
}

//===========================================================================
void Application::onLog(LogType type, const std::string & msg) {
    if (type == kLogError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout << "ERROR: " << msg << endl;
    } else {
        cout << msg << endl;
    }
}

//===========================================================================
void Application::onFileEnd(int64_t offset, IFile * file) {
    if (!file)
        return appSignalShutdown(EX_USAGE);

    Grammar rules;
    getCoreRules(rules);
    if (parseAbnf(rules, m_source)) {
        writeParserFiles(rules);
    }
    appSignalShutdown(EX_OK);
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
    logAddNotify(&app);
    return appRun(app);
}
