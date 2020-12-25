// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// cmtupd.cpp - cmtupd
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

const char kVersion[] = "1.0";

struct Rule {
    regex pattern;
    string prefix;
    vector<string> formats;
};
static Rule s_rules[] = {
    { regex("CMakeLists.txt"), "# " },
    { regex(".*\\.yaml"), "# " },
    { regex(".*\\.yml"), "# " },

    { regex(".*\\.adoc"), "" },
    { regex(".*\\.md"), "" },
    { regex(".*\\.natvis"), "" },     // xml
    { regex(".*\\.xml"), "" },
    { regex(".*\\.xml.sample"), "" },
    { regex(".*\\.xsd"), "" },

    { regex(".*\\.cpp"), "// " },
    { regex(".*\\.h"), "// " },

    { regex(".*\\.abnf"), "; " },

    { regex(".*\\.bat"), ":: " },
};

struct CopyrightYear {
    string_view lastYear;
    int format = 0;
};


/****************************************************************************
*
*   Variables
*
***/

static bool s_update;
static int s_verbose;

static int s_files;             // files scanned for matching comments
static int s_matchedFiles;      // scanned files with matching comments
static int s_comments;          // total matching comments
static int s_updatedComments;   // comments that were (or would be) updated
static int s_updatedFiles;      // files with updated comments


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static Rule * findRule(const string & fname) {
    for (auto&& rule : s_rules) {
        if (regex_match(fname, rule.pattern)) {
            rule.formats = {
                rule.prefix + "Copyright Glen Knowles %n%u%n.%n",
                rule.prefix + "Copyright Glen Knowles %*u - %n%u%n.%n",
            };
            return &rule;
        }
    }
    return nullptr;
}

//===========================================================================
static bool findYear(
    CopyrightYear * out,
    const string & line,
    const Rule & rule
) {
    int spos = 0, epos = 0;
    int len = 0;
    unsigned year = 0;
    out->format = 0;
    for (auto&& fmt : rule.formats) {
        sscanf_s(line.c_str(), fmt.c_str(), &spos, &year, &epos, &len);
        if (len == line.size())
            break;
        out->format += 1;
    }
    if (len != line.size() || year == 0) {
        *out = {};
        return false;
    }
    out->lastYear = string_view(line.data() + spos, epos - spos);
    return true;
}

