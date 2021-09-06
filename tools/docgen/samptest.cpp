// Copyright Glen Knowles 2020 - 2021.
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

    bool update;
    bool compile;

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
    kTestLanguage,
    kTestNoScript,
    kTestGetline,
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
struct CodeLine {
    string text;

    // Offset to input characters to be stripped off the end of the code text
    // string. Only for scripts.
    int input;
};
struct RunInfo {
    unsigned line = 0;
    string lang;
    vector<string> comments;
    string cmdline;
    vector<CodeLine> output;
    map<string, string> env;
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
    unordered_map<string, vector<CodeLine>> testPrefix;
    unordered_map<string, vector<CodeLine>> scriptLines;
};

struct ApplyAttrState {
    std::function<Dim::Detail::Log(TestAttr attr)> logger;
    unsigned fileLine;          // position of source block in file
    multimap<TestAttr, AttrInfo> attrs;
    string lang;                // source block lang
    vector<CodeLine> lines;     // source block lines

    multimap<TestAttr, AttrInfo>::iterator iter;
    TestAttr attr;

    ApplyAttrState(const CodeBlock & blk, const string & file);
    void nextAttr();
    Dim::Detail::Log olog();
};

} // namespace


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
static void genCppHeader(
    CharBuf * out,
    const string & fname,
    const string & srcfile,
    size_t srcline
) {
    out->append("/*******************************************************"
        "*********************\n*\n");
    genFileHeader(out, fname, "*   ");
    out->append("*\n")
        .append("*   Test program for sample located at:\n")
        .append("*       File: ").append(srcfile).pushBack('\n')
        .append("*       Line: ").append(StrFrom(srcline).view())
            .pushBack('\n')
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
    const vector<CodeLine> & lines,
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
        if (args.size() == i + 1) {
            range.second = lines.size() - range.first;
        } else {
            range.second = strToInt(args[i + 1]);
        }
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
    vector<CodeLine> * matched,
    vector<CodeLine> * unmatched,
    const vector<string_view> & args,
    const vector<CodeLine> & lines,
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
static multimap<TestAttr, AttrInfo> getAttrs(
    const CodeBlock & blk,
    const string & file
) {
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
        if (!attr) {
            logMsgWarn() << file << ", line " << blk.line
                << ": unknown attribute: '" << tmp.args[1] << "'";
            continue;
        }
        tmp.args.erase(tmp.args.begin(), tmp.args.begin() + 2);
        attrs.emplace(attr, move(tmp));
    }
    return attrs;
}


/****************************************************************************
*
*   Apply block attributes
*
***/

//===========================================================================
ApplyAttrState::ApplyAttrState(const CodeBlock & blk, const string & file)
    : fileLine(blk.line)
    , attrs(getAttrs(blk, file))
    , iter(attrs.begin())
    , attr(iter == attrs.end() ? kTestInvalid : iter->first)
    , lang(blk.lang)
{
    vector<string_view> rawLines;
    split(&rawLines, blk.content, '\n');
    for (auto&& line : rawLines) {
        auto & cl = lines.emplace_back();
        cl.text = rtrim(line);
        cl.input = (int) cl.text.size();
    }
}

//===========================================================================
void ApplyAttrState::nextAttr() {
    attr = ++iter == attrs.end() ? kTestInvalid : iter->first;
}

//===========================================================================
Dim::Detail::Log ApplyAttrState::olog() {
    return logger(attr);
}

//===========================================================================
// true if ignore enabled
static bool applyIgnoreAttr(ApplyAttrState * as) {
    if (as->attr != kTestIgnore)
        return false;

    if (as->iter->second.args.size() != 0) {
        as->olog() << "expects no arguments.";
    }
    return true;
}

//===========================================================================
// true if language updated
static bool applyLanguageAttr(ApplyAttrState * as) {
    if (as->attr != kTestLanguage)
        return false;

    auto & args = as->iter->second.args;
    as->nextAttr();
    if (args.size() != 1) {
        as->olog() << "must have language argument.";
        return false;
    } else {
        as->lang = args[0];
        return true;
    }
}

