// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// docgen.cpp - docgen
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
}


/****************************************************************************
*
*   Variables
*
***/

static TimePoint s_startTime;


/****************************************************************************
*
*   Load site configuration
*
***/

//===========================================================================
static bool loadSources(Site * out, XNode * root) {
    for (auto && xsrc : elems(root, "Source")) {
        auto & src = out->sources.emplace_back();
        src.tag = attrValue(&xsrc, "tag", "");
        src.name = attrValue(&xsrc, "name", src.tag.c_str());
        src.layout = attrValue(&xsrc, "layout", "");
        src.defaultSource = attrValue(&xsrc, "default", false);
        if (src.defaultSource) {
            if (out->defSource != -1) {
                logMsgError() << "Multiple default sources for site.";
                appSignalShutdown(EX_DATAERR);
                return false;
            }
            out->defSource = out->sources.size() - 1;
        }
        if (src.tag.empty()) {
            logMsgError() << "Missing required Source/@tag attribute.";
            appSignalShutdown(EX_DATAERR);
            return false;
        }
    }
    if (out->sources.empty()) {
        logMsgError() << "No sources defined for site.";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    if (out->defSource == -1)
        out->defSource = 0;
    return true;
}

//===========================================================================
static bool loadPage(Page * out, XNode * root) {
    out->name = attrValue(root, "name", "");
    out->file = attrValue(root, "file", "");
    out->xrefFile = attrValue(root, "xrefFile", out->file.c_str());
    out->urlSegment = attrValue(root, "url", "");
    if (out->urlSegment.empty())
        out->urlSegment = Path(out->file).stem();
    out->pageLayout = attrValue(root, "pageLayout", "");
    return true;
}

//===========================================================================
static bool loadLayouts(Site * out, XNode * root) {
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
                    appSignalShutdown(EX_DATAERR);
                    return false;
                }
                lay.defPage = lay.pages.size() - 1;
            }
        }
        if (lay.pages.empty()) {
            logMsgError() << "No pages defined for "
                "Layout/@name = '" << lay.name << "'.";
            appSignalShutdown(EX_DATAERR);
            return false;
        }
        if (lay.defPage == -1)
            lay.defPage = 0;
        if (!out->layouts.insert({lay.name, lay}).second) {
            logMsgError() << "Multiple definitions for "
                "Layout/@name = '" << lay.name << "'.";
            appSignalShutdown(EX_DATAERR);
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
    out->content = tokenTableGetEnum(
        s_contentTypeTbl,
        ctStr,
        Column::kContentInvalid
    );
    if (!out->content) {
        logMsgError() << "Invalid Page/Column/@content attribute";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    out->maxWidth = attrValue(root, "maxWidth", "");
    return true;
}

//===========================================================================
static bool loadPageLayouts(Site * out, XNode * root) {
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
            appSignalShutdown(EX_DATAERR);
            return false;
        }
    }
    return true;
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char * argv[]) {
    Cli cli;
    cli.header("docgen v"s + kVersion + " (" __DATE__ ")")
        .helpNoArgs()
        .helpCmd()
        .versionOpt(kVersion, "docgen");

    s_startTime = Clock::now();
    (void) cli.exec(argc, argv);
    appSignalUsageError();
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    return appRun(app, argc, argv);
}

//===========================================================================
unique_ptr<Site> loadSite(string * content, string_view path) {
    XDocument doc;
    auto root = doc.parse(content->data(), path);
    if (!root || doc.errmsg()) {
        logParseError("Parsing failed", path, doc.errpos(), *content);
        appSignalShutdown(EX_DATAERR);
        return {};
    }

    auto out = make_unique<Site>();
    auto e = firstChild(root, "Site");
    if (!e) {
        logMsgError() << path << ": Missing Docs/Site";
        appSignalShutdown(EX_DATAERR);
        return {};
    }
    out->name = attrValue(e, "name", "");
    out->outDir = attrValue(e, "outDir", "");
    if (auto github = firstChild(e, "GitHub")) {
        out->github = true;
        out->repoUrl = attrValue(github, "repoUrl", "");
    }
    if (!loadPageLayouts(out.get(), root)
        || !loadLayouts(out.get(), root)
        || !loadSources(out.get(), e)
    ) {
        return {};
    }
    return out;
}

