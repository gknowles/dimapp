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
static bool s_minRules = false;
#else
static bool s_minRules = true;
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
    if (s_minRules) {
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


/****************************************************************************
*
*   LogTask
*
***/

namespace {
class LogTask : public ITaskNotify {
public:
    LogTask(LogType type, const string & msg);

    // ITaskNotify
    void onTask() override;

private:
    LogType m_type;
    string m_msg;
};
int s_errors;
} // namespace

//===========================================================================
LogTask::LogTask(LogType type, const string & msg)
    : m_type(type)
    , m_msg(msg) {
    if (type == kLogError)
        s_errors += 1;
}

//===========================================================================
void LogTask::onTask() {
    if (m_type == kLogError) {
        ConsoleScopedAttr attr(kConsoleError);
        cout << "ERROR: " << m_msg << endl;
    } else {
        cout << m_msg << endl;
    }
    delete this;
}


/****************************************************************************
*
*   Command line syntax
*
***/

//===========================================================================
static void
usageError(ostream & os, const string & msg, const string & detail = {}) {
    os << msg;
    if (detail.size())
        os << '\n' << detail;
    os << R"(
usage: pargen [-?, -h, --help] [-f, --mark-functions=#] [-C, --no-callbacks] 
              [--min-core] [-B, --no-build] [-D, --no-dedup] [--state-detail]
              [--no-write-functions]
              <source file[.abnf]> [<root rule>]
)";
    appSignalShutdown(EX_USAGE);
}


/****************************************************************************
*
*   Application
*
***/

namespace {
class Application : public ILogNotify,
                    public ITaskNotify,
                    public IFileReadNotify {
    int m_argc;
    char ** m_argv;
    string m_source;
    TaskQueueHandle m_logQ;
    experimental::filesystem::path m_srcfile;
    string m_root;

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
    m_logQ = taskCreateQueue("Logging", 1);

    Cli cli;
    // header
    const char version[] = "0.1.0";
    cli.header(
        "pargen v"s + version + " (" __DATE__
        + ") simplistic parser generator\n");
    // positional arguments
    auto & srcfile = cli.opt(&m_srcfile, "[file]")
                         .desc("File containing ABNF rules to process.");
    cli.opt(&m_root, "[root rule]")
        .desc("Root rule to use, overrides %root in <source file>.");
    // options
    cli.versionOpt(version);
    auto & help = cli.opt<bool>("? h").desc("Show this message and exit.");
    auto & test = cli.opt<bool>("test.").desc(
        "Run internal test of ABNF parsing logic.");
    cli.opt(&s_minRules, "min-core", s_minRules)
        .desc("Use reduced core rules: ALPHA, DIGIT, CRLF, HEXDIG, NEWLINE, "
              "VCHAR, and WSP are shortened to fewer (usually 1) characters.");
    cli.opt(&s_cmdopts.markFunction, "f mark-functions", 0)
        .valueDesc("LEVEL")
        .desc("Strength of function tag preprocessing.")
        .choice(0, "0", "No change to function tags (default).")
        .choice(1, "1", "Add function tags to break rule recursion.")
        .choice(2, "2", "Same as #1, but remove all existing tags first.");
    cli.opt(&s_cmdopts.includeCallbacks, "!C callbacks", true)
        .desc("Include callback events, otherwise generated parser reduced "
              "down to pass/fail syntax check.");
    cli.opt(&s_cmdopts.buildStateTree, "!B build", true)
        .desc("Build the state tree, otherwise only stub versions of parser "
              "are generated.");
    cli.opt(&s_cmdopts.dedupStateTree, "!D dedup", true)
        .desc("Purge duplicate entries from the state tree, duplicates occur "
              "when multiple paths through the rules end with the same series "
              "of transitions.");
    cli.opt(&s_cmdopts.writeStatePositions, "s state-detail.")
        .desc("Include details of the states as comments in the generated "
              "parser code - may be extremely verbose.");
    cli.opt(&s_cmdopts.writeFunctions, "write-functions", true)
        .desc("Generate recursion breaking dependent functions.\n"
              "NOTE: If disabled generated files may not be compilable.");
    // footer
    cli.footer(1 + R"(
For additional information, see:
https://github.com/gknowles/dimapp/tree/master/tools/pargen/README.md
)");
    if (!cli.parse(m_argc, m_argv)) {
        if (int code = cli.exitCode()) {
            assert(code == EX_USAGE);
            return usageError(cerr, cli.errMsg(), cli.errDetail());
        } else {
            return appSignalShutdown(code);
        }
    }
    if (*help || m_argc == 1) {
        cli.writeHelp(cout);
        return appSignalShutdown(EX_OK);
    }
    if (*test) {
        int code = internalTest() ? EX_OK : EX_SOFTWARE;
        return appSignalShutdown(code);
    }
    if (!srcfile) {
        return usageError(cerr, "No value given for <source file[.abnf]>");
    }

    // process abnf file
    if (!srcfile->has_extension())
        srcfile->replace_extension("abnf");
    fileReadBinary(this, m_source, *srcfile);
}

//===========================================================================
void Application::onLog(LogType type, const std::string & msg) {
    auto ptr = new LogTask(type, msg);
    taskPush(m_logQ, *ptr);
}

//===========================================================================
void Application::onFileEnd(int64_t offset, IFile * file) {
    if (!file)
        return appSignalShutdown(EX_USAGE);

    fileClose(file);
    TimePoint start = Clock::now();
    Grammar rules;
    getCoreRules(rules);
    if (!parseAbnf(rules, m_source)) {
        logParseError(
            "parsing failed",
            filePath(file).string(),
            rules.errpos(),
            m_source);
        return appSignalShutdown(EX_DATAERR);
    }

    if (!processOptions(rules))
        return appSignalShutdown(EX_DATAERR);

    if (m_root.size())
        rules.setOption(kOptionRoot, m_root);

    ofstream oh(rules.optionString(kOptionApiHeaderFile));
    ofstream ocpp(rules.optionString(kOptionApiCppFile));
    writeParser(oh, ocpp, rules, s_cmdopts);
    oh.close();
    ocpp.close();
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    logMsgDebug() << "Elapsed time: " << elapsed.count() << " seconds";

    if (s_errors) {
        logMsgError() << "Errors encountered: " << s_errors;
        return appSignalShutdown(EX_DATAERR);
    }
    logMsgDebug() << "Errors encountered: " << s_errors;
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
