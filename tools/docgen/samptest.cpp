// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// samptest.cpp - docgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

struct CmdOpts {
    string cfgfile;
    string layout;
    unordered_set<string> pages;
    UnsignedSet lines;

    CmdOpts();
};

struct CodeBlock {
    unsigned line = 0;
    string lang;
    string content;
    vector<string> attrs;
};

enum TestAttr {
    // NOTE: These enums are ordered by priority of execution.
    kTestInvalid,
    kTestIgnore,
    kTestGetline,
    kTestNoScript,
    kTestLanguage,
    kTestAlternate,
    kTestFile,
    kTestPrefix,
    kTestReplace,
    kTestSubset,
    kTests
};
struct AttrInfo {
    vector<string_view> args;
};

struct FileInfo {
    unsigned line = 0;
    string lang;
    vector<string> lines;
};
struct ProgInfo {
    unsigned line = 0;
    unordered_map<string, FileInfo> files;
};
struct RunInfo {
    unsigned line = 0;
    string lang;
    vector<string> comments;
    string cmdline;
    map<size_t, int> input; // {line, pos} view into outputs
    vector<string> output;
};
struct TestInfo {
    unsigned line = 0;
    vector<ProgInfo> alts;
    vector<RunInfo> runs;
};

struct PageInfo {
    Config * out;
    const Page & page;
    function<void()> fn;

    string fname;
    string content;

    unsigned lastProgLine = 0;
    unordered_map<unsigned, TestInfo> tests;
    vector<string> testPrefix;
};

}


/****************************************************************************
*
*   Variables
*
***/

static CmdOpts s_opts;

constexpr TokenTable::Token s_testAttrs[] = {
    { kTestAlternate, "alt" },
    { kTestAlternate, "alternate" },
    { kTestFile, "file" },
    { kTestGetline, "getline" },
    { kTestIgnore, "ignore" },
    { kTestLanguage, "lang" },
    { kTestLanguage, "language" },
    { kTestNoScript, "noscript" },
    { kTestPrefix, "prefix" },
    { kTestReplace, "repl" },
    { kTestReplace, "replace" },
    { kTestSubset, "subset" },
};
const TokenTable s_testAttrTbl(s_testAttrs);

static vector<PageInfo> s_pageInfos;

static auto & s_perfCompile = uperf("docgen.compiles");
static auto & s_perfCompileFailed = uperf("docgen.compiles (failed)");
static auto & s_perfRun = uperf("docgen.runs");
static auto & s_perfRunFailed = uperf("docgen.runs (failed)");


/****************************************************************************
*
*   Generate test files
*
***/

//===========================================================================
static void genFileHeader(
    CharBuf * out,
    const string & fname,
    const string & prefix
) {
    out->append(prefix).append(fname).pushBack('\n')
        .append(prefix).append("Generated by docgen ").append(kVersion)
            .pushBack('\n');
}

//===========================================================================
inline static void genCppHeader(
    CharBuf * out,
    const string & fname,
    const string & srcfile,
    size_t srcline
) {
    out->append("/*******************************************************"
        "*********************\n*\n");
    genFileHeader(out, fname, "* ");
    out->append("*\n")
        .append("* Test program for sample located at:\n")
        .append("*   File: ").append(srcfile).pushBack('\n')
        .append("*   Line: ").append(StrFrom<size_t>(srcline)).pushBack('\n')
        .append("*\n")
        .append("***/\n\n");
}

//===========================================================================
static vector<CodeBlock> findAsciidocBlocks(const PageInfo & info) {
    static const regex s_codeBlock(
        // 1. attrs
        // 2. separator (---- or ////)
        // 3. content
        R"regex(
\[\s*[Ss][Oo][Uu][Rr][Cc][Ee]\s*,([^\]]*)\]\r?
(----|////)\r?
([[:print:][:cntrl:]]*?)\r?
\2(?=\r?
))regex",
        regex::optimize
    );

    vector<CodeBlock> out;
    cmatch m;
    size_t pos = 0;
    for (;;) {
        if (!regex_search(info.content.data() + pos, m, s_codeBlock))
            break;
        auto & code = out.emplace_back();
        code.line = (unsigned) (pos + m.position() + 1);
        auto rawAttrs = m.str(1);
        vector<string_view> attrs;
        split(&attrs, rawAttrs, ',');
        if (!attrs.empty()) {
            code.lang = toLower(trim(attrs[0]));
            for (auto i = 1; i < attrs.size(); ++i)
                code.attrs.emplace_back(trim(attrs[i]));
        }
        code.content = m.str(3);

        pos += m.position() + m.length();
    }

    // Convert character offsets to line numbers
    pos = 0;
    unsigned line = 1;
    for (auto&& code : out) {
        while (pos < code.line) {
            if (info.content[pos++] == '\n')
                line += 1;
        }
        code.line = line + 2;
    }

    return out;
}