//===========================================================================
// true if prefix updated
static bool applyPrefixAttr(
    ApplyAttrState * as,
    vector<CodeLine> * prefix
) {
    if (as->attr != kTestPrefix)
        return false;

    vector<CodeLine> matched;
    vector<CodeLine> unmatched;
    selectLines(
        &matched,
        &unmatched,
        as->iter->second.args,
        as->lines,
        [&as]() { return as->olog(); }
    );
    as->lines = unmatched;
    as->nextAttr();
    if (!matched.empty()) {
        prefix->assign(matched.begin(), matched.end());
        return true;
    }
    return false;
}

//===========================================================================
// true if replacement made
static bool applyReplaceAttr(
    ApplyAttrState * as,
    const vector<CodeLine> & prevLines
) {
    if (as->attr != kTestReplace)
        return false;

    auto fail = [&as]() { as->nextAttr(); return false; };

    if (auto ni = next(as->iter); ni != as->attrs.end()) {
        as->olog()
            << tokenTableGetName(s_testAttrTbl, ni->first)
            << " attribute takes precedence.";
        return fail();
    }
    if (prevLines.empty()) {
        as->olog() << "program must already be defined.";
        return fail();
    }

    auto rawSpans = lineSpans(
        as->iter->second.args,
        prevLines,
        [&as]() { return as->olog(); }
    );
    if (rawSpans.empty()) {
        // argument parsing failed
        return fail();
    }
    struct Replacement {
        pair<size_t, size_t> dst;
        pair<size_t, size_t> src;
    };
    vector<Replacement> repls;
    size_t spos = 0;
    for (auto&& span : rawSpans) {
        if (spos >= as->lines.size()) {
            // spos is size() + 1 unless the block ends with "...".
            as->olog() << "more ranges than block sections.";
            return fail();
        }
        auto & repl = repls.emplace_back();
        repl.dst = span;
        repl.src.first = spos;
        for (; spos < as->lines.size(); ++spos) {
            if (trim(as->lines[spos].text) == "...")
                break;
        }
        repl.src.second = spos - repl.src.first;
        spos += 1;
    }
    sort(repls.begin(), repls.end(), [](auto & a, auto & b) {
        return a.dst.first < b.dst.first;
    });

    vector<CodeLine> out;
    size_t dpos = 0;
    for (auto&& repl : repls) {
        out.insert(
            out.end(),
            prevLines.begin() + dpos,
            prevLines.begin() + repl.dst.first
        );
        dpos = repl.dst.first + repl.dst.second;
        out.insert(
            out.end(),
            as->lines.begin() + repl.src.first,
            as->lines.begin() + repl.src.first + repl.src.second
        );
    }
    out.insert(
        out.end(),
        prevLines.begin() + dpos,
        prevLines.end()
    );
    as->lines = out;
    as->nextAttr();
    return true;
}

//===========================================================================
// true if lines replaced with subset of them
static bool applySubsetAttr(ApplyAttrState * as) {
    bool applied = false;
    if (as->attr != kTestSubset)
        return applied;

    if (auto ni = next(as->iter); ni != as->attrs.end()) {
        as->olog() << "subset attribute ignored in favor of "
            << tokenTableGetName(s_testAttrTbl, ni->first)
            << " attribute.";
    } else {
        vector<CodeLine> matched;
        selectLines(
            &matched,
            nullptr,
            as->iter->second.args,
            as->lines,
            [&as]() { return as->olog(); }
        );
        if (!matched.empty()) {
            as->lines = matched;
            applied = true;
        }
    }
    as->nextAttr();
    return applied;
}

//===========================================================================
// true if ignore enabled
static bool applyNoScriptAttr(ApplyAttrState * as) {
    if (as->attr != kTestNoScript)
        return false;

    if (as->iter->second.args.size() != 0) {
        as->olog() << "expects no arguments.";
    }
    return true;
}

