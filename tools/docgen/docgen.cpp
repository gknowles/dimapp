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
static string translate(string_view fname, string_view content) {
    auto p = Path(fname);
    auto f = fileCreateTemp({}, p.extension());
    fileWriteWait(f, 0, content.data(), content.size());
    auto tmp = string(filePath(f));
    fileClose(f);

    ExecResult res;
    execProgramWait(
        &res,
        {},
        "github-markup.bat",
        tmp
    );

    fileRemove(tmp);
    return toString(res.out);
}

//===========================================================================
static string loadContent(
    const Site & site,
    string_view tag,
    string_view file
) {
    auto path = Path(file).resolve(site.configFile.parentPath());
    if (tag == "HEAD") {
        string content;
        if (!fileLoadBinaryWait(&content, path))
            return {};
        return content;
    }

    string objname = path.str();
    objname.replace(0, site.gitRoot.size() + 1, string(tag) + ":");
    ExecResult res;
    execProgramWait(
        &res,
        {},
        "git",
        "-C",
        site.configFile.parentPath(),
        "show",
        objname
    );
    if (res.err) {
        logMsgError() << "Error: " << res.cmdline;
        logMsgError() << "> " << res.err;
    }
    return toString(res.out);
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
static bool genPage(
    Site * out,
    const Source & src,
    const Layout & layout,
    const Page & page,
    const PageLayout & pglay
) {
    auto raw = loadContent(*out, src.tag, page.file);
    if (raw.empty()) {
        logMsgError() << page.file << ", tag '" << src.tag
            << "': unable to load content";
        appSignalShutdown(EX_DATAERR);
        return false;
    }
    auto content = translate(page.file, raw);
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
    bld.elem("title", page.name + " - " + out->name);
    bld.end(); // </head>

    bld.start("body")
        .attr("data-spy", "scroll")
        .attr("data-target", "#toc")
        .text("\n");
    addNavbar(&bld, *out, src, layout, page);
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

    auto file = Path(src.tag) / page.urlSegment + ".html";
    return addOutput(out, file.str(), move(html));
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
)");
    if (!addOutput(out, "css/docgen.css", move(content)))
        return false;

    return true;
}

//===========================================================================
static bool genSite(Site * out) {
    out->outputs.clear();
    ExecResult res;
    execProgramWait(
        &res,
        {},
        "git",
        "-C",
        out->configFile.parentPath(),
        "rev-parse",
        "--show-toplevel"
    );
    auto buf = toString(res.out);
    out->gitRoot = Path(trim(buf));
    if (out->gitRoot.empty()) {
        logMsgError() << "Error: " << res.cmdline;
        logMsgError() << "> " << res.err;
        appSignalShutdown(EX_OSERR);
        return false;
    }

    if (!genStatics(out))
        return false;
    if (!genRedirect(
        out,
        "index.html",
        out->sources[out->defSource].tag + "/index.html"
    )) {
        return false;
    }

    for (auto && src : out->sources) {
        auto layname = src.layout;
        auto spec = out;

        if (layname.empty()) {
            auto content = loadContent(
                *out,
                src.tag,
                out->configFile.filename()
            );
            src.spec = make_unique<Site>();
            if (!loadSite(
                src.spec.get(),
                &content,
                out->configFile.filename()
            )) {
                return false;
            }
            spec = src.spec.get();
            layname = "default";
        }

        auto layout = spec->layouts.find(layname);
        if (layout == spec->layouts.end()) {
            logMsgError() << "Tag '" << src.tag << "': layout '" << layname
                << "' not defined.";
            appSignalShutdown(EX_DATAERR);
            return false;
        }

        auto & url = layout->second.pages[layout->second.defPage].urlSegment;
        if (!genRedirect(out, src.tag + "/index.html", url + ".html"))
            return false;

        for (auto&& page : layout->second.pages) {
            if (!src.urlSegments.insert(page.urlSegment).second) {
                logMsgError() << "Tag '" << src.tag << "': url segment '"
                    << page.urlSegment << "' multiply defined.";
                appSignalShutdown(EX_DATAERR);
                return false;
            }
        }
    }

    for (auto && src : out->sources) {
        auto layname = src.layout;
        auto spec = src.spec ? src.spec.get() : out;

        if (layname.empty())
            layname = "default";

        auto layout = spec->layouts.find(layname);

        for (auto && page : layout->second.pages) {
            auto pglayname = page.pageLayout.empty()
                ? "default"s
                : page.pageLayout;
            auto pglay = spec->pageLayouts.find(pglayname);
            if (pglay == spec->pageLayouts.end()) {
                logMsgError() << "Tag '" << src.tag << "': page layout '"
                    << pglayname << "' not defined.";
                appSignalShutdown(EX_DATAERR);
                return false;
            }
            if (!genPage(out, src, layout->second, page, pglay->second))
                return false;
        }
    }

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

    string content;
    if (!fileLoadBinaryWait(&content, *specfile))
        return appSignalUsageError(EX_DATAERR);

    Site site;
    if (!loadSite(&site, &content, *specfile))
        return;
    site.configFile = fileAbsolutePath(*specfile);
    logMsgInfo() << "Processing '" << site.configFile << "' into '"
        << site.outDir << "'.";
    if (!genSite(&site))
        return;

    logMsgInfo() << "Site generated successfully.";
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

    return appRun(app, argc, argv);
}
