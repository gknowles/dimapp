// Copyright Glen Knowles 2016 - 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// pargen.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 2, 2, 1 };


/****************************************************************************
*
*   Declarations
*
***/

namespace {

class LogTask
    : public ITaskNotify
    , public ILogNotify
{
public:
    LogTask () {}
    LogTask (const LogMsg & log);

    // ITaskNotify
    void onTask () override;
    // ILogNotify
    void onLog (const LogMsg & log) override;

private:
    string m_msg;
    LogMsg m_log;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static RunOptions s_cmdopts;
static TaskQueueHandle s_logQ;


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool parseAbnf (
    Grammar & rules,
    const string & src,
    bool minRules
) {
    AbnfParser parser(rules, minRules);
    const char * ptr = src.c_str();
    if (!parser.parse(ptr)) {
        rules.errpos(parser.errpos());
        return false;
    }
    normalize(rules);
    return true;
}

//===========================================================================
static void getCoreRules (Grammar & rules) {
    const char * coreRules = R"(
ALPHA   =  %x41-5A / %x61-7A ; A-Z / a-z
BIT     =  "0" / "1"
CHAR    =  %x01-7F
CR      =  %x0D
CRLF    =  CR LF
CTL     =  %x00-1F / %x7F
DIGIT   =  %x30-39 ; 0-9
DQUOTE  =  %x22
HEXDIG  =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
HTAB    =  %x09
LF      =  %x0A
LWSP    =  *(WSP / NEWLINE WSP)
NEWLINE =  LF
NEWLINE =/ CRLF { NoMinRules }
OCTET   =  %x00-FF
SP      =  %x20
VCHAR   =  %x21-7E
WSP     =  SP
WSP     =/ HTAB { NoMinRules }
)";
    [[maybe_unused]] bool valid = parseAbnf(
        rules,
        coreRules,
        s_cmdopts.minRules
    );
    assert(valid);
}

//===========================================================================
static bool internalTest () {
    bool valid;
    Grammar rules;
    getCoreRules(rules);
    rules.addOption("%root", "LWSP");
    rules.addOption("%api.prefix", "Test");
    valid = processOptions(rules);
    if (s_cmdopts.verbose)
        logMsgInfo() << "Process options: " << valid;

    ostringstream abnf;
    for (auto && rule : rules.rules()) {
        writeRule(abnf, rule, 79, "");
    }
    rules.clear();
    valid = parseAbnf(rules, abnf.str(), s_cmdopts.minRules);
    if (s_cmdopts.verbose)
        logMsgInfo() << "Valid: " << valid;
    if (!valid)
        return false;

    ostringstream o1;
    for (auto && rule : rules.rules()) {
        writeRule(o1, rule, 79, "");
    }

    ostringstream o2;
    rules.clear();
    valid = parseAbnf(rules, o1.str(), s_cmdopts.minRules);
    for (auto && rule : rules.rules()) {
        writeRule(o2, rule, 79, "");
    }
    valid = valid && (o1.str() == o2.str());
    if (s_cmdopts.verbose)
        logMsgInfo() << "Round trip: " << valid;
    if (!valid)
        return false;

    logMsgInfo() << "All tests passed (" << appBaseName() << ")";
    return true;
}


/****************************************************************************
*
*   LogTask
*
***/

//===========================================================================
LogTask::LogTask (const LogMsg & log)
    : m_msg(log.msg)
    , m_log({log.type, log.loc, m_msg})
{}

//===========================================================================
void LogTask::onLog (const LogMsg & log) {
    if (s_cmdopts.verbose || log.type != kLogTypeDebug) {
        auto ptr = NEW(LogTask)(log);
        if (s_logQ) {
            taskPush(s_logQ, ptr);
        } else {
            ptr->onTask();
        }
    }
}

//===========================================================================
void LogTask::onTask () {
    if (m_log.type == kLogTypeError) {
        ConsoleScopedAttr attr(kConsoleError);
        cerr << "ERROR: " << m_log.msg << endl;
    } else {
        cout << m_log.msg << endl;
    }
    delete this;
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app (int argc, char * argv[]) {
    s_logQ = taskCreateQueue("Logging", 1);

    Cli cli;
    // header
    cli.header(cli.header() + " simplistic parser generator");
    // positional arguments
    auto & srcfile = cli.opt<Path>("[source file(.abnf)]")
        .desc("File containing ABNF rules to process.");
    auto & root = cli.opt<string>("[root rule]")
        .desc("Root rule to use, overrides %root in <source file>.");
    // options
    cli.helpNoArgs();
    auto & test = cli.opt<bool>("test.").group("~").desc(
        "Run internal test of ABNF parsing logic.");
    cli.opt(&s_cmdopts.minRules, "min-rules", false)
        .desc("Use reduced core rules: ALPHA, DIGIT, CRLF, HEXDIG, NEWLINE, "
              "VCHAR, and WSP are shortened to fewer (usually 1) characters. "
              "And ignores user rules tagged with NoMinRules.");
    cli.opt(&s_cmdopts.mergeRules, "m merge-rules", false)
        .desc("Allow merging of rules that aren't required to be separate.");
    cli.opt("f mark-functions", 1)
        .valueDesc("LEVEL")
        .desc("Strength of function tag preprocessing.")
        .choice(0, "0", "Remove all function tags.")
        .choice(1, "1", "No change to function tags (default).")
        .choice(2, "2", "Add function tags to break rule recursion.")
        .choice(3, "3", "Same as #2, but remove all existing tags first.")
        .check([](auto & cli, auto & opt, const string & val) {
            s_cmdopts.resetFunctions = (*opt == 0 || *opt == 3);
            s_cmdopts.markFunctions = (*opt == 2 || *opt == 3);
            return true;
        });
    cli.opt(&s_cmdopts.includeCallbacks, "!C callbacks", true)
        .desc("Include callback events, otherwise generated parser reduced "
              "down to pass/fail syntax check.");
    cli.opt(&s_cmdopts.buildStateTree, "!B build", true)
        .desc("Build the state tree, otherwise only stub versions of parser "
              "are generated.");
    cli.opt(&s_cmdopts.stateTreeDepthLimit, "l depth-limit", 0)
        .desc("Limit state tree depth, skip anything that would go deeper. "
              "0 for unlimited.");
    cli.opt(&s_cmdopts.dedupStateTree, "!D dedup", true)
        .desc("Purge duplicate entries from the state tree, duplicates occur "
              "when multiple paths through the rules end with the same series "
              "of transitions.");
    cli.opt(&s_cmdopts.writeStatePositions, "s state-detail.")
        .desc("Include details of the states as comments in the generated "
              "parser code - may be extremely verbose.");
    cli.opt(&s_cmdopts.writeFunctions, "write-functions", true)
        .desc("Generate recursion breaking dependent functions.\n"
              "  \t\v\v\v\v\v\vNOTE: Disable for testing only. If disabled "
              "the generated files may not be compilable.");
    cli.opt(&s_cmdopts.verbose, "v verbose")
        .desc("Display details of what's happening during processing.");
    // footer
    cli.footer(1 + R"(
For additional information, see:
https://github.com/gknowles/dimapp/tree/master/tools/pargen/README.md
)");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();
    if (*test) {
        int code = internalTest() ? EX_OK : EX_SOFTWARE;
        return appSignalShutdown(code);
    }
    if (!srcfile)
        return appSignalUsageError("No value given for <source file[.abnf]>");

    // initialize rules
    TimePoint start = Clock::now();
    Grammar rules;
    getCoreRules(rules);

    // process ABNF sources
    deque<string> sources;
    sources.push_back(srcfile->defaultExt("abnf").c_str());
    map<string, string> contents;
    while (!sources.empty()) {
        auto path = sources.front();
        sources.pop_front();
        if (contents.count(path))
            continue;

        string source;
        fileLoadBinaryWait(&source, path);
        if (source.empty())
            return appSignalUsageError(EX_USAGE);
        auto & content = contents[path] = move(source);
        if (!parseAbnf(rules, content, s_cmdopts.minRules)) {
            logParseError(
                "parsing failed",
                path,
                rules.errpos(),
                content
            );
            return appSignalShutdown(EX_DATAERR);
        }
        path = Path(path).removeFilename().c_str();
        for (auto && f : rules.optionStrings(kOptionInclude))
            sources.push_back(Path(f).resolve(path).c_str());
        rules.eraseOption(kOptionInclude);
    }

    if (!processOptions(rules))
        return appSignalShutdown(EX_DATAERR);

    if (root->size())
        rules.setOption(kOptionRoot, *root);

    auto ohName = rules.optionString(kOptionApiOutputHeader);
    ofstream oh(ohName);
    if (!oh) {
        logMsgError() << ohName << ": open failed";
        return appSignalShutdown(EX_IOERR);
    }
    auto ocppName = rules.optionString(kOptionApiOutputCpp);
    ofstream ocpp(ocppName);
    if (!ocpp) {
        logMsgError() << ocppName << ": open failed";
        return appSignalShutdown(EX_IOERR);
    }
    writeParser(oh, ocpp, rules, s_cmdopts);
    oh.close();
    ocpp.close();
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    logMsgInfo() << "Elapsed time: " << elapsed.count() << " seconds";

    if (int errs = logGetMsgCount(kLogTypeError)) {
        logMsgError() << "Errors encountered: " << errs;
        return appSignalShutdown(EX_DATAERR);
    }
    logMsgInfo() << "Errors encountered: " << 0;
    s_logQ = {};
    appSignalShutdown(EX_OK);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main (int argc, char * argv[]) {
    consoleCatchCtrlC(false);
    LogTask logger;
    logMonitor(&logger);
    return appRun(app, argc, argv, kVersion, "pargen");
}
