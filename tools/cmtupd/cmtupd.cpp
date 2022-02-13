// Copyright Glen Knowles 2020 - 2022.
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

const VersionInfo kVersion = { 1, 1 };


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
struct MatchDef {
    Regex match;
    string replace;
    vector<Capture> captures;
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
};

struct Result {
    unsigned matched = 0;
    unsigned changed = 0;
    unsigned newer = 0;
    string content;
};

} // namespace


/****************************************************************************
*
*   CmdOpts
*
***/

struct CmdOpts {
    vector<string> files;
    string configFile;

    bool check;
    bool update;
    bool show;
    int verbose;

    CmdOpts();
};
static CmdOpts s_opts;


/****************************************************************************
*
*   Variables
*
***/

static auto & s_perfSelectedFiles = uperf("cmtupd files selected");
static auto & s_perfScannedFiles = uperf("cmtupd files scanned");
static auto & s_perfUnfinishedFiles = uperf("cmtupd files unfinished");
static auto & s_perfQueuedFiles = uperf("cmtupd files (queued)");
static auto & s_perfMatchedFiles = uperf("cmtupd files matched");
static auto & s_perfComments = uperf("cmtupd comments matched");
static auto & s_perfUpdatedComments = uperf("cmtupd comments updated");
static auto & s_perfUpdatedFiles = uperf("cmtupd files updated");
static auto & s_perfNewerFiles = uperf("cmtupd files failed (future)");

static mutex s_progressMut;
static string s_currentYear = []() {
    tm tm;
    timeToDesc(&tm, timeNow());
    return to_string(tm.tm_year + 1900);
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
            appSignalShutdown(EX_DATAERR);
            return false;
        }
        if (*name == '$') {
            logMsgError() << "Invalid Var @name '" << name << "', names "
                "starting with '$' are reserved for internally generated "
                "values.";
        }
        (*out)[name] = attrValue(&xvar, "value", "");
    }
    return true;
}

//===========================================================================
static bool loadMatches(Rule * out, XNode * root) {
    for (auto&& xmatch : elems(root, "Match")) {
        auto & match = out->matchDefs.emplace_back();
        match.match.pattern = attrValue(&xmatch, "regex");
        match.replace = attrValue(&xmatch, "replace", "");
        for (auto&& xcap : elems(&xmatch, "Capture")) {
            auto cvar = attrValue(&xcap, "var");
            auto cdef = attrValue(&xcap, "default", "");
            if (!cvar) {
                logMsgError() << "Invalid Rule/Match/Capture element, "
                    "must have @const or @var";
                appSignalShutdown(EX_DATAERR);
                return false;
            }
            match.captures.emplace_back(cvar, cdef);
        }
    }
    return true;
}