//===========================================================================
unique_ptr<Site> loadSite(const string & specfile) {
    string content;
    if (!fileLoadBinaryWait(&content, specfile)) {
        appSignalUsageError(EX_DATAERR);
        return {};
    }

    auto site = loadSite(&content, specfile);
    if (!site)
        return {};

    site->configFile = fileAbsolutePath(specfile);
    return site;
}

//===========================================================================
bool addOutput(
    Site * out,
    const string & file,
    CharBuf && content,
    bool normalize
) {
    if (out->outputs.contains(file)) {
        logMsgError() << file << ": multiply defined";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    if (!normalize) {
        out->outputs[file] = move(content);
    } else {
        auto& buf = out->outputs[file];
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
    return true;
}

//===========================================================================
void exec(
    function<void(string&&)> fn,
    string_view cmdline,
    string_view errTitle
) {
    struct Exec : IExecNotify {
        function<void(string&&)> fn;
        string cmdline;
        string errTitle;

        void onExecComplete(bool canceled, int exitCode) override {
            if (exitCode) {
                logMsgError() << "Error: " << errTitle;
                logMsgError() << " - " << cmdline;
                logMsgError() << " - " << m_err;
                appSignalShutdown(EX_IOERR);
            } else {
                if (m_err) {
                    logMsgWarn() << "Warn: " << errTitle;
                    logMsgWarn() << " - " << cmdline;
                    logMsgWarn() << " - " << m_err;
                }
                fn(toString(m_out));
            }
            delete this;
        }
    };
    auto notify = new Exec;
    notify->fn = fn;
    notify->cmdline = cmdline;
    notify->errTitle = errTitle;
    execProgram(notify, notify->cmdline);
}

//===========================================================================
void writeContent(
    function<void()> fn,
    string_view path,
    string_view content
) {
    struct Write : IFileWriteNotify {
        function<void()> fn;
        string path;

        void onFileWrite(
            int written,
            std::string_view data,
            int64_t offset,
            FileHandle f
        ) override {
            if (written != data.size()) {
                logMsgError() << path << ": error writing file.";
                appSignalShutdown(EX_IOERR);
            } else {
                fn();
            }
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
    const Site & site,
    string_view tag,
    string_view file
) {
    auto path = Path(file).resolve(site.configFile.parentPath());
    if (tag == "HEAD") {
        struct FileContent : IFileReadNotify {
            function<void(string&&)> fn;
            string buf;

            bool onFileRead(
                size_t * bytesUsed,
                std::string_view data,
                bool more,
                int64_t offset,
                FileHandle f
            ) override {
                fn(move(buf));
                delete this;
                return false;
            }
        };
        auto notify = new FileContent;
        notify->fn = fn;
        fileLoadBinary(notify, &notify->buf, path);
        return;
    }

    string objname = path.str();
    objname.replace(0, site.gitRoot.size() + 1, string(tag) + ":");

    auto cmdline = Cli::toCmdlineL(
        "git",
        "-C",
        site.configFile.parentPath(),
        "show",
        objname
    );
    exec(fn, cmdline, objname);
}

//===========================================================================
bool writeOutputs(const Site & out) {
    auto odir = Path(out.outDir).resolve(out.configFile.parentPath());
    if (!fileCreateDirs(odir)) {
        logMsgError() << odir << ": unable to create directory.";
        appSignalShutdown(EX_IOERR);
        return false;
    }
    for (auto&& f : FileIter(odir, "*.*", FileIter::fDirsLast)) {
        if (!strstr(f.path.c_str(), "/.git")) {
            if (!fileRemove(f.path)) {
                logMsgError() << f.path << ": unable to remove.";
                appSignalShutdown(EX_IOERR);
                return false;
            }
        }
    }
    for (auto&& output : out.outputs) {
        auto path = Path(output.first).resolve(odir);
        fileCreateDirs(path.parentPath());
        auto f = fileOpen(path, File::fCreat | File::fExcl | File::fReadWrite);
        if (!f) {
            logMsgError() << path << ": unable to create.";
            appSignalShutdown(EX_IOERR);
            return false;
        }
        for (auto&& data : output.second.views()) {
            if (!fileAppendWait(f, data.data(), data.size())) {
                logMsgError() << path << ": unable to write.";
                appSignalShutdown(EX_IOERR);
                fileClose(f);
                return false;
            }
        }
        fileClose(f);
    }
    return true;
}