//===========================================================================
static vector<CodeBlock> findBlocks(const PageInfo & info) {
    switch (info.page.type) {
    case Page::kAsciidoc:
        return findAsciidocBlocks(info);
    default:
        return {};
    }
}

//===========================================================================
static vector<pair<size_t, size_t>> lineSpans(
    const vector<string_view> & args,
    const vector<string_view> & lines,
    std::function<Dim::Detail::Log()> olog
) {
    vector<pair<size_t, size_t>> out;
    if (args.empty()) {
        olog() << "requires range arguments.";
        return out;
    }
    for (unsigned i = 0; i < args.size(); i += 2) {
        auto & range = out.emplace_back();
        range.first = strToInt(args[i]);
        range.second = args.size() == i + 1
            ? lines.size() - range.first
            : strToInt(args[i + 1]);
        if (range.first > range.first + range.second
            || range.first >= lines.size()
            || range.first + range.second > lines.size()
        ) {
            olog() << "invalid line span ("
                << range.first << ' ' << range.second
                << ") for block with " << lines.size() << " lines.";
            out.clear();
            return out;
        }
    }

    // check for overlapping ranges
    auto sorted = out;
    sort(sorted.begin(), sorted.end(), [](auto & a, auto & b) {
        return a < b;
    });
    for (unsigned i = 0; i + 1 < sorted.size(); ++i) {
        if (sorted[i].first + sorted[i].second > sorted[i + 1].first) {
            olog() << "overlapping line spans.";
            out.clear();
            return out;
        }
    }

    return out;
}

//===========================================================================
static void selectLines(
    vector<string_view> * matched,
    vector<string_view> * unmatched,
    const vector<string_view> & args,
    const vector<string_view> & lines,
    std::function<Dim::Detail::Log()> olog
) {
    if (matched)
        matched->clear();
    if (unmatched)
        unmatched->clear();
    auto spans = lineSpans(args, lines, olog);
    if (spans.empty())
        return;
    auto b = lines.begin();
    auto pos = 0;
    for (auto&& span : spans) {
        if (unmatched) {
            unmatched->insert(
                unmatched->end(),
                b + pos,
                b + span.first
            );
        }
        if (matched) {
            matched->insert(
                matched->end(),
                b + span.first,
                b + span.first + span.second
            );
        }
    }
    if (unmatched) {
        auto & span = spans.back();
        unmatched->insert(
            unmatched->end(),
            b + span.first + span.second,
            lines.end()
        );
    }
}

//===========================================================================
static multimap<TestAttr, AttrInfo> getAttrs(const CodeBlock & blk) {
    multimap<TestAttr, AttrInfo> attrs;
    for (auto&& val : blk.attrs) {
        AttrInfo tmp;
        split(&tmp.args, val);
        if (tmp.args.size() < 2 || tmp.args[0] != "test")
            continue;
        auto attr = tokenTableGetEnum(
            s_testAttrTbl,
            tmp.args[1],
            kTestInvalid
        );
        if (!attr)
            continue;
        tmp.args.erase(tmp.args.begin(), tmp.args.begin() + 2);
        attrs.emplace(attr, move(tmp));
    }
    return attrs;
}

//===========================================================================
static void badAttrPrefix(
    ostream & os,
    const PageInfo & info,
    const CodeBlock & blk,
    TestAttr attr
) {
    os << info.page.file << ", line " << blk.line
        << ": " << tokenTableGetName(s_testAttrTbl, attr)
        << " attribute ignored, ";
}

