// Copyright Glen Knowles 2020 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// sitegen.cpp - docgen
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

    CmdOpts();
};

struct TocEntry {
    size_t pos;
    unsigned depth;
    std::string name;
    std::string link;
};

struct GenPageInfo {
    Config * out;
    const Version & ver;
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

static CmdOpts s_opts;


/****************************************************************************
*
*   Generate site files
*
***/

//===========================================================================
static void genFileHeader(CharBuf * out, const string & fname) {
    out->append(fname).pushBack('\n')
        .append("Generated by docgen ").append(kVersion).pushBack('\n');
}

//===========================================================================
static void genHtmlHeader(CharBuf * out, const string & fname) {
    out->append("<!DOCTYPE html>\n<!--\n");
    genFileHeader(out, fname);
    out->append("-->\n");
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
    static const regex s_tocEntry(
        //           depth            id        name
        R"regex(<[hH]([1-6])(?:|\sid="([^"]+)")>(.*)</[hH]\1>)regex",
        regex::optimize
    );

    unordered_map<string, int> ids;
    vector<TocEntry> out;
    cmatch m;
    size_t pos = 0;
    int h1Count = 0;
    for (;;) {
        if (!regex_search(content->data() + pos, m, s_tocEntry))
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
            te.link += StrFrom<int>(num).view();
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
    // and bump up all other headers by one.
    if (h1Count == 1 && out[0].depth == 1) {
        out.erase(out.begin());
        for (auto&& te : out)
            te.depth -= 1;
    }

    return out;
}

//===========================================================================
static void updateXrefLinks(string * content, const Layout & layout) {
    static const regex s_xrefLink(
        //               ref
        R"regex(<a href="([^#"]+)[^"]*">)regex",
        regex::optimize
    );

    unordered_map<string_view, string> mapping;
    for (auto&& page : layout.pages) {
        mapping[page.xrefFile] = page.urlSegment + ".html";
    }

    cmatch m;
    size_t pos = 0;
    for (;;) {
        if (!regex_search(content->data() + pos, m, s_xrefLink))
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
    const Config & cfg,
    const Version & version,
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
            .text(cfg.siteName)
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
    for (auto&& ver : cfg.versions) {
        if (ver.tag == version.tag)
            break;
        if (ver.defaultSource)
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
                .text(version.name);
    if (afterDefault)
        addBadgeOld(&bld);
    bld.end()
        .start("div")
            .attr("class", "dropdown-menu dropdown-menu-md-right")
            .attr("aria-labelledby", "navbarDropdown");
    afterDefault = false;
    for (auto&& ver : cfg.versions) {
        bld.start("a");
        if (ver.tag != version.tag) {
            bld.attr("class", "dropdown-item");
        } else {
            bld.attr("class", "dropdown-item active");
        }
        if (ver.urlSegments.contains(page.urlSegment)) {
            bld.attr("href", "../" + ver.tag + "/" + page.urlSegment + ".html");
        } else {
            bld.attr("href", "../" + ver.tag + "/index.html");
        }
        bld.text(ver.name);
        if (afterDefault)
            addBadgeOld(&bld);
        bld.end();
        if (ver.defaultSource)
            afterDefault = true;
    }
    bld.end() // div.dropdown-menu
        .end(); // div.nav-item

    // GitHub link
    if (cfg.github) {
        bld.start("a")
            .attr("class", "nav-link py-0 px-3")
            .attr("href", cfg.repoUrl)
            .attr("style", "font-size: 24px;")
            .start("i")
                .attr("style", "width: 24px")
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
    string && content,
    const string & fname
) {
    auto cfg = info->ver.cfg ? info->ver.cfg.get() : info->out;
    string layname = info->ver.layout.empty() ? "default" : info->ver.layout;
    auto & layout = cfg->layouts.find(layname)->second;
    auto pglayname = info->page.pageLayout.empty()
        ? "default"s
        : info->page.pageLayout;
    auto & pglay = cfg->pageLayouts.find(pglayname)->second;

    updateXrefLinks(&content, layout);
    auto toc = createToc(&content);

    CharBuf html;
    genHtmlHeader(&html, fname);
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
    bld.start("link")
        .attr("rel", "stylesheet")
        .attr("href", "../css/asciidoc.css")
        .end();
    bld.elem("title", info->page.name + " - " + info->out->siteName);
    bld.end(); // </head>

    bld.start("body")
        .attr("data-spy", "scroll")
        .attr("data-target", "#toc")
        .text("\n");
    addNavbar(&bld, *info->out, info->ver, layout, info->page);
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
    auto cfg = info->ver.cfg ? info->ver.cfg.get() : info->out;
    string layname = info->ver.layout.empty() ? "default" : info->ver.layout;

    unsigned what = 0;

    if (phase == what++) {
        // Load page markup.
        auto pglayname = info->page.pageLayout.empty()
            ? "default"s
            : info->page.pageLayout;
        auto pglay = cfg->pageLayouts.find(pglayname);
        if (pglay == cfg->pageLayouts.end()) {
            logMsgError() << "Tag '" << info->ver.tag << "': page layout '"
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
            info->ver.tag,
            info->page.file
        );
        return;
    }
    if (info->content.empty()) {
        logMsgError() << info->page.file << ", tag '" << info->ver.tag
            << "': unable to load content";
        appSignalShutdown(EX_DATAERR);
        return;
    }
    if (phase == what++) {
        // Save markup to temporary file.
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
        // Apply patch from config file
        if (!info->page.patch.empty()) {
            auto cmdline = Cli::toCmdlineL(
                "patch",
                "-r",
                info->fname + "#",
                info->fname
            );
            execTool(
                [info, what](string && out) {
                    fileRemove(info->fname + "#");
                    if (out.empty())
                        appSignalShutdown(EX_IOERR);
                    if (appStopping()) {
                        info->fn();
                        return;
                    }
                    genPage(info, what);
                },
                cmdline,
                info->page.file + ", tag '" + info->ver.tag + "'",
                { .stdinData = info->page.patch }
            );
            return;
        }

        // No patch, continue to next phase.
        phase = what;
    }
    if (phase == what++) {
        // Convert markup into HTML fragment.
        auto cmdline = Cli::toCmdlineL("github-markup.bat", info->fname);
        execTool(
            [info, what](string && out) {
                fileRemove(info->fname);
                if (appStopping()) {
                    info->fn();
                    return;
                }
                info->content = move(out);
                genPage(info, what);
            },
            cmdline,
            info->page.file + ", tag '" + info->ver.tag + "'"
        );
        return;
    }
    if (phase == what++) {
        // Update HTML fragment, embed into HTML page, and add to site output
        // files.
        auto file = Path(info->ver.tag) / info->page.urlSegment + ".html";
        auto fname = file.str();
        auto html = processPageContent(info, move(info->content), fname);
        if (!addOutput(info->out, fname, move(html)))
            return;
        info->fn();
        return;
    }

    assert(!"unknown phase");
}

//===========================================================================
static bool genStatics(Config * out) {
    CharBuf content;
    string fname;

    //-----------------------------------------------------------------------
    // .nojekyll
    fname = ".nojekyll";
    content.clear();
    genFileHeader(&content, fname);
    if (!addOutput(out, fname, move(content)))
        return false;

    //-----------------------------------------------------------------------
    // css/asciidoc.css
    fname = "css/asciidoc.css";
    content.assign("/*\n");
    genFileHeader(&content, fname);
    content.append(1 + R"(
*/

.halign-center {
    text-align: center;
}
.halign-left {
    text-align: left;
}
.halign-right {
    text-align: right;
}
.table .valign-top {
    vertical-align: top;
}
.table .valign-middle {
    vertical-align: middle;
}
.table .valign-right {
    vertical-align: bottom;
}
)");
    if (!addOutput(out, fname, move(content)))
        return false;

    //-----------------------------------------------------------------------
    // css/docgen.css
    fname = "css/docgen.css";
    content.assign("/*\n");
    genFileHeader(&content, fname);
    content.append(1 + R"(
*/

body {
    position: relative;
}

nav.navbar {
    padding: 0rem 1rem;
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
li p,
table p:last-child {
    margin-bottom: 0;
}
table {
    margin-bottom: 1rem;
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
    if (!addOutput(out, fname, move(content)))
        return false;

    return true;
}

//===========================================================================
static bool genRedirect(
    Config * out,
    string_view outputFile,
    string_view targetUrl
) {
    auto fname = string(outputFile);
    CharBuf html;
    genHtmlHeader(&html, fname);
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
    return addOutput(out, fname, move(html));
}

//===========================================================================
static void genSite(Config * out, unsigned phase = 0) {
    unsigned what = 0;

    if (phase == what++) {
        // Generate infrastructure files for site
        if (!genStatics(out))
            return;

        // Load layouts of all versions
        out->pendingWork = (unsigned) out->versions.size();
        for (auto && ver : out->versions) {
            auto layname = ver.layout;

            if (!layname.empty()) {
                out->pendingWork -= 1;
            } else {
                loadContent(
                    [out, &ver, what](auto && content) {
                        if (ver.cfg = loadConfig(
                            &content,
                            out->configFile.filename()
                        )) {
                            genSite(out, what);
                        }
                    },
                    *out,
                    ver.tag,
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

        // Calculate page metadata
        for (auto&& ver : out->versions) {
            auto spec = ver.cfg ? ver.cfg.get() : out;
            string layname = ver.layout.empty() ? "default" : ver.layout;
            auto layout = spec->layouts.find(layname);

            if (layout == spec->layouts.end()) {
                logMsgError() << "Tag '" << ver.tag << "': layout '" << layname
                    << "' not defined.";
                appSignalShutdown(EX_DATAERR);
                return;
            }

            // Generate infrastructure files for version
            auto & url = layout->second.pages[layout->second.defPage].urlSegment;
            if (!genRedirect(out, ver.tag + "/index.html", url + ".html"))
                return;
            if (ver.defaultSource) {
                if (!genRedirect(out, "index.html", ver.tag + "/index.html"))
                    return;
            }

            // Count each page as pending work.
            out->pendingWork += (unsigned) layout->second.pages.size();

            // Populate list of URLs for version
            for (auto&& page : layout->second.pages) {
                if (!ver.urlSegments.insert(page.urlSegment).second) {
                    logMsgError() << "Tag '" << ver.tag << "': url segment '"
                        << page.urlSegment << "' multiply defined.";
                    appSignalShutdown(EX_DATAERR);
                    return;
                }
                if (ver.defaultSource) {
                    auto fname = page.urlSegment + ".html";
                    if (!genRedirect(out, fname, ver.tag + "/" + fname))
                        return;
                }
            }
        }

        // Generate pages
        for (auto && ver : out->versions) {
            auto spec = ver.cfg ? ver.cfg.get() : out;
            string layname = ver.layout.empty() ? "default" : ver.layout;
            auto layout = spec->layouts.find(layname);

            // Generate pages for version
            for (auto && page : layout->second.pages) {
                auto info = new GenPageInfo({ out, ver, page });
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

        // Replace site output directory with all the new files.
        auto odir = Path(out->siteDir).resolve(out->configFile.parentPath());
        if (!writeOutputs(odir, out->outputs))
            return;

        // Clean up
        auto count = out->outputs.size();
        delete out;

        logMsgInfo() << count << " generated files.";
        if (int errs = logGetMsgCount(kLogTypeError)) {
            ConsoleScopedAttr attr(kConsoleError);
            cerr << "Generation failures: " << errs << endl;
        } else {
            ConsoleScopedAttr ca(kConsoleCheer);
            logMsgInfo() << "Website generated successfully.";
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

static bool genCmd(Cli & cli);

//===========================================================================
CmdOpts::CmdOpts() {
    Cli cli;
    cli.command("site")
        .desc("Generate website files")
        .action(genCmd);
    cli.opt(&cfgfile, "c conf")
        .desc("docgen site configuration to process. "
            "(default: {GIT_ROOT}/docs/docgen.xml)");
}

//===========================================================================
static bool genCmd(Cli & cli) {
    auto cfg = loadConfig(s_opts.cfgfile);
    if (!cfg)
        return cli.fail(EX_DATAERR, "");
    if (cfg->siteDir.empty()) {
        logMsgError() << cfg->configFile
            << ": Output directory for site unspecified.";
        return cli.fail(EX_DATAERR, "");
    }

    ostringstream os;
    os << "Making WEBSITE files from '" << cfg->configFile
        << "' to '" << cfg->siteDir << "'.";
    auto out = logMsgInfo();
    cli.printText(out, os.str());
    genSite(cfg.release());

    return cli.fail(EX_PENDING, "");
}