//===========================================================================
// true if any code lines have input text
static bool applyGetlineAttr(ApplyAttrState * as) {
    bool applied = false;
    for (; as->attr == kTestGetline; as->nextAttr()) {
        auto & args = as->iter->second.args;
        if (args.size() != 2) {
            as->olog() << "must have line and start position arguments.";
            continue;
        }
        unsigned line = strToUint(args[0]);
        if (line >= as->lines.size()) {
            as->olog() << "start line past end of block.";
            continue;
        }
        auto pos = strToInt(args[1]);
        auto len = (int) as->lines[line].text.size();
        if (pos >= len || -pos > len) {
            as->olog() << "start character must be within line.";
            continue;
        }
        if (pos < 0)
            pos += len;
        as->lines[line].input = pos;
        applied = true;
    }
    return applied;
}


/****************************************************************************
*
*   Process code blocks
*
***/

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
    ApplyAttrState * as
) {
    assert(info->out->compilers.contains(as->lang));

    string fname = "main";
    bool keepAlts = false;
    bool keepFiles = false;

    if (as->attr == kTestAlternate) {
        if (ti->alts.empty()) {
            as->olog() << "no previously defined program.";
        } else if (as->iter->second.args.size() != 0) {
            as->olog() << "expects no arguments.";
        } else {
            keepAlts = true;
        }
        as->nextAttr();
    }
    if (as->attr == kTestFile) {
        auto & keys = as->iter->second.args;
        if (keys.size() != 1) {
            as->olog() << "must have filename argument.";
        } else {
            keepFiles = true;
            fname = keys[0];
        }
        as->nextAttr();
    }
    applyPrefixAttr(as, &info->testPrefix[as->lang]);
    applySubsetAttr(as);

    vector<CodeLine> flines;
    if (!ti->alts.empty() && ti->alts.back().files.contains(fname)) {
        auto & lines = ti->alts.back().files[fname].lines;
        for (auto&& line : lines) {
            auto & fl = flines.emplace_back();
            fl.text = line;
            fl.input = (int) fl.text.size();
        }
    }
    if (applyReplaceAttr(as, flines))
        keepFiles = true;
    assert(as->attr == kTestInvalid);

    ProgInfo prog;
    if (ti->alts.empty() || !keepFiles) {
        prog.line = as->fileLine;
    } else {
        prog = ti->alts.back();
        prog.line = as->fileLine;
    }
    auto & file = prog.files[fname];
    file.lang = as->lang;
    file.line = as->fileLine;

    if (ti->runs.empty()) {
        if (!ti->line)
            ti->line = as->fileLine;
    } else {
        info->tests[ti->line] = *ti;
        ti->runs.clear();
        ti->line = as->fileLine;
    }
    // Materialize the new lines in case what they're referencing is removed
    // by removing the alts or program.
    file.lines.clear();
    for (auto&& line : as->lines) {
        file.lines.push_back(line.text);
    }
    if (!keepAlts)
        ti->alts.clear();
    ti->alts.push_back(move(prog));
    info->lastProgLine = ti->alts.back().line;
}