//===========================================================================
static void processCompiler(
    PageInfo * info,
    TestInfo * ti,
    const CodeBlock & blk
) {
    string fname = "main";
    auto lang = blk.lang;
    bool keepAlts = false;
    bool keepFiles = false;

    auto attrs = getAttrs(blk);
    auto iter = attrs.begin();
    auto attr = iter == attrs.end() ? kTestInvalid : iter->first;

    auto logger = [&info, &blk](TestAttr attr) {
        return [attr, &info, &blk]() {
            auto os = logMsgWarn();
            badAttrPrefix(os, *info, blk, attr);
            return os;
        };
    };
    auto olog = [&logger](TestAttr attr) {
        return logger(attr)();
    };

    if (attr == kTestIgnore) {
        if (iter->second.args.size() != 0) {
            olog(attr) << "expects no arguments.";
        }
        return;
    }
    if (attr == kTestLanguage) {
        auto & args = iter->second.args;
        if (args.size() != 1) {
            olog(attr) << "must have language argument.";
        } else {
            lang = args[0];
        }
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }
    if (attr == kTestAlternate) {
        if (ti->alts.empty()) {
            olog(attr) << "no previously defined program.";
        } else if (iter->second.args.size() != 0) {
            olog(attr) << "expects no arguments.";
        } else {
            keepAlts = true;
        }
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }
    if (attr == kTestFile) {
        auto & keys = iter->second.args;
        if (keys.size() != 1) {
            olog(attr) << "must have filename argument.";
        } else {
            keepFiles = true;
            fname = keys[0];
        }
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }
    vector<string_view> lines;
    split(&lines, blk.content, '\n');
    for (auto&& line : lines)
        line = rtrim(line);
    if (attr == kTestPrefix) {
        vector<string_view> matched;
        vector<string_view> unmatched;
        selectLines(
            &matched,
            &unmatched,
            iter->second.args,
            lines,
            logger(attr)
        );
        if (!matched.empty())
            info->testPrefix.assign(matched.begin(), matched.end());
        lines = unmatched;
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }

    if (attr == kTestReplace)
        keepFiles = true;

    ProgInfo prog;
    if (ti->alts.empty() || !keepFiles) {
        prog.line = blk.line;
    } else {
        prog = ti->alts.back();
        prog.line = blk.line;
    }
    auto & file = prog.files[fname];
    file.lang = lang;
    file.line = blk.line;

    if (attr == kTestReplace) {
        if (next(iter) != attrs.end()) {
            attr = (++iter)->first;
            olog(kTestReplace)
                << tokenTableGetName(s_testAttrTbl, attr)
                << " attribute takes precedence.";
        } else if (file.lines.empty()) {
            olog(attr) << "program must already be defined.";
        } else {
            vector<string_view> flines(file.lines.begin(), file.lines.end());
            auto rawSpans = lineSpans(iter->second.args, flines, logger(attr));
            if (rawSpans.empty()) {
                // argument parsing failed
                goto ABORT_REPLACE;
            }
            struct Replacement {
                pair<size_t, size_t> dst;
                pair<size_t, size_t> src;
            };
            vector<Replacement> repls;
            size_t spos = 0;
            for (auto&& span : rawSpans) {
                if (spos >= lines.size()) {
                    // spos is size() + 1 unless the block ends with "...".
                    olog(attr) << "more ranges than block sections.";
                    goto ABORT_REPLACE;
                }
                auto & repl = repls.emplace_back();
                repl.dst = span;
                repl.src.first = spos;
                for (; spos < lines.size(); ++spos) {
                    if (trim(lines[spos]) == "...")
                        break;
                }
                repl.src.second = spos - repl.src.first;
                spos += 1;
            }
            sort(repls.begin(), repls.end(), [](auto & a, auto & b) {
                return a.dst.first < b.dst.first;
            });

            vector<string_view> out;
            size_t dpos = 0;
            for (auto&& repl : repls) {
                out.insert(
                    out.end(),
                    file.lines.begin() + dpos,
                    file.lines.begin() + repl.dst.first
                );
                dpos = repl.dst.first + repl.dst.second;
                out.insert(
                    out.end(),
                    lines.begin() + repl.src.first,
                    lines.begin() + repl.src.first + repl.src.second
                );
            }
            out.insert(
                out.end(),
                file.lines.begin() + dpos,
                file.lines.end()
            );
            lines = out;
            keepFiles = true;
        }
    ABORT_REPLACE:
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }

    if (attr == kTestSubset) {
        if (next(iter) != attrs.end()) {
            attr = (++iter)->first;
            olog(kTestSubset) << "subset attribute ignored in favor of "
                << tokenTableGetName(s_testAttrTbl, attr)
                << " attribute.";
        } else {
            vector<string_view> matched;
            selectLines(
                &matched,
                nullptr,
                iter->second.args,
                lines,
                logger(attr)
            );
            if (!matched.empty())
                lines = matched;
        }
        attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
    }

    assert(attr == kTestInvalid);
    if (ti->runs.empty()) {
        if (!ti->line)
            ti->line = blk.line;
    } else {
        info->tests[ti->line] = *ti;
        ti->runs.clear();
        ti->line = blk.line;
    }
    // Materialize the new lines in case what they're referencing is removed
    // by removing the alts or program.
    vector<string> newLines(lines.begin(), lines.end());
    file.lines = move(newLines);
    if (!keepAlts)
        ti->alts.clear();
    ti->alts.push_back(move(prog));
    info->lastProgLine = ti->alts.back().line;
}

