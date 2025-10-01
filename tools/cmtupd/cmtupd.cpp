// Copyright Glen Knowles 2020 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// cmtupd.cpp - cmtupd
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 2, 0, 1 };


/****************************************************************************
*
*   Declarations
*
***/

const char kVarPrefix[] = "Prefix";
const char kVarCommitYear[] = "$CommitYear";
const char kVarCurrentYear[] = "$CurrentYear";
const char kVarLastYear[] = "LastYear";

namespace {

struct Regex {
    regex re;
    string pattern;
};
struct Capture {
    string name;
    string defaultValue;
};
struct Condition {
    string opName;
    function<bool(const string&, const string&)> op;
    string arg1;
    string arg2;
};
struct Action {
    enum Type { kInvalid, kSkip, kUpdate } type = {};
    Condition cond;
    string report;
    ConsoleAttr reportType = {};
    unsigned reportPriority = {};

    strong_ordering operator<=>(const Action & other) const {
        // Lowest type first.
        auto rc = reportType <=> other.reportType;
        if (rc == 0) {
            // Best (lowest) priority first.
            rc = reportPriority <=> other.reportPriority;
        }
        return rc;
    }
};
const TokenTable::Token s_actionTypes[] = {
    { Action::kSkip, "skip" },
    { Action::kUpdate, "update" },
};
const TokenTable s_actionTypeTbl(s_actionTypes);

struct MatchDef {
    Regex match;
    string replace;
    vector<Capture> captures;
    vector<Action> actions;
};
struct Group {
    unordered_map<string, string> vars;
    vector<Regex> fileNames;
    vector<Regex> fileExcludes;
    vector<MatchDef> matchDefs;
};
struct Rule {
    string name;
    vector<MatchDef> matchDefs;
    vector<Group> groups;
};
struct Config {
    Path configFile;
    Path gitRoot;
    vector<Rule> rules;
    unsigned numActions = {};   // used to assign default action priorities
};

struct Count {
    const Action * act;
    unsigned count = {};
    strong_ordering operator<=>(const Count & other) const {
        return *act <=> *other.act;
    }
};
struct Result {
    unsigned matched = {};
    unsigned unchanged = {};
    vector<Count> cnts;
    unordered_map<string, size_t> byReport;
    string content;
};

} // namespace


/****************************************************************************
*
*   CmdOpts
*
***/

namespace {
struct CmdOpts {
    vector<string> files;
    string configFile;

    bool check;
    bool update;
    bool show;
    int verbose;

    CmdOpts();
};
} // namespace
static CmdOpts s_opts;


/****************************************************************************
*
*   Variables
*
***/

static auto & s_perfSelectedFiles = uperf("cmtupd files selected");
static auto & s_perfSkippedFiles = uperf("cmtupd files skipped (unselected)");
static auto & s_perfScannedFiles = uperf("cmtupd files scanned");
static auto & s_perfUnfinishedFiles = uperf("cmtupd files unfinished");
static auto & s_perfQueuedFiles = uperf("cmtupd files (queued)");
static auto & s_perfMatchedFiles = uperf("cmtupd files matched");
static auto & s_perfMatchByRule = uperf("cmtupd rule matches");
static auto & s_perfUpdateByRule = uperf("cmtupd rule triggered updates");
static auto & s_perfUnchangedFiles = uperf("cmtupd files unchanged");
static auto & s_perfUpdatedFiles = uperf("cmtupd files updated");

static mutex s_progressMut;
static Result s_result;
const string s_currentYear = []() {
    tm tm;
    timeToDesc(&tm, timeNow());
    return toString(tm.tm_year + 1900);
}();


/****************************************************************************
*
*   Helpers
*
***/

thread_local string s_compilePattern;
thread_local string s_compileDetailMsg;

//===========================================================================
static void compileRegex(
    Regex * regex,
    string_view dmsg,
    const string & pattern,
    regex::flag_type flags = regex_constants::ECMAScript
) {
    s_compilePattern = pattern;
    s_compileDetailMsg = dmsg;
    regex->pattern = pattern;
    auto prev = set_terminate([]() -> void {
        logMsgError() << "Invalid regular expression '"
            << s_compilePattern << "'";
        logMsgInfo() << s_compileDetailMsg;
        _CrtSetDbgFlag(0);
        _exit(EX_DATAERR);
    });
    regex->re.assign(pattern, flags);
    set_terminate(prev);
}


