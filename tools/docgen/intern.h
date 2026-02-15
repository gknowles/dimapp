// Copyright Glen Knowles 2020 - 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - docgen
#pragma once


/****************************************************************************
*
*   Declarations
*
***/

// forward declarations
struct Config;

struct Version {
    std::string name; // defaults to tag
    std::string layout;
    std::string tag;
    bool defaultSource = false;

    std::unique_ptr<Config> cfg;
    std::unordered_set<std::string> urlSegments;
};

struct Spawn {
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
    bool untrackedChildren = false;
};

struct Compiler {
    std::string fileExt;
    Spawn compile;
};

struct Script {
    std::string prefix;
    std::string commentPrefix;
    std::vector<std::regex> envSets;
    Spawn shell;
};

struct Column {
    enum Content {
        kContentInvalid,
        kContentGroupToc,
        kContentToc,
        kContentBody,
    };

    Content content;
    std::string maxWidth;
};

struct PageLayout {
    std::string name;
    std::string scrollSpy;
    std::vector<Column> columns;
};

enum LoadMode : uint8_t {
    fLoadSite  = 1,  // included in site generation
    fLoadTests = 2,  // searched for tests
    fLoadAny   = fLoadSite | fLoadTests,
};

struct Page {
    enum Type {
        kUnknown,
        kAsciidoc,
        kMarkdown,
        kCpp,
    };
    std::string name;
    std::string file;
    Type type = kUnknown;
    std::string urlSegment;
    std::string urlRoot;
    bool defaultPage = false;
    std::string xrefFile; // defaults to file
    std::string patch;

    // For child pages
    size_t rootPage = (size_t) -1;
    unsigned depth = 0;
    std::list<Page> pages;
    size_t defChildPage = (size_t) -1;

    // Inherited
    std::string pageLayout;
    Dim::EnumFlags<LoadMode> modes = fLoadAny;
};

struct Layout {
    std::string name;
    size_t defPage = (size_t) -1;
    std::list<Page> pages;
};

struct SiteFile {
    std::string file;
    std::string tag;
    std::string url;
};

struct Config {
    Dim::Path configFile;
    Dim::Path gitRoot;
    std::string tag;

    std::string siteName;
    std::string siteDir;
    std::string siteFavicon;
    bool github = false;
    std::string repoUrl;
    size_t defVersion = (size_t) -1;
    std::vector<Version> versions;
    std::unordered_map<std::string, Version*> versionsByTag;
    std::vector<SiteFile> files;

    std::string sampDir;
    std::unordered_map<std::string, std::shared_ptr<Compiler>> compilers;
    std::unordered_map<std::string, std::shared_ptr<Script>> scripts;

    std::unordered_map<std::string, Layout> layouts;
    std::unordered_map<std::string, PageLayout> pageLayouts;

    std::unordered_map<std::string, Dim::CharBuf> outputs;

    unsigned phase = 0;
    std::atomic<unsigned> pendingWork = 0;
};


/****************************************************************************
*
*   Public API
*
***/

void writeContent(
    std::function<void()> fn,
    std::string_view path,
    std::string_view content
);

void loadContent(
    std::function<void(std::string&&)> fn,
    const Config & site,
    std::string_view tag,
    std::string_view file
);

std::unique_ptr<Config> loadConfig(std::string_view cfgfile, LoadMode mode);
std::unique_ptr<Config> loadConfig(
    std::string * content,
    std::string_view path,
    std::string_view gitRoot,
    LoadMode mode
);

bool addOutput(
    Config * out,
    const Dim::Path & file,
    Dim::CharBuf && content,
    bool forceCRLF = true
);
bool writeOutputs(
    std::string_view odir,
    const std::unordered_map<std::string, Dim::CharBuf> & files
);