//===========================================================================
static bool replaceFile(
    string_view path,
    const vector<string_view> & content
) {
    CharBuf buf;
    for (auto&& line : content) {
        buf.append(line);
        buf.append("\r\n");
    }
    auto f = fileOpen(
        path,
        File::fCreat | File::fTrunc | File::fReadWrite | File::fBlocking
    );
    if (!f) {
        logMsgError() << path << ": unable to open.";
        appSignalShutdown(EX_IOERR);
        return false;
    }
    for (auto&& data : buf.views()) {
        if (!fileAppendWait(f, data.data(), data.size())) {
            logMsgError() << path << ": unable to write.";
            appSignalShutdown(EX_IOERR);
            fileClose(f);
            return false;
        }
    }
    fileClose(f);
    return true;
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char *argv[]) {
    Cli cli;
    cli.header("cmdupd v"s + kVersion + " (" __DATE__ ")")
        .helpNoArgs()
        .versionOpt(kVersion, "cmdupd");
    auto & files = cli.optVec<string>("[FILE]")
        .desc("Files to search (via git ls-files) for updatable comments.");
    cli.opt<bool>(&s_update, "u update.").desc("Update found comments.");
    cli.opt<bool>("v verbose.")
        .check([](auto&, auto&, auto & val) {
            s_verbose += (val == "1");
            return true;
        })
        .desc("Show status of all matched files.");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    vector<string> args = { "git", "ls-files" };
    args.insert(args.end(), files->begin(), files->end());
    ExecResult res;
    execProgramWait(&res, args);
    if (res.exitCode) {
        logMsgError() << res.err.view();
        return appSignalShutdown(EX_OSERR);
    }
    auto rawNames = toString(res.out);
    vector<string_view> fnames;
    split(&fnames, rawNames, '\n');

    vector<string> dateArgs = { "git", "log", "--format=%aI", "-n1", "" };
    for (auto&& f : fnames) {
        f = trim(f);
        if (f.empty())
            continue;
        auto fname = (string) Path(f).filename();
        auto rule = findRule(fname);
        if (!rule) {
            if (s_verbose > 1)
                cout << f << "... skipped" << endl;
            continue;
        }

        s_files += 1;
        string content;
        if (!fileLoadBinaryWait(&content, f))
            break;
        vector<string_view> lines;
        split(&lines, content, '\n');
        if (!lines.empty() && lines.back().empty())
            lines.pop_back();

        forward_list<string> changedLines;
        string commitYear;
        for (auto&& line : lines) {
            line = rtrim(line);
            auto tmp = string(line);
            CopyrightYear cdate;
            if (!findYear(&cdate, tmp, *rule))
                continue;

            s_comments += 1;
            if (commitYear.empty()) {
                dateArgs.back() = f;
                execProgramWait(&res, dateArgs);
                TimePoint commitTime;
                tm tm;
                if (!timeParse8601(&commitTime, trim(res.out.view()))
                    || !timeToDesc(&tm, commitTime)
                ) {
                    logMsgError() << "No commit time for '" << f << "'";
                    return appSignalShutdown(EX_OSERR);
                }
                commitYear = StrFrom<int>(tm.tm_year + 1900);
            }
            if (cdate.lastYear != commitYear) {
                s_updatedComments += 1;
                if (cdate.format == 0) {
                    auto pos = cdate.lastYear.data() - tmp.data()
                        + cdate.lastYear.size();
                    tmp.insert(pos, commitYear);
                    tmp.insert(pos, " - ");
                } else {
                    tmp.replace(
                        cdate.lastYear.data() - tmp.data(),
                        cdate.lastYear.size(),
                        commitYear
                    );
                }
                changedLines.push_front(tmp);
                line = changedLines.front();
            }
        }
        if (commitYear.empty()) {
            cout << f << "... ";
            ConsoleScopedAttr attr(kConsoleWarn);
            cout << "no matching comments" << endl;
        } else {
            s_matchedFiles += 1;
            if (changedLines.empty()) {
                if (s_verbose) {
                    cout << f << "... unchanged" << endl;
                }
            } else {
                s_updatedFiles += 1;
                if (!s_update) {
                    cout << f << "... ";
                    ConsoleScopedAttr attr(kConsoleWarn);
                    cout << "out of date" << endl;
                } else {
                    if (!replaceFile(f, lines))
                        return;
                    cout << f << "... ";
                    ConsoleScopedAttr attr(kConsoleNote);
                    cout << "UPDATED" << endl;
                }
            }
        }
    }
    if (s_verbose > 1
        || s_verbose && s_files
        || s_matchedFiles != s_files
        || s_updatedFiles
    ) {
        cout << endl;
    }
    cout << "Files: " << fnames.size() << " selected, "
        << s_files << " checked";
    if (s_matchedFiles != s_files) {
        cout << ", ";
        ConsoleScopedAttr attr(kConsoleWarn);
        cout << s_files - s_matchedFiles << " unmatched";
    }
    if (s_updatedFiles) {
        if (s_update) {
            cout << ", ";
            ConsoleScopedAttr attr(kConsoleNote);
            cout << s_updatedFiles << " updated";
        } else {
            cout << ", ";
            ConsoleScopedAttr attr(kConsoleWarn);
            cout << s_updatedFiles << " would be updated";
        }
    }
    cout << endl;

    if (s_updatedFiles && !s_update)
        cout << "Use '-u' to update." << endl;

    logPauseStopwatch();
    return appSignalShutdown(EX_OK);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv);
}
