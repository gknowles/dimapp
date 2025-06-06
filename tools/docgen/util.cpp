// Copyright Glen Knowles 2020 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// util.cpp - docgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

constexpr TokenTable::Token s_fileTypes[] = {
    { (int) Page::kAsciidoc, ".adoc" },
    { (int) Page::kAsciidoc, ".asc" },
    { (int) Page::kAsciidoc, ".asciidoc" },
    { (int) Page::kMarkdown, ".md" },
    { (int) Page::kMarkdown, ".markdown" },
};
const TokenTable s_fileTypeTbl(s_fileTypes);


/****************************************************************************
*
*   Load configuration
*
***/

//===========================================================================
static bool loadVersions(Config * out, XNode * root) {
    for (auto && xver : elems(root, "Version")) {
        auto & ver = out->versions.emplace_back();
        ver.tag = attrValue(&xver, "tag", "");
        ver.name = attrValue(&xver, "name", ver.tag.c_str());
        ver.layout = attrValue(&xver, "layout", "");
        ver.defaultSource = attrValue(&xver, "default", false);
        if (ver.defaultSource) {
            if (out->defVersion != -1) {
                logMsgError() << "Multiple default versions for site.";
                return false;
            }
            out->defVersion = out->versions.size() - 1;
        }
        if (ver.tag.empty()) {
            logMsgError() << "Missing required Version/@tag attribute.";
            return false;
        }
    }
    if (out->versions.empty()) {
        logMsgError() << "No versions defined for site.";
        return false;
    }
    for (auto&& ver : out->versions) {
        auto ib = out->versionsByTag.try_emplace(ver.tag, &ver);
        if (!ib.second) {
            logMsgError() << "Multiple versions with same @tag attribute.";
            return false;
        }
    }
    if (out->defVersion == -1)
        out->defVersion = 0;
    return true;
}