//===========================================================================
static bool loadFileNames(
    Group * out,
    XNode * root,
    const unordered_map<string, string> & vars
) {
    for (auto&& xfile : elems(root, "File")) {
        auto & file = out->fileNames.emplace_back();
        if (auto re = attrValue(&xfile, "regex")) {
            compileRegex(
                &file,
                "Found in Rule/Group/File element",
                attrValueSubst(re, vars)
            );
        } else {
            logMsgError() << "Invalid Rule/Group/File element, "
                "must have @regex";
            appSignalShutdown(EX_DATAERR);
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadFileExcludes(
    Group * out,
    XNode * root,
    const unordered_map<string, string> & vars
) {
    for (auto&& xexcl : elems(root, "Exclude")) {
        auto & excl = out->fileExcludes.emplace_back();
        if (auto re = attrValue(&xexcl, "regex")) {
            compileRegex(
                &excl,
                "Found in Rule/Group/Exclude element",
                attrValueSubst(re, vars)
            );
        } else {
            logMsgError() << "Invalid Rule/Group/Exclude element, "
                "must have @regex";
            appSignalShutdown(EX_DATAERR);
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
            || !loadFileNames(&grp, &xgroup, vars)
            || !loadFileExcludes(&grp, &xgroup, vars)
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
static bool loadRules(Config * out, XNode * root) {
    for (auto&& xrule : elems(root, "Rule")) {
        auto & rule = out->rules.emplace_back();
        rule.name = attrValue(&xrule, "name");
        unordered_map<string, string> vars;
        if (!loadVars(&vars, &xrule)
            || !loadMatches(&rule, &xrule)
            || !loadGroups(&rule, &xrule, vars)
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
    if (!loadRules(out.get(), root))
        return {};
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
    string_view fnameRaw
) {
    auto fname = string(fnameRaw);
    vector<const Group *> out;
    for (auto&& rule : cfg.rules) {
        if (auto grp = findGroup(rule, fname))
            out.push_back(grp);
    }
    return out;
}

//===========================================================================
static bool replaceFile(
    string_view path,
    const string & content
) {
    FileHandle file;
    auto ec = fileOpen(
        &file,
        path,
        File::fCreat | File::fTrunc | File::fReadWrite | File::fBlocking
    );
    if (ec) {
        logMsgError() << path << ": unable to open.";
        appSignalShutdown(EX_IOERR);
        return false;
    }
    if (auto ec = fileAppendWait(nullptr, file, content); ec) {
        logMsgError() << path << ": unable to write.";
        appSignalShutdown(EX_IOERR);
        fileClose(file);
        return false;
    }
    fileClose(file);
    return true;
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
            << "Group:\n";
        for (auto&& grp : rule.groups) {
            [[maybe_unused]] auto & prefix = grp.vars.at(kVarPrefix);
            for (auto&& mat : grp.matchDefs) {
                os << "  Replace\t" << mat.replace << '\n';
            }
            for (auto&& var : grp.vars) {
                os << "  Var\t" << var.first << '\t' << var.second << '\n';
            }
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
    if (s_perfQueuedFiles && --s_perfQueuedFiles)
        return;

    if (s_opts.verbose > 1
        || s_opts.verbose && s_perfScannedFiles
        || s_perfMatchedFiles != s_perfScannedFiles
        || s_perfNewerFiles
        || s_perfUpdatedFiles
    ) {
        cout << endl;
    }
    cout << "Files: " << s_perfSelectedFiles << " selected";
    if (auto excluded = s_perfSelectedFiles - s_perfScannedFiles)
        cout << ", " << excluded << " excluded";
    if (s_perfMatchedFiles != s_perfScannedFiles) {
        cout << ", ";
        ConsoleScopedAttr attr(kConsoleWarn);
        cout << s_perfScannedFiles - s_perfMatchedFiles << " unmatched";
    }
    if (s_perfNewerFiles) {
        cout << ", ";
        ConsoleScopedAttr attr(kConsoleWarn);
        cout << s_perfNewerFiles << " newer";
    }
    if (s_perfUpdatedFiles) {
        if (s_opts.update) {
            cout << ", ";
            ConsoleScopedAttr attr(kConsoleNote);
            cout << s_perfUpdatedFiles << " updated";
        } else {
            cout << ", ";
            ConsoleScopedAttr attr(kConsoleWarn);
            cout << s_perfUpdatedFiles << " obsolete";
        }
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
    auto commitYear = rawVars.at(kVarCommitYear);
    string out;
    cmatch m;
    size_t pos = 0;
    auto changes = 0;
    for (;; pos += m.position() + m.length()) {
        if (!regex_search(res->content.data() + pos, m, mat.match.re))
            break;
        out.append(res->content, pos, m.position());
        for (auto i = 0; auto&& cap : mat.captures) {
            i += 1;
            if (m[i].matched)
                vars[cap.name] = m[i].str();
        }
        for (auto i = 0; auto&& cap : mat.captures) {
            i += 1;
            if (!m[i].matched)
                vars[cap.name] = attrValueSubst(cap.defaultValue, vars);
        }
        s_perfComments += 1;
        res->matched += 1;
        auto & lastYear = vars[kVarLastYear];
        if (lastYear > commitYear) {
            res->newer += 1;
            out.append(m[0].str());
        } else if (lastYear == commitYear) {
            out.append(m[0].str());
        } else {
            s_perfUpdatedComments += 1;
            changes += 1;
            auto rstr = attrValueSubst(mat.replace, vars);
            out.append(rstr);
        }
    }
    if (changes) {
        res->changed += changes;
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
    string commitYear = StrFrom(tm.tm_year + 1900).c_str();

    Result res;
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
    if (!res.matched) {
        cout << fname << "... unmatched" << endl;
    } else {
        s_perfMatchedFiles += 1;
        if (!res.changed) {
            if (res.newer) {
                s_perfNewerFiles += 1;
                cout << fname << "... ";
                ConsoleScopedAttr attr(kConsoleWarn);
                cout << "NEWER" << endl;
            } else if (s_opts.verbose) {
                cout << fname << "... unchanged" << endl;
            }
        } else {
            s_perfUpdatedFiles += 1;
            if (!s_opts.update) {
                cout << fname << "... ";
                ConsoleScopedAttr attr(kConsoleWarn);
                cout << "obsolete";
            } else {
                if (!replaceFile(fullPath, res.content))
                    return;
                cout << fname << "... ";
                ConsoleScopedAttr attr(kConsoleNote);
                cout << "UPDATED";
            }
            if (res.newer) {
                s_perfNewerFiles += 1;
                cout << " and ";
                ConsoleScopedAttr attr(kConsoleWarn);
                cout << "NEWER";
            }
            cout << endl;
        }
    }

    s_perfScannedFiles += 1;
    s_perfUnfinishedFiles -= 1;
}

//===========================================================================
static void processFiles(const Config * cfg) {
    vector<string> args = { "git", "-C", cfg->gitRoot.str(), "ls-files" };
    for (auto&& file : s_opts.files) {
        Path tmp;
        fileAbsolutePath(&tmp, file);
        file = move(tmp.str());
    }
    args.insert(args.end(), s_opts.files.begin(), s_opts.files.end());
    auto rawNames = execToolWait(Cli::toCmdline(args), "List depot files");
    if (rawNames.empty())
        return;
    vector<string_view> fnames;
    split(&fnames, rawNames, '\n');

    vector<string> dateArgs = {
        "git", "-C", cfg->gitRoot.str(), "log", "--format=%aI", "-n1", ""
    };
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
            continue;
        }

        s_perfQueuedFiles += 1;
        dateArgs.back() = f;
        execTool(
            [cfg, grps, fname](string && out) {
                processFile(cfg, grps, fname, out);
                finalReport(cfg);
            },
            Cli::toCmdline(dateArgs),
            f,
            { .hq = taskComputeQueue() },
            { 0, 128, 259 } // allowed exit codes
        );
    }

    if (!s_perfSelectedFiles)
        finalReport(cfg);
}


/****************************************************************************
*
*   Command Line
*
***/

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
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
static void app(int argc, char *argv[]) {
    Cli cli;
    cli.helpNoArgs();
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    auto conf = loadConfig(s_opts.configFile);
    if (!conf)
        return;

    if (s_opts.show)
        showConfig(conf.get());
    if (s_opts.check || s_opts.update) {
        processFiles(conf.release());
        return;
    }
    if (!s_opts.show) {
        return appSignalUsageError(
            "No operation selected.",
            "Pick some combination of --show, --check, or --update."
        );
    }

    appSignalShutdown(EX_OK);
}

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv, kVersion);
}