//===========================================================================
static map<unsigned, int> getInputs(
    const multimap<TestAttr, AttrInfo> & attrs,
    const PageInfo & info,
    const CodeBlock & blk
) {
    map<unsigned, int> out;
    auto ii = attrs.equal_range(kTestGetline);
    for (auto i = ii.first; i != ii.second; ++i) {
        auto & args = i->second.args;
        if (args.size() != 2) {
            auto os = logMsgWarn();
            badAttrPrefix(os, info, blk, kTestGetline);
            os << "must have line and start position arguments.";
            continue;
        }
        unsigned line = strToUint(args[0]);
        int pos = strToInt(args[1]);
        out[line] = pos;
    }
    return out;
}

//===========================================================================
static void processScript(
    PageInfo * info,
    TestInfo * ti,
    const CodeBlock & blk
) {
    auto lname = blk.lang;

    auto attrs = getAttrs(blk);
    if (attrs.contains(kTestIgnore))
        return;

    if (auto i = attrs.find(kTestLanguage); i != attrs.end()) {
        auto & args = i->second.args;
        if (!args.empty())
            lname = args.front();
    }
    if (!info->out->scripts.contains(lname))
        return;
    auto & lang = *info->out->scripts[lname];

    if (attrs.contains(kTestNoScript)) {
        auto & run = ti->runs.emplace_back();
        run.line = blk.line;
        run.lang = lname;
        return;
    }

    auto input = getInputs(attrs, *info, blk);

    vector<string_view> lines;
    split(&lines, blk.content, '\n');
    enum { kInvalid, kComment, kExecute, kInput, kOutput };
    vector<int> types;
    for (auto&& line : lines) {
        line = rtrim(line);
        int type = kOutput;
        if (line.starts_with(lang.commentPrefix)) {
            type = kComment;
        } else if (line.starts_with(lang.prefix)) {
            type = kExecute;
        }
        types.push_back(type);
    }
    types.push_back(kInvalid);

    for (unsigned i = 0; i < lines.size();) {
        RunInfo run;
        run.line = blk.line + i;
        run.lang = lname;
        for (; types[i] == kComment; ++i) {
            auto cmt = lines[i].substr(lang.commentPrefix.size());
            cmt = trim(cmt);
            run.comments.emplace_back(cmt);
        }
        if (types[i] == kInvalid)
            return;
        if (types[i] != kExecute) {
            logMsgWarn() << info->page.file << ", line " << blk.line + i
                << ": expected executable statement in script block.";
            return;
        }
        run.cmdline = trim(lines[i].substr(lang.prefix.size()));
        for (++i; types[i] == kOutput; ++i) {
            while (input.contains(i)) {
                auto len = (int) lines[i].size();
                auto pos = input[i];
                if (pos >= 0) {
                    if (pos >= len)
                        break;
                } else {
                    if (-pos > len)
                        break;
                    pos += len;
                }
                run.input[run.output.size()] = pos;
                break;
            }
            run.output.emplace_back(lines[i]);
        }
        ti->runs.push_back(move(run));
    }
}

//===========================================================================
static string testPath(
    const PageInfo & info,
    size_t line
) {
    auto seg = info.page.urlSegment;
    auto str = StrFrom<size_t>(line);
    seg.push_back('/');
    seg.append(digits10(info.lastProgLine) - size(str), '0');
    seg.append(str);
    return seg;
}

//===========================================================================
static string fullTestPath(
    const PageInfo & info,
    size_t line
) {
    Path root(testPath(info, line));
    root.resolve(info.out->sampDir);
    root.resolve(info.out->configFile.parentPath());
    return root.str();
}

