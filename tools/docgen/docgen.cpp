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

struct GenPageInfo {
    Site * out;
    const Source & src;
    const Page & page;
    function<void()> fn;

    string fname;
    string content;
};

}


/****************************************************************************
*
*   Variables
*
***/

TimePoint s_startTime;


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

//===========================================================================
static bool loadSite(Site * out, string * content, string_view path) {
    XDocument doc;
    auto root = doc.parse(content->data(), path);
    if (!root || doc.errmsg()) {
        logParseError("Parsing failed", path, doc.errpos(), *content);
        appSignalShutdown(EX_DATAERR);
        return false;
    }

    *out = {};
    auto e = firstChild(root, "Site");
    if (!e) {
        logMsgError() << path << ": Missing Docs/Site";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    out->name = attrValue(e, "name", "");
    out->outDir = attrValue(e, "outDir", "");
    if (auto github = firstChild(e, "GitHub")) {
        out->github = true;
        out->repoUrl = attrValue(github, "repoUrl", "");
    }
    return loadPageLayouts(out, root)
        && loadLayouts(out, root)
        && loadSources(out, e);
}


/****************************************************************************
*
*   Generate site files
*
***/

//===========================================================================
static bool addOutput(
    Site * out,
    const string & file,
    CharBuf && content,
    bool normalize = true
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
string genAutoId(string_view title) {
    constexpr unsigned kStartChar = 1;
    constexpr unsigned kChar = 2;

    struct CharDefs {
        uint8_t flags[256];
    };
    static const CharDefs s_defs = [](){
        CharDefs out = {};
        for (unsigned char ch
            : "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"
        ) {
            out.flags[ch] |= kStartChar | kChar;
        }
        for (unsigned char ch : "0123456789") {
            out.flags[ch] |= kChar;
        }
        return out;
    }();

    string out;
    if (title.empty())
        return out;

    bool multisep = false;
    if (~s_defs.flags[(unsigned char) title[0]] & kStartChar) {
        out += '-';
        multisep = true;
    }
    for (unsigned char ch : title) {
        if (~s_defs.flags[ch] & kChar) {
            if (!multisep) {
                out += '-';
                multisep = true;
            }
            continue;
        }
        out += ch;
        multisep = false;
    }
    toLower(out.data());
    return out;
}

//===========================================================================
static vector<TocEntry> createToc(string * content) {
    static const std::regex s_toc_entry(
        //           depth            id        text
        R"regex(<[hH]([1-6])(?:|\sid="([^"]+)")>(.*)</[hH]\1>)regex"
    );

    unordered_map<string, int> ids;
    vector<TocEntry> out;
    cmatch m;
    size_t pos = 0;
    int h1Count = 0;
    for (;;) {
        if (!regex_search(content->data() + pos, m, s_toc_entry))
            break;
        auto & te = out.emplace_back();
        te.pos = pos + m.position();
        te.depth = content->data()[pos + m.position(1)] - '0';
        if (te.depth == 1)
            h1Count += 1;
        te.name = m.str(3);

        if (m[2].matched) {
            te.link = genAutoId(m.str(2));
        } else {
            te.link = genAutoId(te.name);
        }
        auto num = ids[te.link] += 1;
        if (num > 1) {
            te.link += "--";
            te.link += StrFrom<int>(num);
        }
        if (m[2].matched) {
            auto olen = m.length(2);
            content->replace(pos + m.position(2), olen, te.link);
            pos += te.link.size() - olen;
        } else {
            auto tmp = string(" id=\"") + te.link + '"';
            content->insert(pos + m.position(1) + 1, tmp);
            pos += tmp.size();
        }

        pos += m.position() + m.length();
    }

    // If there's only one H1 and it's the first H*, remove it from the TOC
    // and increase the bump up all other headers by one.
    if (h1Count == 1 && out[0].depth == 1) {
        out.erase(out.begin());
        for (auto&& te : out)
            te.depth -= 1;
    }

    return out;
}

//===========================================================================
static void updateXrefLinks(string * content, const Layout & layout) {
    static const std::regex s_xref_link(
        //           depth            id        text
        R"regex(<a href="([^#"]+)[^"]*">)regex"
    );

    unordered_map<string_view, string> mapping;
    for (auto&& page : layout.pages) {
        mapping[page.xrefFile] = page.urlSegment + ".html";
    }

    cmatch m;
    size_t pos = 0;
    for (;;) {
        if (!regex_search(content->data() + pos, m, s_xref_link))
            break;
        if (m[1].matched) {
            auto ref = m.str(1);
            auto map = mapping.find(ref);
            if (map != mapping.end()) {
                content->replace(pos + m.position(1), ref.size(), map->second);
                pos += map->second.size() - ref.size();
            }
        }
        pos += m.position() + m.length();
    }
}

//===========================================================================
static void exec(
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
static void writeContent(
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
static void loadContent(
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
static void addFontAwesomeHead(IXBuilder * out) {
    auto & bld = *out;
    bld.start("link")
        .attr("rel", "stylesheet")
        .attr("href", "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/"
            "4.7.0/css/font-awesome.min.css")
        .end();
}

//===========================================================================
static void addHighlightJsHead(IXBuilder * out) {
    auto & bld = *out;
    bld.start("link")
        .attr("rel", "stylesheet")
        .attr("href", "https://cdnjs.cloudflare.com/ajax/libs/"
            "highlight.js/10.1.2/styles/default.min.css")
        .end();
    bld.start("script")
        .attr("src", "https://cdnjs.cloudflare.com/ajax/libs/"
            "highlight.js/10.1.2/highlight.min.js")
        .text("")
        .end();
    bld.start("script")
        .text(1 + R"(
document.addEventListener('DOMContentLoaded', (event) => {
  document.querySelectorAll('pre code').forEach((block) => {
    try {
        var lang = block.parentElement.lang.toLowerCase();
        if (lang == 'shell session') {
            lang = 'console';
        }
        var code = hljs.highlight(lang, block.textContent);
        block.className += 'hljs';
        block.innerHTML = code.value;
    } catch (e) {
    }
  });
});
)")
        .end();
}

//===========================================================================
static void addBootstrapHead(IXBuilder * out) {
    auto & bld = *out;
    bld.start("meta")
        .attr("name", "viewport")
        .attr("content", "width=device-width, initial-scale=1, "
            "shrink-to-fit=no")
        .end();
    bld.start("link")
        .attr("rel", "stylesheet")
        .attr("href", "https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/"
            "css/bootstrap.min.css")
        .attr("integrity", "sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9H"
            "uZOnIxN0hoP+VmmDGMN5t9UJ0Z")
        .attr("crossorigin", "anonymous")
        .end();
    return;
}

//===========================================================================
static void addBootstrapBody(IXBuilder * out) {
    auto & bld = *out;
    bld.start("script")
        .attr("src", "https://code.jquery.com/jquery-3.5.1.slim.min.js")
        .attr("integrity", "sha384-DfXdz2htPH0lsSSs5nCTpuj/zy4C+OGpamoFVy38M"
            "VBnE+IbbVYUew+OrCXaRkfj")
        .attr("crossorigin", "anonymous")
        .text("")
        .end();
    bld.start("script")
        .attr("src", "https://cdn.jsdelivr.net/npm/popper.js@1.16.1/dist/umd/"
            "popper.min.js")
        .attr("integrity", "sha384-9/reFTGAW83EW2RDu2S0VKaIzap3H66lZH81PoYlF"
            "hbGU+6BZp6G7niu735Sk7lN")
        .attr("crossorigin", "anonymous")
        .text("")
        .end();
    bld.start("script")
        .attr("src", "https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/"
            "bootstrap.min.js")
        .attr("integrity", "sha384-B4gt1jrGC7Jh4AgTPSdUtOBvfO8shuf57BaghqFfP"
            "lYxofvL8/KUEfYiJOMMV+rV")
        .attr("crossorigin", "anonymous")
        .text("")
        .end();
    return;
}

//===========================================================================
static void addBadgeOld(IXBuilder * out) {
    out->text(" ")
        .start("span")
            .attr("class", "badge badge-secondary")
            .text("Old")
            .end();
}

//===========================================================================
static void addNavbar(
    IXBuilder * out,
    const Site & site,
    const Source & source,
    const Layout & layout,
    const Page & page
) {
    const char kBgClass[] = "bg-primary";
    const enum { kDark, kLight } kSchemaType = kDark;

    auto & bld = *out;
    bld.start("nav")
        .startAttr("class")
            .text("navbar navbar-expand-sm sticky-top ")
            .text(kBgClass).text(" ")
            .text(kSchemaType == kDark ? "navbar-dark" : "navbar-light")
            .endAttr()
        .start("a")
            .attr("class", "navbar-brand font-weight-bolder")
            .attr("href", "..")
            .text(site.name)
            .end()
        .start("button")
            .attr("class", "navbar-toggler")
            .attr("type", "button")
            .attr("data-toggle", "collapse")
            .attr("data-target", "#navbarNavAltMarkup")
            .attr("aria-controls", "navbarNavAltMarkup")
            .attr("aria-expanded", "false")
            .attr("aria-label", "Toggle navigation")
            .start("span")
                .attr("class", "navbar-toggler-icon")
                .text("").end()
            .end()
        .start("div")
            .attr("class", "collapse navbar-collapse")
            .attr("id", "navbarNavAltMarkup");

    // Page from layout
    bld.start("div")
        .attr("class", "navbar-nav mr-auto");
    for (auto&& pg : layout.pages) {
        if (pg.urlSegment != page.urlSegment) {
            bld.start("a")
                .attr("class", "nav-link")
                .attr("href", pg.urlSegment + ".html")
                .text(pg.name)
                .end();
        } else {
            bld.start("a")
                .attr("class", "nav-link active")
                .attr("href", pg.urlSegment + ".html")
                .text(pg.name)
                .text(" ")
                .start("span")
                    .attr("class", "sr-only")
                    .text("(current)")
                    .end()
                .end();
        }
    }
    bld.end(); // div.navbar-nav

    // Version links
    bool afterDefault = false;
    for (auto&& src : site.sources) {
        if (src.tag == source.tag)
            break;
        if (src.defaultSource)
            afterDefault = true;
    }
    bld.start("div")
        .attr("class", "navbar-nav")
        .start("div")
            .attr("class", "nav-item dropdown")
            .start("a")
                .attr("class", "nav-link dropdown-toggle")
                .attr("href", "#")
                .attr("id", "navbarDropdown")
                .attr("role", "button")
                .attr("data-toggle", "dropdown")
                .attr("data-reference", "parent")
                .attr("aria-haspopup", "true")
                .attr("aria-expanded", "false")
                .attr("title", "Version")
                .text(source.name);
    if (afterDefault)
        addBadgeOld(&bld);
    bld.end()
        .start("div")
            .attr("class", "dropdown-menu dropdown-menu-md-right")
            .attr("aria-labelledby", "navbarDropdown");
    afterDefault = false;
    for (auto&& src : site.sources) {
        bld.start("a");
        if (src.tag != source.tag) {
            bld.attr("class", "dropdown-item");
        } else {
            bld.attr("class", "dropdown-item active");
        }
        if (src.urlSegments.contains(page.urlSegment)) {
            bld.attr("href", "../" + src.tag + "/" + page.urlSegment + ".html");
        } else {
            bld.attr("href", "../" + src.tag + "/index.html");
        }
        bld.text(src.name);
        if (afterDefault)
            addBadgeOld(&bld);
        bld.end();
        if (src.defaultSource)
            afterDefault = true;
    }
    bld.end() // div.dropdown-menu
        .end(); // div.nav-item

    // GitHub link
    if (site.github) {
        bld.start("a")
            .attr("class", "nav-link py-0 px-3")
            .attr("href", site.repoUrl)
            .attr("style", "font-size: 24px;")
            .start("i")
                .attr("class", "fa fa-github")
                .attr("title", "View Source")
                .text("").end()
            .end();
    }

    bld.end(); // div.navbar-nav
    bld.end(); // div#navbarNavAltMarkup
    bld.end(); // nav
}

//===========================================================================
static void addToc(IXBuilder * out, const vector<TocEntry> & entries) {
    auto & bld = *out;
    bld.start("div")
        .attr("class", "d-none d-lg-block col-lg-auto")
        .attr("style", "margin-top: 1.25rem;")
        .start("nav")
            .attr(
                "class",
                "nav sticky-top d-print-none d-none d-lg-flex flex-column"
                )
            .attr("id", "toc")
            .start("div");

    for (auto&& ent : entries) {
        if (ent.depth >= 1 && ent.depth <= 3) {
            bld.start("a")
                << attr("class") << "nav-link toc-" << ent.depth << endAttr
                << attr("href") << "#" << ent.link << endAttr
                << "";
            bld.addRaw(ent.name);
            bld.end();
        }
    }

    bld.end() // div.bg-light
        .end() // nav
        .end(); // div.col-md-2
}

//===========================================================================
static CharBuf processPageContent(
    GenPageInfo * info,
    string && content
) {
    auto spec = info->src.spec ? info->src.spec.get() : info->out;
    string layname = info->src.layout.empty() ? "default" : info->src.layout;
    auto & layout = spec->layouts.find(layname)->second;
    auto pglayname = info->page.pageLayout.empty()
        ? "default"s
        : info->page.pageLayout;
    auto & pglay = spec->pageLayouts.find(pglayname)->second;

    updateXrefLinks(&content, layout);
    auto toc = createToc(&content);

    CharBuf html;
    html.append("<!doctype html>\n");
    XBuilder bld(&html);
    bld.start("html")
        .attr("lang", "en")
        .start("head")
        .start("meta").attr("charset", "utf-8").end();
    addBootstrapHead(&bld);
    addFontAwesomeHead(&bld);
    addHighlightJsHead(&bld);
    bld.start("link")
        .attr("rel", "stylesheet")
        .attr("href", "../css/docgen.css")
        .end();
    bld.elem("title", info->page.name + " - " + info->out->name);
    bld.end(); // </head>

    bld.start("body")
        .attr("data-spy", "scroll")
        .attr("data-target", "#toc")
        .text("\n");
    addNavbar(&bld, *info->out, info->src, layout, info->page);
    bld.start("div")
        .attr("class", "container")
        .start("div")
            .attr("class", "row flex-nowrap");
    for (auto&& col : pglay.columns) {
        switch (col.content) {
        case Column::kContentInvalid:
            assert(!"INTERNAL: Invalid column content");
            break;
        case Column::kContentToc:
            addToc(&bld, toc);
            break;
        case Column::kContentBody:
            bld.start("div")
                .attr(
                    "class",
                    "col col-lg-9 table table-sm table-striped table-hover"
                    )
                .attr("role", "main")
                .attr("style", "margin-top: 1rem;")
                .text("");
            html.append(content);
            bld.end();
            break;
        }
    }
    bld.end(); // div.row
    bld.end(); // div.container-fluid

    addBootstrapBody(&bld);
    bld.end() // </body>
        .end(); // </html>
    return html;
}

//===========================================================================
static void genPage(GenPageInfo * info, unsigned phase = 0) {
    auto spec = info->src.spec ? info->src.spec.get() : info->out;
    string layname = info->src.layout.empty() ? "default" : info->src.layout;

    unsigned what = 0;
    if (phase == what++) {
        auto pglayname = info->page.pageLayout.empty()
            ? "default"s
            : info->page.pageLayout;
        auto pglay = spec->pageLayouts.find(pglayname);
        if (pglay == spec->pageLayouts.end()) {
            logMsgError() << "Tag '" << info->src.tag << "': page layout '"
                << pglayname << "' not defined.";
            appSignalShutdown(EX_DATAERR);
            return;
        }
        loadContent(
            [info, what](auto && content) {
                info->content = move(content);
                genPage(info, what);
            },
            *info->out,
            info->src.tag,
            info->page.file
        );
        return;
    }
    if (phase == what++) {
        if (info->content.empty()) {
            logMsgError() << info->page.file << ", tag '" << info->src.tag
                << "': unable to load content";
            appSignalShutdown(EX_DATAERR);
            return;
        }
        auto p = Path(info->page.file);
        p = fileTempName(p.extension());
        info->fname = p.str();
        writeContent(
            [info, what]() { genPage(info, what); },
            p,
            info->content
        );
        return;
    }
    if (phase == what++) {
        auto cmdline = Cli::toCmdlineL("github-markup.bat", info->fname);
        exec(
            [info, what](string && out) {
                fileRemove(info->fname);
                info->content = move(out);
                if (info->content.empty()) {
                    logMsgError() << info->page.file << ", tag '"
                        << info->src.tag << "': unable to load content";
                    appSignalShutdown(EX_DATAERR);
                } else {
                    genPage(info, what);
                }
            },
            cmdline,
            info->page.file + ", tag '" + info->src.tag + "'"
        );
        return;
    }
    if (phase == what++) {
        auto html = processPageContent(info, move(info->content));
        auto file = Path(info->src.tag) / info->page.urlSegment + ".html";
        if (!addOutput(info->out, file.str(), move(html)))
            return;
        info->fn();
        return;
    }

    assert(!"unknown phase");
}

//===========================================================================
static bool genStatics(Site * out) {
    CharBuf content;

    //-----------------------------------------------------------------------
    // .nojekyll
    content.clear();
    if (!addOutput(out, ".nojekyll", move(content), false))
        return false;

    //-----------------------------------------------------------------------
    // css/docgen.css
    content.assign(1 + R"(
body {
    position: relative;
}

nav.navbar {
    padding: .125rem 1rem;
}
nav.navbar-dark .navbar-nav .nav-link {
    color: white;
}
nav.navbar .navbar-nav .nav-link:hover,
nav.navbar .navbar-nav .nav-link.active {
    background-color: steelblue;
    color: white;
}
nav.navbar span.badge {
    margin-left: .25rem;
}
nav.navbar .dropdown-menu {
    min-width: unset;
}
nav.navbar .dropdown-menu .dropdown-item:hover {
    background-color: steelblue;
    color: white;
}

nav#toc {
    top: 4rem;
    height: calc(100vh - 5rem);
    overflow-y: auto;
}
nav#toc div {
    border-right: 1px solid #eee;
    border-left: 1px solid #eee;
}
nav#toc .nav-link {
    padding-top: 0;
    padding-bottom: 0;
    font-size: .875em;
}
nav#toc .nav-link:hover {
    background-color: steelblue;
    color: white;
}
nav#toc .nav-link.active {
    color: black;
}
nav#toc .nav-link.toc-2 {
    padding-left: 2rem;
}
nav#toc .nav-link.toc-3 {
    padding-left: 3rem;
}

h1[id]::before,
h2[id]::before,
h3[id]::before,
h4[id]::before,
h5[id]::before,
h6[id]::before {
    display: block;
    height: calc(3rem + 1rem);
    margin-top: -4rem;
    visibility: hidden;
    content: "";
}
ul p {
    margin-bottom: 0;
}
table {
    margin-bottom: 1rem;
}
table p {
    margin-bottom: 0;
}
div.scrollable-x {
    overflow-x: auto;
    margin-bottom: 1rem;
}
div.scrollable-x table {
    margin-bottom: 0;
}
table.smaller-td-font td {
    font-size: smaller;
}
)");
    if (!addOutput(out, "css/docgen.css", move(content)))
        return false;

    return true;
}

