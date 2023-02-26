// Copyright Glen Knowles 2020 - 2023.
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
        kContentToc,
        kContentBody,
    };

    Content content;
    std::string maxWidth;
};

struct PageLayout {
    std::string name;
    std::vector<Column> columns;
};

struct Page {
    enum Type {
        kUnknown,
        kAsciidoc,
        kMarkdown,
    };
    std::string name;
    std::string file;
    Type type = kUnknown;
    std::string urlSegment;
    std::string pageLayout;
    bool defaultPage = false;
    std::string xrefFile; // defaults to file
    std::string patch;
    bool site = true;   // processed by site generation
    bool test = true;   // searched for tests
};

struct Layout {
    std::string name;
    size_t defPage = (size_t) -1;
    std::vector<Page> pages;
};

struct Config {
    Dim::Path configFile;
    Dim::Path gitRoot;

    std::string siteName;
    std::string siteDir;
    bool github = false;
    std::string repoUrl;
    size_t defVersion = (size_t) -1;
    std::vector<Version> versions;

    std::string sampDir;
    std::unordered_map<std::string, std::shared_ptr<Compiler>> compilers;
    std::unordered_map<std::string, std::shared_ptr<Script>> scripts;

    std::unordered_map<std::string, Layout> layouts;
    std::unordered_map<std::string, PageLayout> pageLayouts;

    std::unordered_map<std::string, Dim::CharBuf> outputs;

    unsigned pendingWork = 0;
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

std::unique_ptr<Config> loadConfig(std::string_view cfgfile);
std::unique_ptr<Config> loadConfig(
    std::string * content,
    std::string_view path
);

bool addOutput(
    Config * out,
    const std::string & file,
    Dim::CharBuf && content,
    bool normalize = true
);
bool writeOutputs(
    std::string_view odir,
    std::unordered_map<std::string, Dim::CharBuf> files
);