//===========================================================================
static void addOutputScript(
    PageInfo * info,
    const TestInfo & test,
    const ProgInfo & prog
) {
    auto & cfg = *info->out;

    CharBuf content;
    genFileHeader(&content, "script.txt", "; ");
    content.append("; From '")
        .append(info->page.file)
        .append("', line ")
        .append(StrFrom<unsigned>(test.runs.front().line))
        .append("\n");
    content.append(";\n");
    auto len = content.size();
    for (auto&& run : test.runs) {
        if (run.cmdline.empty())
            continue;
        content.append("; Line ")
            .append(StrFrom<unsigned>(run.line))
            .append("\n");
        for (auto&& cmt : run.comments)
            content.append("# ").append(cmt).append("\n");
        content.append("$ ").append(run.cmdline).append("\n");
        for (unsigned pos = 0; pos < run.output.size(); ++pos) {
            if (run.input.contains(pos)) {
                content.append("< ")
                    .append(run.output[pos].data() + run.input.at(pos))
                    .pushBack('\n');
            }
            content.append("> ").append(run.output[pos]).pushBack('\n');
        }
    }
    if (len == content.size()) {
        // No runs were added
        content.append("; No script, compile only test\n");
    }
    auto path = Path(testPath(*info, prog.line)) / "script.txt";
    addOutput(&cfg, path.str(), move(content));
}

//===========================================================================
static void processPage(PageInfo * info, unsigned phase = 0) {
    string layname = s_opts.layout.empty() ? "default" : s_opts.layout;
    auto & cfg = *info->out;

    if (info->content.empty()) {
        logMsgError() << info->page.file << ": unable to load content";
        appSignalShutdown(EX_DATAERR);
        return;
    }

    vector<CodeBlock> blocks = findBlocks(*info);
    TestInfo ti;
    for (auto&& blk : blocks) {
        if (cfg.compilers.contains(blk.lang)) {
            processCompiler(info, &ti, blk);
        } else if (cfg.scripts.contains(blk.lang)) {
            processScript(info, &ti, blk);
        }
    }

    for (auto&& test : info->tests) {
        for (auto&& prog : test.second.alts) {
            Path root(testPath(*info, prog.line));
            for (auto&& file : prog.files) {
                auto path = root / file.first;
                auto & comp = cfg.compilers[file.second.lang];
                path.defaultExt(comp->fileExt);
                CharBuf content;
                genCppHeader(
                    &content,
                    path.str(),
                    info->page.file,
                    file.second.line
                );
                for (auto&& line : info->testPrefix)
                    content.append(line).append("\n");
                for (auto&& line : file.second.lines)
                    content.append(line).append("\n");
                addOutput(&cfg, path.str(), move(content));
            }
            addOutputScript(info, test.second, prog);
        }
    }

    info->fn();
}


/****************************************************************************
*
*   Run tests
*
***/

struct ProgWork {
    function<void()> fn;
    const PageInfo & info;
    const TestInfo & test;
    const ProgInfo & prog;

    unsigned pendingWork = 0;
};

static void runProgTests(ProgWork * work, unsigned phase = 0);

//===========================================================================
static void runProgDone(
    ProgWork * work,
    unsigned phase,
    const RunInfo & run,
    ExecResult && res
) {
    string out = toString(res.out) + toString(res.err);
    vector<string_view> lines;
    split(&lines, out, '\n');
    vector<string> tmp;
    for (auto il = 0; il < lines.size(); ++il) {
        if (run.input.contains(il)) {
            auto pos = run.input.at(il);
            auto line = lines[il];
            auto & withInput =
                tmp.emplace_back(line.substr(0, pos));
            withInput +=
                run.output[il].substr(run.input.at(il));
            lines.insert(
                lines.begin() + il + 1,
                line.substr(pos)
            );
            lines[il] = withInput;
        }
    }
    auto i = -1;
    auto olen = run.output.size();
    if (lines.size() != olen) {
        if (lines.size() == olen + 1
            && rtrim(lines.back()).empty()
        ) {
            lines.pop_back();
        } else if (lines.size() + 1 == olen
            && run.output.back().empty()
        ) {
            // last output line blank, ignore it
            olen -= 1;
        }
    }
    auto len = min(lines.size(), olen);
    for (i = 0; i < len; ++i) {
        auto line = rtrim(lines[i]);
        if (run.output[i] != line)
            goto FAILED;
    }
    if (lines.size() != olen)
        goto FAILED;
    runProgTests(work, phase);
    return;

FAILED:
    s_perfRunFailed += 1;
    logMsgError() << run.line << ", "
        << testPath(work->info, work->prog.line);
    for (auto pos = 0; auto&& line : lines) {
        if (pos++ == i) {
            logMsgInfo() << "+ " << line;
            logMsgInfo() << "- " << run.output[i];
        } else {
            logMsgInfo() << "> " << line;
        }
    }
    work->pendingWork = 1;
    runProgTests(work, phase);
}