//===========================================================================
static bool genRedirect(
    Site * out,
    string_view outputFile,
    string_view targetUrl
) {
    CharBuf html;
    html.append("<!doctype html>\n");
    XBuilder bld(&html);
    bld << start("html")
        << start("head")
            << start("meta") << attr("charset", "utf-8") << end
            << start("meta")
                << attr("http-equiv", "refresh")
                << attr("content")
                    << "0; url='" << targetUrl << "'" << endAttr
                << end
            << end
        << end;
    return addOutput(out, string(outputFile), move(html));
}

//===========================================================================
static bool writeSite(Site * out) {
    auto odir = Path(out->outDir).resolve(out->configFile.parentPath());
    if (!fileCreateDirs(odir)) {
        logMsgError() << odir << ": unable to create directory.";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    for (auto&& f : FileIter(odir, "*.*", FileIter::fDirsLast)) {
        if (!strstr(f.path.c_str(), "/.git")) {
            if (!fileRemove(f.path)) {
                logMsgError() << f.path << ": unable to remove.";
                appSignalShutdown(EX_DATAERR);
                return false;
            }
        }
    }
    for (auto&& output : out->outputs) {
        auto path = Path(output.first).resolve(odir);
        fileCreateDirs(path.parentPath());
        auto f = fileOpen(path, File::fCreat | File::fExcl | File::fReadWrite);
        if (!f) {
            logMsgError() << path << ": unable to create.";
            appSignalShutdown(EX_DATAERR);
            return false;
        }
        for (auto&& data : output.second.views()) {
            if (!fileAppendWait(f, data.data(), data.size())) {
                logMsgError() << path << ": unable to write.";
                appSignalShutdown(EX_DATAERR);
                fileClose(f);
                return false;
            }
        }
        fileClose(f);
    }
    return true;
}

//===========================================================================
static void genSite(Site * out, unsigned phase = 0) {
    unsigned what = 0;
    if (phase == what++) {
        out->outputs.clear();
        auto cmdline = Cli::toCmdlineL(
            "git",
            "-C",
            out->configFile.parentPath(),
            "rev-parse",
            "--show-toplevel"
        );
        exec(
            [out, what](auto && content) {
                out->gitRoot = Path(trim(content));
                genSite(out, what);
            },
            cmdline,
            out->configFile
        );
        return;
    }
    if (phase == what++) {
        if (!genStatics(out))
            return;
        if (!genRedirect(
            out,
            "index.html",
            out->sources[out->defSource].tag + "/index.html"
        )) {
            return;
        }

        out->pendingWork = (unsigned) out->sources.size();
        for (auto && src : out->sources) {
            auto layname = src.layout;

            if (!layname.empty()) {
                out->pendingWork -= 1;
            } else {
                loadContent(
                    [out, &src, what](auto && content) {
                        src.spec = make_unique<Site>();
                        if (loadSite(
                            src.spec.get(),
                            &content,
                            out->configFile.filename()
                        )) {
                            genSite(out, what);
                        }
                    },
                    *out,
                    src.tag,
                    out->configFile.filename()
                );
            }
        }

        out->pendingWork += 1;
        phase = what;
    }
    if (phase == what++) {
        if (--out->pendingWork) {
            // Still have more layouts to load.
            return;
        }

        for (auto && src : out->sources) {
            auto spec = src.spec ? src.spec.get() : out;
            string layname = src.layout.empty() ? "default" : src.layout;
            auto layout = spec->layouts.find(layname);

            if (layout == spec->layouts.end()) {
                logMsgError() << "Tag '" << src.tag << "': layout '" << layname
                    << "' not defined.";
                appSignalShutdown(EX_DATAERR);
                return;
            }

            auto & url = layout->second.pages[layout->second.defPage].urlSegment;
            if (!genRedirect(out, src.tag + "/index.html", url + ".html"))
                return;

            // Count each page as pending work.
            out->pendingWork += (unsigned) layout->second.pages.size();

            for (auto&& page : layout->second.pages) {
                if (!src.urlSegments.insert(page.urlSegment).second) {
                    logMsgError() << "Tag '" << src.tag << "': url segment '"
                        << page.urlSegment << "' multiply defined.";
                    appSignalShutdown(EX_DATAERR);
                    return;
                }
            }
        }

        // generate pages
        for (auto && src : out->sources) {
            auto spec = src.spec ? src.spec.get() : out;
            string layname = src.layout.empty() ? "default" : src.layout;
            auto layout = spec->layouts.find(layname);

            for (auto && page : layout->second.pages) {
                auto info = new GenPageInfo({ out, src, page });
                info->fn = [info, what]() {
                    genSite(info->out, what);
                    delete info;
                };
                genPage(info);
            }
        }

        out->pendingWork += 1;
        phase = what;
    }
    if (phase == what++) {
        if (--out->pendingWork) {
            // Still have more pages to generate.
            return;
        }

        if (!writeSite(out))
            return;
        TimePoint finish = Clock::now();
        chrono::duration<double> elapsed = finish - s_startTime;
        logMsgInfo() << "Elapsed time: " << elapsed.count() << " seconds.";
        logMsgInfo() << out->outputs.size() << " generated files.";
        logMsgInfo() << "Site generated successfully.";
        appSignalShutdown(EX_OK);
        delete out;
        return;
    }

    assert(!"unknown phase");
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
        .versionOpt(kVersion, "docgen");
    auto & specfile = cli.opt<Path>("[site def file]", Path("docgen.xml"))
        .desc("docgen site definition to process.");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    s_startTime = Clock::now();
    string content;
    if (!fileLoadBinaryWait(&content, *specfile))
        return appSignalUsageError(EX_DATAERR);

    auto site = make_unique<Site>();
    if (!loadSite(site.get(), &content, *specfile))
        return;
    site->configFile = fileAbsolutePath(*specfile);
    logMsgInfo() << "Processing '" << site->configFile << "' into '"
        << site->outDir << "'.";
    genSite(site.release());
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