/****************************************************************************
*
*   Load configuration
*
***/

//===========================================================================
static bool loadVars(unordered_map<string, string> * out, XNode * root) {
    for (auto&& xvar : elems(root, "Var")) {
        auto name = attrValue(&xvar, "name");
        if (!name) {
            logMsgError() << "Invalid Var element, must have @name";
            return false;
        }
        if (*name == '$') {
            logMsgError() << "Invalid Var @name '" << name << "', names "
                "starting with '$' are reserved for internally generated "
                "values.";
            continue;
        }
        (*out)[name] = attrValue(&xvar, "value", "");
    }
    return true;
}

//===========================================================================
static bool loadCaptures(MatchDef * out, XNode * root) {
    for (auto&& xe : elems(root, "Capture")) {
        auto cvar = attrValue(&xe, "var");
        auto cdef = attrValue(&xe, "default", "");
        if (!cvar) {
            logMsgError() << "Invalid Rule/Match/Capture element, "
                "must have @const or @var";
            return false;
        }
        out->captures.emplace_back(cvar, cdef);
    }
    return true;
}

//===========================================================================
static bool loadCond(Condition * out, XNode * root) {
    auto opName = attrValue(root, "op");
    if (opName) {
        out->opName = opName;
        if (out->opName == "=") out->op = equal_to<string>();
        else if (out->opName == "!=") out->op = not_equal_to<string>();
        else if (out->opName == "<") out->op = less<string>();
        else if (out->opName == "<=") out->op = less_equal<string>();
        else if (out->opName == ">") out->op = greater<string>();
        else if (out->opName == ">=") out->op = greater_equal<string>();
        else {
            logMsgError() << "Invalid Rule/Match/Action/@op, "
                "allowed values: =, !=, <, <=, >, or >=";
            return false;
        }
    }
    out->arg1 = attrValue(root, "arg1", "");
    out->arg2 = attrValue(root, "arg2", "");
    return true;
}

//===========================================================================
static bool loadActions(
    MatchDef * out,
    XNode * root,
    unsigned * numActions
) {
    for (auto&& xe : elems(root, "Action")) {
        auto & act = out->actions.emplace_back();
        act.type = s_actionTypeTbl.find(
            attrValue(&xe, "type"),
            Action::kInvalid
        );
        if (!act.type) {
            logMsgError() << "Invalid Rule/Match/Action/@type, must be: "
                "skip, or update";
            return false;
        }
        auto rep = attrValue(&xe, "report", "???");
        auto rtype = attrValue(&xe, "reportType", "normal");
        if (act.type == Action::kUpdate && s_opts.check) {
            rep = attrValue(&xe, "checkReport", rep);
            rtype = attrValue(&xe, "checkReportType", rtype);
        }
        act.report = rep;
        if (!parse(&act.reportType, rtype)) {
            logMsgError() << "Invalid Rule/Match/Action/@reportType, must be: "
                "normal, cheer, note, warn, or error";
            return false;
        }
        if (act.type == Action::kUpdate && act.reportType <= kConsoleNormal) {
            logMsgError() << "Update actions must have @reportType greater "
                "than 'normal'";
            return false;
        }
        act.reportPriority = ++*numActions;
        if (!loadCond(&act.cond, &xe))
            return false;
    }
    return true;
}