//===========================================================================
static void processScript(
    PageInfo * info,
    TestInfo * ti,
    ApplyAttrState * as
) {
    assert(info->out->scripts.contains(as->lang));

    if (applyNoScriptAttr(as)) {
        auto & run = ti->runs.emplace_back();
        run.line = as->fileLine;
        run.lang = as->lang;
        return;
    }
    applyGetlineAttr(as);

    auto & scriptLines = info->scriptLines[as->lang];
    applyReplaceAttr(as, scriptLines);
    scriptLines = as->lines;

    applySubsetAttr(as);

    assert(as->attr == kTestInvalid);
    auto & lang = *info->out->scripts[as->lang];

    enum { kInvalid, kComment, kExecute, kInput, kOutput };
    vector<int> types;
    for (auto&& line : as->lines) {
        auto text = rtrim(line.text);
        line.text = text;
        int type = kOutput;
        if (text.starts_with(lang.commentPrefix)) {
            type = kComment;
        } else if (text.starts_with(lang.prefix)) {
            type = kExecute;
        }
        types.push_back(type);
    }
    types.push_back(kInvalid);

    map<string, string> env;
    for (unsigned i = 0; i < as->lines.size();) {
        auto text = (string_view) as->lines[i].text;
        auto input = as->lines[i].input != text.size();
        RunInfo run;
        run.line = as->fileLine + i;
        run.lang = as->lang;
        while (types[i] == kComment) {
            if (input) {
                logMsgWarn() << info->page.file << ", line "
                    << as->fileLine + i << ": getline attribute only allowed "
                    << "for output lines.";
            }
            auto cmt = trim(text.substr(lang.commentPrefix.size()));
            run.comments.emplace_back(cmt);

            i += 1;
            if (types[i] == kInvalid) {
                // End of list.
                return;
            }
            text = (string_view) as->lines[i].text;
            input = as->lines[i].input != text.size();
        }
        if (types[i] != kExecute) {
            logMsgWarn() << info->page.file << ", line " << as->fileLine + i
                << ": expected executable statement in script block.";
            return;
        }
        if (input) {
            logMsgWarn() << info->page.file << ", line "
                << as->fileLine + i << ": getline attribute only allowed "
                << "for output lines.";
        }
        run.cmdline = trim(text.substr(lang.prefix.size()));
        for (auto&& eset : lang.envSets) {
            match_results<string_view::const_iterator> m;
            if (regex_match(text.begin(), text.end(), m, eset)) {
                assert(m.size() == 3);
                auto n = m[1].str();
                auto v = m[2].str();
                if (n.empty()) {
                    logMsgWarn() << info->page.file << ", line "
                        << as->fileLine + i
                        << ": setting environment variable with no name.";
                    return;
                }
                if (v.empty()) {
                    env.erase(n);
                } else {
                    env[n] = v;
                }
                run.cmdline.clear();
                break;
            }
        }
        run.env = env;
        for (++i; types[i] == kOutput; ++i) {
            run.output.emplace_back(as->lines[i]);
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
    auto str = StrFrom(line);
    seg.push_back('/');
    seg.append(digits10(info.lastProgLine) - size(str), '0');
    seg.append(str.view());
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
    auto path = Path(testPath(*info, prog.line)) / "script.txt";

    CharBuf content;
    genFileHeader(&content, path.str(), "; ");
    content.append("; From '")
        .append(info->page.file)
        .append("', line ")
        .append(StrFrom(test.runs.front().line).view())
        .append("\n");
    content.append(";\n");
    auto len = content.size();
    for (auto&& run : test.runs) {
        if (run.cmdline.empty())
            continue;
        content.append("; Line ")
            .append(StrFrom(run.line).view())
            .append("\n");
        for (auto&& cmt : run.comments)
            content.append("# ").append(cmt).append("\n");
        content.append("$ ").append(run.cmdline).append("\n");
        for (auto&& line : run.output) {
            if (line.input < line.text.size()) {
                content.append("< ")
                    .append(line.text.substr(line.input))
                    .pushBack('\n');
            }
            content.append("> ").append(line.text).pushBack('\n');
        }
    }
    if (len == content.size()) {
        // No runs were added
        content.append("; No script, compile only test\n");
    }
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
        ApplyAttrState as(blk, info->page.file);
        as.logger = [&info, &blk](TestAttr attr) {
            auto os = logMsgWarn();
            badAttrPrefix(os, *info, blk, attr);
            return os;
        };
        if (applyIgnoreAttr(&as))
            continue;
        applyLanguageAttr(&as);

        if (cfg.compilers.contains(as.lang)) {
            processCompiler(info, &ti, &as);
        } else if (cfg.scripts.contains(as.lang)) {
            processScript(info, &ti, &as);
        } else {
            logMsgWarn() << info->page.file << ", line " << blk.line
                << ": unknown language: '" << as.lang << "'";
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
                for (auto&& line : info->testPrefix[file.second.lang])
                    content.append(line.text).append("\n");
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
    for (auto i = 0; i < lines.size(); ++i) {
        if (i >= run.output.size())
            break;
        const auto & output = run.output[i];
        if (output.input < output.text.size()) {
            auto pos = output.input;
            auto line = lines[i];
            auto & withInput =
                tmp.emplace_back(line.substr(0, pos));
            withInput += output.text.substr(pos);
            lines.insert(
                lines.begin() + i + 1,
                line.substr(pos)
            );
            lines[i] = withInput;
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
            && run.output.back().text.empty()
        ) {
            // last output line blank, ignore it
            olen -= 1;
        }
    }
    auto len = min(lines.size(), olen);
    for (i = 0; i < len; ++i) {
        auto line = rtrim(lines[i]);
        if (run.output[i].text != line)
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
            logMsgInfo() << "- " << run.output[i].text;
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
        if (work->fn) {
            work->fn();
            work->fn = {};
        }
        if (--work->pendingWork == 0)
            delete work;
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
            s_perfCompile += 1;
            if (first) {
                first = false;
                auto out = execToolWait(
                    cmdline,
                    title,
                    { .workingDir = workDir.str() }
                );
                if (out.empty()) {
                    s_perfCompileFailed += 1;
                    appSignalShutdown(EX_DATAERR);
                    work->pendingWork = 1;
                    runProgTests(work, what);
                    return;
                }
                work->pendingWork -= 1;
            } else {
                execTool(
                    [work, what](string && out) {
                        if (out.empty())
                            s_perfCompileFailed += 1;
                        runProgTests(work, what);
                    },
                    cmdline,
                    title,
                    { .workingDir = workDir.str() }
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
            // Execute just the next test waiting run.
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
            for (auto&& line : run.output) {
                if (line.input < line.text.size())
                    input += line.text.substr(line.input);
                input += '\n';
            }

            s_perfRun += 1;
            execProgram(
                [work, phase, &run](ExecResult && res) {
                    runProgDone(work, phase, run, move(res));
                },
                cmd,
                {
                    .workingDir = workDir.str(),
                    .envVars = run.env,
                    .stdinData = input,
                    .concurrency = envProcessors()
                }
            );
            return;
        }

        work->fn();
        delete work;
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
        auto count = out->outputs.size();
        if (s_opts.update) {
            auto odir = Path(out->sampDir)
                .resolve(out->configFile.parentPath());
            if (s_opts.update && !writeOutputs(odir, out->outputs))
                return;
            logMsgInfo() << count << " files updated.";
            ConsoleScopedAttr ca(kConsoleNote);
            logMsgInfo() << "Tests generated.";
        } else {
            logMsgInfo() << count << " files calculated.";
            ConsoleScopedAttr ca(kConsoleNote);
            logMsgInfo() << "Tests calculated.";
        }

        // Compile and execute tests
        out->pendingWork = 1;
        if (s_opts.compile) {
            runTests(
                [out, what]() { testSamples(out, what); },
                s_pageInfos
            );
        }

        phase = what;
    }
    if (phase == what++) {
        if (--out->pendingWork) {
            // Still have more tests to process.
            return;
        }

        // Clean up
        delete out;

        auto errs = s_perfCompileFailed + s_perfRunFailed;
        if (s_opts.compile) {
            logMsgInfo() << s_perfCompile << " programs compiled.";
            logMsgInfo() << s_perfRun << " runs executed.";
            if (errs) {
                ConsoleScopedAttr ca(kConsoleError);
                logMsgInfo() << "Compile and run failures: " << errs;
            } else {
                ConsoleScopedAttr ca(kConsoleNote);
                logMsgInfo() << "Tests run successfully.";
            }
        }

        if (!errs) {
            ConsoleScopedAttr ca(kConsoleCheer);
            logMsgInfo() << "Completed successfully.";
        }
        logStopwatch();
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
    cli.opt(&cfgfile, "c conf")
        .desc("Site configuration to process. "
            "(default: {GIT_ROOT}/docs/docgen.xml)");
    cli.opt(&layout, "layout", "default")
        .desc("Layout with files to process.");

    cli.opt(&update, "update", true)
        .desc("Update files used to compile/run tests.");
    cli.opt(&compile, "compile", true)
        .desc("Compile and run tests, but only if update is also enabled.");

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

    ostringstream os;
    os << "Making TEST files from '" << cfg->configFile << "' to '"
        << cfg->sampDir << "'.";
    auto out = logMsgInfo();
    cli.printText(out, os.str());
    testSamples(cfg.release());

    return cli.fail(EX_PENDING, "");
}