//===========================================================================
static void runProgTests(ProgWork * work, unsigned phase) {
    if (appStopping()) {
        work->fn();
        return;
    }

    unsigned what = 0;
    auto & cfg = *work->info.out;
    auto workDir = Path(fullTestPath(work->info, work->prog.line));
    auto workDrive = workDir.drive();
    auto workDriveDir = fileGetCurrentDir(workDrive);
    auto curDir = fileGetCurrentDir();

    if (phase == what++) {
        // Launch compilers
        auto filesByLang = unordered_map<string, vector<string>>();

        for (auto&& file : work->prog.files)
            filesByLang[file.second.lang].push_back(file.first);
        work->pendingWork = (unsigned) filesByLang.size() + 1;
        static bool first = true;
        for (auto&& [lang, files] : filesByLang) {
            auto comp = cfg.compilers[lang];

            auto args = comp->compileArgs;
            for (auto&& file : files) {
                Path fname(file);
                fname.defaultExt(comp->fileExt);
                args.push_back(fname.str());
            }
            auto cmdline = Cli::toCmdline(args);
            auto title = lang + ", " + testPath(work->info, work->prog.line);
            if (first) {
                first = false;
                work->pendingWork -= 1;
                auto out = execWait(cmdline, title, workDir);
                if (out.empty()) {
                    appSignalShutdown(EX_DATAERR);
                    return;
                }
            } else {
                s_perfCompile += 1;
                exec(
                    [work, what](string && out) {
                        if (out.empty())
                            s_perfCompileFailed += 1;
                        runProgTests(work, what);
                    },
                    cmdline,
                    title,
                    workDir
                );
            }
        }

        phase = what;
    }
    if (phase == what++) {
        if (--work->pendingWork) {
            // Still more compiles to process.
            return;
        }

        work->pendingWork = (unsigned) work->test.runs.size() + 1;
        phase = what;
    }
    if (phase == what++) {
        while (--work->pendingWork) {
            // Execute just the next waiting run.
            auto pos = work->test.runs.size() - work->pendingWork;
            auto & run = work->test.runs[pos];
            if (run.cmdline.empty())
                continue;
            auto & lang = *cfg.scripts[run.lang];

            auto args = Cli::toArgv(run.cmdline);
            args.insert(
                args.begin(),
                lang.shellArgs.begin(),
                lang.shellArgs.end()
            );
            auto cmd = Cli::toCmdline(args);

            string input;
            for (auto&& [line, pos] : run.input) {
                input += run.output.at(line).substr(pos);
                input += '\n';
            }

            fileSetCurrentDir(workDir);
            s_perfRun += 1;
            execProgram(
                [work, phase, &run](ExecResult && res) {
                    runProgDone(work, phase, run, move(res));
                },
                cmd,
                { .stdinData = input }
            );
            fileSetCurrentDir(workDriveDir);
            if (curDir != workDriveDir)
                fileSetCurrentDir(curDir);
            return;
        }

        work->fn();
        return;
    }

    assert(!"unknown phase");
}

//===========================================================================
static void runTests(
    function<void()> fn,
    const vector<PageInfo> & infos
) {
    if (infos.empty())
        return;
    auto out = infos.front().out;
    for (auto&& info : infos) {
        for (auto&& test : info.tests)
            out->pendingWork += (unsigned) test.second.alts.size();
    }

    for (auto&& info : infos) {
        for (auto&& test : info.tests) {
            for (auto&& prog : test.second.alts) {
                if (!s_opts.lines.empty()
                    && !s_opts.lines.contains(prog.line)
                ) {
                    out->pendingWork -= 1;
                    continue;
                }
                auto work = new ProgWork ({
                    .fn = fn,
                    .info = info,
                    .test = test.second,
                    .prog = prog
                });
                runProgTests(work);
            }
        }
    }
}