//===========================================================================
static bool loadFiles(Config * out, XNode * root) {
    for (auto && xe : elems(root, "File")) {
        auto & file = out->files.emplace_back();
        file.file = attrValue(&xe, "file", "");
        file.tag = attrValue(&xe, "tag", "");
        file.url = attrValue(&xe, "url", "");
        if (file.tag.empty()) {
            logMsgError() << "Missing required File/@tag attribute.";
            return false;
        }
        if (file.file.empty()) {
            logMsgError() << "Missing required File/@file attribute.";
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadSpawn(Spawn * out, XNode * root) {
    for (auto&& elem : elems(root, "Arg"))
        out->args.emplace_back(attrValue(&elem, "value", ""));
    for (auto&& elem : elems(root, "Env")) {
        auto name = attrValue(&elem, "name");
        if (!name) {
            logMsgError() << "Missing required Env/@name attribute.";
            return false;
        }
        out->env[name] = attrValue(&elem, "value", "");
    }
    out->untrackedChildren = attrValue(root, "untracked", false);
    return true;
}

//===========================================================================
static bool loadScripts(Config * out, XNode * root) {
    for (auto&& xs : elems(root, "Script")) {
        auto script = make_shared<Script>();
        script->prefix =
            attrValue(firstChild(&xs, "ShellPrefix"), "value", "");
        script->commentPrefix =
            attrValue(firstChild(&xs, "CommentPrefix"), "value", "");
        for (auto&& xarg : elems(&xs, "SetEnv")) {
            auto raw = attrValue(&xarg, "regex");
            if (!raw)
                continue;
            auto & regex = script->envSets.emplace_back();
            regex.assign(raw, regex::optimize);
            if (regex.mark_count() != 2) {
                logMsgError() << "SetEnv/@regex must have two captures.";
                return false;
            }
        }

        if (!loadSpawn(&script->shell, firstChild(&xs, "Shell")))
            return false;
        if (script->shell.args.empty())
            continue;

        for (auto&& xlang : elems(&xs, "Lang")) {
            auto name = attrValue(&xlang, "name");
            if (!name) {
                logMsgError() << "Missing required Lang/@name attribute.";
                return false;
            }
            auto lname = toLower(string_view(name));
            out->scripts.emplace(lname, script);
        }
    }
    return true;
}

//===========================================================================
static bool loadCompilers(Config * out, XNode * root) {
    for (auto&& xs : elems(root, "Code")) {
        auto comp = make_shared<Compiler>();

        // Code/Compile
        if (!loadSpawn(&comp->compile, firstChild(&xs, "Compile")))
            return false;
        if (comp->compile.args.empty())
            continue;

        // Code/File
        comp->fileExt = attrValue(firstChild(&xs, "File"), "extension", "");

        // Code/Lang
        for (auto&& xlang : elems(&xs, "Lang")) {
            auto name = attrValue(&xlang, "name");
            if (!name) {
                logMsgError() << "Missing required Lang/@name attribute.";
                return false;
            }
            auto lname = toLower(string_view(name));
            if (out->scripts.contains(lname)) {
                logMsgError() << "Language '" << lname << "' can't be both "
                    "code and script.";
                return false;
            }
            out->compilers.emplace(lname, comp);
        }
    }
    return true;
}

//===========================================================================
static bool loadPage(Page * out, XNode * root) {
    out->name = attrValue(root, "name", "");
    out->file = attrValue(root, "file", "");
    out->type = s_fileTypeTbl.find(
        Path(out->file).extension(),
        Page::kUnknown
    );
    out->modes = {};
    if (attrValue(root, "site", true))
        out->modes |= fLoadSite;
    if (attrValue(root, "test", true))
        out->modes |= fLoadTests;
    out->xrefFile = attrValue(root, "xrefFile", out->file.c_str());
    out->urlSegment = attrValue(root, "url", "");
    if (out->urlSegment.empty())
        out->urlSegment = Path(out->file).stem();
    out->pageLayout = attrValue(root, "pageLayout", "default");
    out->patch = trimBlock(text(firstChild(root, "Patch"), ""));
    return true;
}

//===========================================================================
static bool loadLayouts(Config * out, XNode * root, LoadMode mode) {
    for (auto && xlay : elems(root, "Layout")) {
        Layout lay;
        lay.name = attrValue(&xlay, "name", "");
        for (auto && xpage : elems(&xlay, "Page")) {
            auto & pg = lay.pages.emplace_back();
            if (!loadPage(&pg, &xpage))
                return false;
            if (pg.defaultPage) {
                if (lay.defPage != -1) {
                    logMsgError() << "Multiple default pages for "
                        "Layout/@name = '" << lay.name << "'.";
                    return false;
                }
                lay.defPage = lay.pages.size() - 1;
            }
            if (!pg.modes.any(mode))
                lay.pages.pop_back();
        }
        if (lay.pages.empty()) {
            if (mode == fLoadSite) {
                logMsgError() << "No pages defined for "
                    "Layout/@name = '" << lay.name << "'.";
                return false;
            }
            continue;
        }
        if (lay.defPage == -1)
            lay.defPage = 0;
        if (!out->layouts.insert({lay.name, lay}).second) {
            logMsgError() << "Multiple definitions for "
                "Layout/@name = '" << lay.name << "'.";
            return false;
        }
    }
    return true;
}

constexpr TokenTable::Token s_contentTypes[] = {
    { Column::kContentToc, "toc" },
    { Column::kContentBody, "body" },
};
static const TokenTable s_contentTypeTbl(s_contentTypes);

//===========================================================================
static bool loadColumn(Column * out, XNode * root) {
    auto ctStr = attrValue(root, "content", "");
    out->content = s_contentTypeTbl.find(ctStr, Column::kContentInvalid);
    if (!out->content) {
        logMsgError() << "Invalid Page/Column/@content attribute";
        return false;
    }
    out->maxWidth = attrValue(root, "maxWidth", "");
    return true;
}

//===========================================================================
static bool loadPageLayouts(Config * out, XNode * root) {
    for (auto && xlayout : elems(root, "PageLayout")) {
        PageLayout layout;
        layout.name = attrValue(&xlayout, "name", "");
        for (auto && xcol : elems(&xlayout, "Column")) {
            auto & col = layout.columns.emplace_back();
            if (!loadColumn(&col, &xcol))
                return false;
        }
        if (!out->pageLayouts.insert({layout.name, layout}).second) {
            logMsgError() << "Multiple definitions for "
                "PageLayout/@name = '" << layout.name << "'";
            return false;
        }
    }
    return true;
}

//===========================================================================
unique_ptr<Config> loadConfig(
    string * content,
    string_view path,
    string_view gitRoot,
    LoadMode mode
) {
    XDocument doc;
    auto root = doc.parse(content->data(), path);
    if (!root || doc.errmsg()) {
        logParseError("Parsing failed", path, doc.errpos(), *content);
        return {};
    }
    auto out = make_unique<Config>();
    if (!loadPageLayouts(out.get(), root)
        || !loadLayouts(out.get(), root, mode)
    ) {
        return {};
    }

    // Site
    auto site = firstChild(root, "Site");
    if (!site) {
        logMsgError() << path << ": Missing Docs/Site";
        return {};
    }
    out->siteName = attrValue(site, "name", "");
    out->siteDir = attrValue(site, "outDir", "");
    out->siteFavicon = attrValue(site, "favicon", "");
    if (auto github = firstChild(site, "GitHub")) {
        out->github = true;
        out->repoUrl = attrValue(github, "repoUrl", "");
    }
    if (!loadVersions(out.get(), site)
        || !loadFiles(out.get(), site)
    ) {
        return {};
    }

    // Samples
    if (auto samp = firstChild(root, "Samples")) {
        out->sampDir = attrValue(samp, "outDir", "");
        if (!loadScripts(out.get(), samp)
            || !loadCompilers(out.get(), samp)
        ) {
            return {};
        }
    }

    if (auto ec = fileAbsolutePath(&out->configFile, path); ec)
        return {};
    out->gitRoot = gitRoot;
    return out;
}

//===========================================================================
unique_ptr<Config> loadConfig(string_view cfgfile, LoadMode mode) {
    string configFile;
    string gitRoot;
    string content;
    if (!gitLoadConfig(
        &content,
        &configFile,
        &gitRoot,
        cfgfile,
        "docs/docgen.xml"
    )) {
        return {};
    }
    return loadConfig(&content, configFile, gitRoot, mode);
}


/****************************************************************************
*
*   Load and write content
*
***/

//===========================================================================
void writeContent(
    function<void()> fn,
    string_view path,
    string_view content
) {
    struct Write : IFileWriteNotify {
        function<void()> fn;
        string path;

        void onFileWrite(const FileWriteData & data) override {
            if (data.written != data.data.size()) {
                logMsgError() << path << ": error writing file.";
                appSignalShutdown(EX_IOERR);
            }
            fn();
            delete this;
        }
    };

    auto notify = new Write;
    notify->fn = fn;
    notify->path = path;
    fileSaveBinary(notify, path, content);
}

//===========================================================================
void loadContent(
    function<void(string&&)> fn,
    const Config & site,
    string_view tag,
    string_view file
) {
    auto configDir = site.configFile.parentPath();
    auto path = Path(file).resolve(configDir);
    if (tag == "HEAD") {
        struct FileContent : IFileReadNotify {
            function<void(string&&)> fn;
            string buf;
            string tag;
            string path;

            bool onFileRead(
                size_t * bytesUsed,
                const FileReadData & data
            ) override {
                if (data.ec) {
                    logMsgError() << path << '@' << tag
                        << ": error reading file.";
                    appSignalShutdown(EX_IOERR);
                }
                fn(move(buf));
                delete this;
                return false;
            }
        };
        auto notify = new FileContent;
        notify->fn = fn;
        notify->tag = tag;
        notify->path = path;
        fileLoadBinary(notify, &notify->buf, path);
        return;
    }

    string objname = path.str();
    objname.replace(0, site.gitRoot.size() + 1, string(tag) + ":");

    auto cmdline = Cli::toCmdlineL(
        "git",
        "-C",
        configDir,
        "show",
        objname
    );
    execTool(
        [fn](auto && res) {
            fn(move(res.output));
        },
        cmdline,
        objname
    );
}

//===========================================================================
bool addOutput(
    Config * out,
    const Path & file,
    CharBuf && content,
    bool forceCRLF
) {
    auto & fname = file.str();

    CharBuf buf;
    if (!forceCRLF) {
        buf = move(content);
    } else {
        bool hasCR = false;
        for (auto&& v : content.views()) {
            for (unsigned i = 0; i < v.size(); ++i) {
                if (v[i] == '\r') {
                    if (hasCR)
                        buf += '\n';
                    buf += '\r';
                    hasCR = true;
                } else if (v[i] == '\n') {
                    if (!hasCR)
                        buf += '\r';
                    buf += '\n';
                    hasCR = false;
                } else {
                    if (hasCR) {
                        buf += '\n';
                        hasCR = false;
                    }
                    buf += v[i];
                }
            }
        }
        if (hasCR)
            buf += '\n';
    }

    static mutex s_mut;
    scoped_lock lock(s_mut);
    if (out->outputs.contains(fname)) {
        logMsgError() << fname << ": multiply defined";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    out->outputs[fname] = move(buf);
    return true;
}

//===========================================================================
bool writeOutputs(
    string_view odir,
    const unordered_map<string, CharBuf> & files
) {
    if (auto ec = fileCreateDirs(odir); ec) {
        logMsgError() << odir << ": unable to create directory.";
        appSignalShutdown(EX_IOERR);
        return false;
    }
    for (auto&& f : fileGlob(odir, "**", Glob::fDirsLast)) {
        if (auto ec = fileRemove(f.path); ec) {
            if (!f.isdir) {
                appSignalShutdown(EX_IOERR);
                return false;
            }
        }
    }
    for (auto&& output : files) {
        using enum File::OpenMode;

        auto path = Path(output.first).resolve(odir);
        fileCreateDirs(path.parentPath());
        FileHandle file;
        auto ec = fileOpen(
            &file,
            path,
            fCreat | fExcl | fReadWrite | fBlocking
        );
        if (ec) {
            logMsgError() << path << ": unable to create.";
            appSignalShutdown(EX_IOERR);
            return false;
        }
        for (auto&& data : output.second.views()) {
            if (fileAppendWait(nullptr, file, data.data(), data.size())) {
                logMsgError() << path << ": unable to write.";
                appSignalShutdown(EX_IOERR);
                fileClose(file);
                return false;
            }
        }
        fileClose(file);
    }
    return true;
}