//===========================================================================
static bool loadMatches(Rule * out, XNode * root, unsigned * numActions) {
    for (auto&& xmatch : elems(root, "Match")) {
        auto & match = out->matchDefs.emplace_back();
        match.match.pattern = attrValue(&xmatch, "regex");
        match.replace = attrValue(&xmatch, "replace", "");
        if (!loadCaptures(&match, &xmatch)
            || !loadActions(&match, &xmatch, numActions)
        ) {
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadRegexes(
    vector<Regex> * out,
    XNode * root,
    const char elemName[],
    const unordered_map<string, string> & vars
) {
    auto elemPath = string("Rule/Group/") + elemName + " element";
    for (auto&& xfile : elems(root, elemName)) {
        auto & rx = out->emplace_back();
        if (auto val = attrValue(&xfile, "regex")) {
            compileRegex(
                &rx,
                "Found in " + elemPath,
                attrValueSubst(val, vars)
            );
        } else {
            logMsgError() << "Invalid " << elemPath << ", "
                "must have @regex";
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadGroups(
    Rule * out,
    XNode * root,
    const unordered_map<string, string> & vars
) {
    for (auto&& xgroup : elems(root, "Group")) {
        auto & grp = out->groups.emplace_back();
        grp.vars = vars;
        grp.vars[kVarPrefix]; // make sure kVarPrefix exists
        grp.vars[kVarCurrentYear] = s_currentYear;
        if (!loadVars(&grp.vars, &xgroup)
            || !loadRegexes(&grp.fileNames, &xgroup, "File", vars)
            || !loadRegexes(&grp.fileExcludes, &xgroup, "Exclude", vars)
        ) {
            return false;
        }
        if (grp.fileNames.empty()) {
            out->groups.pop_back();
            continue;
        }
        grp.matchDefs = out->matchDefs;
        for (auto&& mat : grp.matchDefs) {
            compileRegex(
                &mat.match,
                "Found in Rule/Match/@regex attribute",
                attrValueSubst(mat.match.pattern, grp.vars)
            );
        }
    }
    return true;
}

//===========================================================================
static bool loadRules(
    Config * out,
    XNode * root,
    const unordered_map<string, string> & vars
) {
    for (auto&& xrule : elems(root, "Rule")) {
        auto & rule = out->rules.emplace_back();
        rule.name = attrValue(&xrule, "name");
        auto rvars = vars;
        if (!loadVars(&rvars, &xrule)
            || !loadMatches(&rule, &xrule, &out->numActions)
            || !loadGroups(&rule, &xrule, rvars)
        ) {
            return false;
        }
    }
    return true;
}

//===========================================================================
unique_ptr<Config> loadConfig(string_view cfgfile) {
    string configFile;
    string gitRoot;
    string content;
    if (!gitLoadConfig(
        &content,
        &configFile,
        &gitRoot,
        cfgfile,
        "conf/cmtupd.xml"
    )) {
        return {};
    }

    XDocument doc;
    auto root = doc.parse(content.data(), configFile);
    if (!root || doc.errmsg()) {
        logParseError("Parsing failed", configFile, doc.errpos(), content);
        appSignalShutdown(EX_DATAERR);
        return {};
    }
    auto out = make_unique<Config>();
    if (auto ec = fileAbsolutePath(&out->configFile, configFile); ec)
        return {};
    out->gitRoot = gitRoot;
    unordered_map<string, string> vars;
    if (!loadVars(&vars, root)
        || !loadRules(out.get(), root, vars)
    ) {
        appSignalShutdown(EX_DATAERR);
        return {};
    }
    return out;
}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static const Group * findGroup(const Rule & rule, const string & fname) {
    for (auto&& grp : rule.groups) {
        bool found = false;
        for (auto&& fn : grp.fileNames) {
            if (regex_match(fname, fn.re)) {
                found = true;
                break;
            }
        }
        if (found) {
            for (auto&& ex : grp.fileExcludes) {
                if (regex_match(fname, ex.re)) {
                    found = false;
                    break;
                }
            }
            if (found)
                return &grp;
        }
    }
    return nullptr;
}

//===========================================================================
static vector<const Group *> findGroups(
    const Config & cfg,
    const Path & fnameRaw
) {
    auto fname = fnameRaw.str();
    vector<const Group *> out;
    for (auto&& rule : cfg.rules) {
        if (auto grp = findGroup(rule, fname))
            out.push_back(grp);
    }
    return out;
}

//===========================================================================
static const Action * findAction(
    const MatchDef & def,
    const unordered_map<string, string> & vars
) {
    for (auto&& val : def.actions) {
        if (!val.cond.op)
            return &val;
        auto arg1 = attrValueSubst(val.cond.arg1, vars);
        auto arg2 = attrValueSubst(val.cond.arg2, vars);
        if (val.cond.op(arg1, arg2))
            return &val;
    }
    return nullptr;
}

//===========================================================================
static bool replaceFile(
    string_view path,
    const string & content
) {
    using enum File::OpenMode;

    FileHandle file;
    auto ec = fileOpen(&file, path, fCreat | fTrunc | fReadWrite | fBlocking);
    if (ec) {
        logMsgError() << path << ": unable to open, " << ec.value();
        appSignalShutdown(EX_IOERR);
        return false;
    }
    if (auto ec = fileAppendWait(nullptr, file, content); ec) {
        logMsgError() << path << ": unable to write, " << ec.value();
        appSignalShutdown(EX_IOERR);
        fileClose(file);
        return false;
    }
    fileClose(file);
    return true;
}

//===========================================================================
static void updateResult(Result * out, const Action * act, int count = 1) {
    auto ib = out->byReport.emplace(pair{act->report, out->cnts.size()});
    Count * cnt = {};
    if (ib.second) {
        cnt = &out->cnts.emplace_back();
        cnt->act = act;
    } else {
        cnt = &out->cnts[ib.second];
        if (*act > *cnt->act)
            cnt->act = act;
    }
    cnt->count += count;
}

//===========================================================================
static void updateResult(Result * out, const Result & res) {
    out->matched += (res.matched > 0);
    out->unchanged += (res.unchanged > 0);
    for (auto&& cnt : res.cnts) {
        assert(cnt.count);
        updateResult(out, cnt.act);
    }
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void showConfig(const Config * cfg) {
    ostringstream os;
    os << "Configuration file: " << cfg->configFile << '\n'
        << "Git root: " << cfg->gitRoot << '\n'
        << '\n';
    for (auto&& rule : cfg->rules) {
        os << "Rule: " << rule.name << '\n'
            << "Matches:\n";
        for (auto&& mat : rule.matchDefs) {
            os << '\f';
            os << "  Regex\t" << mat.match.pattern << '\n';
            os << "  Replace\t" << mat.replace << '\n';
            os << '\f';
            for (auto&& cap : mat.captures) {
                os << "  Capture\t" << cap.name << '\t' << cap.defaultValue
                    << '\n';
            }
            os << "  Actions:\n";
            os << '\f';
            for (auto&& act : mat.actions) {
                os << "    " << (act.type == Action::kSkip ? "Skip" : "Update")
                    << '\t' << act.report
                    << "\t\a5 15\a" << toString(act.reportType);
                if (!act.cond.opName.empty()) {
                    os << '\t' << act.cond.arg1
                        << ' ' << act.cond.opName << ' '
                        << act.cond.arg2;
                }
                os << '\n';
            }
        }
        os << "Groups:\n";
        for (auto&& grp : rule.groups) {
            [[maybe_unused]] auto & prefix = grp.vars.at(kVarPrefix);
            os << '\f';
            for (auto&& mat : grp.matchDefs) {
                os << "  Replace\t" << mat.replace << '\n';
            }
            os << '\f';
            for (auto&& var : grp.vars) {
                os << "  Var\t\a15 30\a"
                    << var.first << '\t' << var.second << '\n';
            }
            os << '\f';
            for (auto&& fn : grp.fileNames) {
                os << "  File\t" << fn.pattern << '\n';
            }
            for (auto&& fn : grp.fileExcludes) {
                os << "  Exclude\t" << fn.pattern << '\n';
            }
            os << '\n';
        }
        if (rule.groups.empty())
            os << '\n';
    }
    Cli cli;
    cli.printText(cout, os.str());
}

//===========================================================================
static void finalReport(const Config * cfg) {
    assert(!s_perfQueuedFiles);

    if (s_opts.verbose > 1
        || s_opts.verbose && s_perfScannedFiles
        || s_perfMatchedFiles != s_perfScannedFiles
        || s_perfUpdatedFiles
    ) {
        cout << endl;
    }
    cout << "Files: " << s_perfSelectedFiles << " selected";
    if (auto excluded = s_perfSelectedFiles - s_perfScannedFiles)
        cout << ", " << excluded << " excluded";
    if (auto unchanged = s_perfUnchangedFiles.load())
        cout << ", " << unchanged << " unchanged";

    Action act = { Action::kSkip, {}, "unmatched", kConsoleWarn, 0 };
    if (auto unmatched = s_perfScannedFiles - s_perfMatchedFiles)
        updateResult(&s_result, &act, unmatched);

    s_result.byReport.clear();
    sort(s_result.cnts.begin(), s_result.cnts.end());
    for (auto&& cnt : s_result.cnts) {
        cout << ", ";
        ConsoleScopedAttr attr(cnt.act->reportType);
        cout << cnt.count << ' ' << cnt.act->report;
    }
    cout << endl;

    if (s_perfUpdatedFiles && !s_opts.update)
        cout << "Use '-u' to update." << endl;

    delete cfg;
    logStopwatch();
    appSignalShutdown(EX_OK);
}

//===========================================================================
static bool updateContent(
    Result * res,
    const MatchDef & mat,
    const unordered_map<string, string> & rawVars,
    string_view fname
) {
    auto vars = rawVars;
    string out;
    cmatch m;
    size_t pos = 0;
    auto changes = 0;
    for (;; pos += m.position() + m.length()) {
        if (!regex_search(res->content.data() + pos, m, mat.match.re))
            break;
        s_perfMatchByRule += 1;
        res->matched += 1;
        out.append(res->content, pos, m.position());
        // Save values from matched captures.
        for (auto i = 0; auto&& cap : mat.captures) {
            i += 1;
            if (m[i].matched)
                vars[cap.name] = m[i].str();
        }
        // Save default values for captures that didn't match. This is done
        // after the matched are saved so substitutions in the defaults can
        // reference them.
        for (auto i = 0; auto&& cap : mat.captures) {
            i += 1;
            if (!m[i].matched)
                vars[cap.name] = attrValueSubst(cap.defaultValue, vars);
        }

        auto act = findAction(mat, vars);
        if (!act) {
            res->unchanged += 1;
            continue;
        }
        updateResult(res, act);

        if (act->type == Action::kSkip) {
            out.append(m[0].str());
        } else {
            assert(act->type == Action::kUpdate);
            s_perfUpdateByRule += 1;
            changes += 1;
            auto rstr = attrValueSubst(mat.replace, vars);
            out.append(rstr);
        }
    }
    if (changes) {
        out.append(res->content, pos);
        res->content = move(out);
        return true;
    }
    return false;
}

//===========================================================================
static void processFile(
    const Config * cfg,
    const vector<const Group *> & grps,
    Path fname,
    string_view commitTimeStr
) {
    Result res;
    s_perfUnfinishedFiles += 1;
    if (appStopping())
        return;

    TimePoint commitTime;
    tm tm;
    if (!timeParse8601(&commitTime, trim(commitTimeStr))
        || !timeToDesc(&tm, commitTime)
    ) {
        logMsgError() << "No commit time for '" << fname << "'";
        return appSignalShutdown(EX_OSERR);
    }
    string commitYear = toString(tm.tm_year + 1900);

    auto fullPath = fname;
    fullPath.resolve(cfg->gitRoot);
    if (fileLoadBinaryWait(&res.content, fullPath))
        return appSignalShutdown(EX_IOERR);

    for (auto&& grp : grps) {
        auto vars = grp->vars;
        vars[kVarCommitYear] = commitYear;
        for (auto&& mat : grp->matchDefs)
            updateContent(&res, mat, vars, fname);
    }

    scoped_lock lk(s_progressMut);
    bool updated = false;
    if (!res.matched) {
        cout << fname << "... unmatched" << endl;
    } else {
        s_perfMatchedFiles += 1;
        res.byReport.clear();
        sort(res.cnts.begin(), res.cnts.end());
        if (s_opts.verbose
            || !res.cnts.empty()
                && res.cnts.back().act->reportType > kConsoleNormal
        ) {
            cout << fname << "... ";
            auto first = true;
            if (res.unchanged) {
                first = false;
                cout << "unchanged";
            }
            for (auto&& cnt : res.cnts) {
                if (first) {
                    first = false;
                } else {
                    cout << ", ";
                }
                if (cnt.act->reportType == kConsoleNormal) {
                    cout << cnt.act->report;
                } else {
                    assert(cnt.act->reportType != kConsoleInvalid);
                    ConsoleScopedAttr attr(cnt.act->reportType);
                    cout << cnt.act->report;
                }
                if (!updated && cnt.act->type == Action::kUpdate) {
                    updated = true;
                    s_perfUpdatedFiles += 1;
                }
            }
            cout << endl;
        }
        if (!updated) {
            if (res.unchanged)
                s_perfUnchangedFiles += 1;
        } else {
            if (s_opts.update)
                replaceFile(fullPath, res.content);
        }
    }
    updateResult(&s_result, res);

    s_perfScannedFiles += 1;
    s_perfUnfinishedFiles -= 1;
}

//===========================================================================
static void processFiles(const Config * cfg) {
    vector<string> args = { "git", "-C", cfg->gitRoot.str(), "ls-files" };
    for (auto&& file : s_opts.files) {
        Path tmp;
        fileAbsolutePath(&tmp, file);
        file = move(tmp).str();
    }
    args.insert(args.end(), s_opts.files.begin(), s_opts.files.end());
    auto res = execToolWait(Cli::toCmdline(args), "List depot files");
    vector<string_view> fnames;
    split(&fnames, res.output, '\n');

    vector<string> dateArgs = {
        "git", "-C", cfg->gitRoot.str(), "log", "--format=%aI", "-n1", ""
    };
    s_perfQueuedFiles += 1;
    for (auto&& f : fnames) {
        f = trim(f);
        if (f.empty())
            continue;
        s_perfSelectedFiles += 1;
        auto fname = Path(f);
        auto grps = findGroups(*cfg, fname);
        if (grps.empty()) {
            if (s_opts.verbose > 1) {
                scoped_lock lk(s_progressMut);
                cout << f << "... skipped" << endl;
            }
            s_perfSkippedFiles += 1;
            continue;
        }

        s_perfQueuedFiles += 1;
        dateArgs.back() = f;
        execTool(
            [cfg, grps, fname](auto && res) {
                processFile(cfg, grps, fname, res.output);
                if (!--s_perfQueuedFiles)
                    finalReport(cfg);
            },
            Cli::toCmdline(dateArgs),
            f,
            { .hq = taskComputeQueue() },
            { 0, 128, 259 } // allowed exit codes
        );
    }

    // Decrement queued file count and report if none remaining.
    if (!--s_perfQueuedFiles)
        finalReport(cfg);
}


/****************************************************************************
*
*   Command Line
*
***/

//===========================================================================
static void app(Cli & cli) {
    auto conf = loadConfig(s_opts.configFile);
    if (!conf)
        return cli.fail(EX_DATAERR);

    if (s_opts.show)
        showConfig(conf.get());
    if (s_opts.check || s_opts.update) {
        processFiles(conf.release());
        return cli.fail(EX_PENDING);
    }
    if (!s_opts.show) {
        return cli.badUsage(
            "No operation selected.",
            "",
            "Pick some combination of --show, --check, and --update."
        );
    }
}

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
    cli.helpNoArgs().action(app);
    cli.optVec<string>(&files, "[FILE]")
        .desc("Files to search (via git ls-files) for updatable comments.");
    cli.opt<string>(&configFile, "c conf")
        .desc("Configuration to process. "
            "(default: {GIT_ROOT}/conf/cmtupd.xml)");
    cli.opt<bool>(&check, "c check").desc("Check for updatable comments.");
    cli.opt<bool>(&update, "u update").desc("Update found comments.");
    cli.opt<bool>(&show, "s show").desc("Show configuration being executed.");
    cli.opt<bool>("v verbose.")
        .desc("Show status of all matched files. Use twice to also show "
            "excluded files.")
        .check([this](auto&, auto&, auto & val) {
            verbose += (val == "1");
            return true;
        });
}

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(argc, argv, kVersion);
}