//===========================================================================
static void testSamples(Config * out, unsigned phase = 0) {
    if (appStopping())
        return;

    unsigned what = 0;

    if (phase == what++) {
        // Generate tests
        string layname = s_opts.layout.empty() ? "default" : s_opts.layout;
        auto layout = out->layouts.find(layname);

        // Generate tests for layout
        s_pageInfos.reserve(layout->second.pages.size());
        out->pendingWork = (unsigned) layout->second.pages.size() + 1;
        for (auto && page : layout->second.pages) {
            if (page.type != Page::kAsciidoc) {
                out->pendingWork -= 1;
                continue;
            }
            if (!s_opts.pages.empty()
                && !s_opts.pages.contains(page.urlSegment)
            ) {
                out->pendingWork -= 1;
                continue;
            }
            auto & info = s_pageInfos.emplace_back(out, page);
            info.fn = [info, what]() {
                testSamples(info.out, what);
            };
            loadContent(
                [&info, what](auto && content) {
                    info.content = move(content);
                    processPage(&info);
                },
                *info.out,
                "HEAD",
                info.page.file
            );
        }

        phase = what;
    }
    if (phase == what++) {
        if (--out->pendingWork) {
            // Still have more pages to process.
            return;
        }

        // Replace test output directory with all the new files.
        auto odir = Path(out->sampDir).resolve(out->configFile.parentPath());
        if (!writeOutputs(odir, out->outputs))
            return;
        auto count = out->outputs.size();
        logMsgInfo() << count << " generated files.";
        {
            ConsoleScopedAttr ca(kConsoleNote);
            logMsgInfo() << "Tests generated successfully.";
        }

        // Compile and execute tests
        out->pendingWork = 1;
        runTests(
            [out, what]() { testSamples(out, what); },
            s_pageInfos
        );

        phase = what;
    }
    if (phase == what++) {
        if (--out->pendingWork) {
            // Still have more tests to process.
            return;
        }

        // Clean up
        delete out;

        logMsgInfo() << s_perfCompile << " programs compiled.";
        logMsgInfo() << s_perfRun << " runs executed.";
        if (auto errs = s_perfCompileFailed + s_perfRunFailed) {
            ConsoleScopedAttr ca(kConsoleError);
            logMsgInfo() << "Compile and run failures: " << errs;
        } else {
            ConsoleScopedAttr ca(kConsoleCheer);
            logMsgInfo() << "Tests run successfully.";
        }

        logPauseStopwatch();
        appSignalShutdown(EX_OK);
        return;
    }

    assert(!"unknown phase");
}


/****************************************************************************
*
*   Command line
*
***/

static bool testCmd(Cli & cli);

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
    cli.command("test")
        .desc("Test code samples embedded in documentation files")
        .action(testCmd);
    cli.opt(&cfgfile, "f file", "docgen.xml")
        .desc("Site definition to process.");
    cli.opt(&layout, "layout", "default")
        .desc("Layout with files to process.");

    enum { kPage, kLine } mode = kPage;
    cli.opt(&mode, "page", kPage).flagValue(true)
        .desc("Following operands are the urls of the pages within the "
            "layout to process.");
    cli.opt(&mode, "line", kLine).flagValue()
        .desc("Following operands are line numbers, or line number "
            "ranges, of the pages that may contain samples to be processed.");
    cli.optVec<string>("[operand]")
        .desc("Page urls and/or line ranges that filter samples to process.")
        .check([this, &mode](auto & cli, auto & opt, auto & val) {
            if (mode == kPage) {
                pages.insert(val);
            } else {
                UnsignedSet tmp(val);
                lines.insert(tmp);
            }
            return true;
        });
}

//===========================================================================
static bool testCmd(Cli & cli) {
    auto cfg = loadConfig(s_opts.cfgfile);
    if (!cfg)
        return cli.fail(EX_DATAERR, "");
    if (cfg->sampDir.empty()) {
        logMsgError() << cfg->configFile
            << ": Output directory for samples unspecified.";
        return cli.fail(EX_DATAERR, "");
    }

    logMsgInfo() << "Making TEST files from '" << cfg->configFile << "' to '"
        << cfg->sampDir << "'.";
    testSamples(cfg.release());

    return cli.fail(EX_PENDING, "");
}
