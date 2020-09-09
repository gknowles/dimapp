// Copyright Glen Knowles 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - docgen
#pragma once


/****************************************************************************
*
*   Declarations
*
***/

const char kVersion[] = "1.0.0";


// forward declarations
struct Site;

struct TocEntry {
    size_t pos;
    unsigned depth;
    std::string name;
    std::string link;
};

struct Source {
    std::string name; // defaults to tag
    std::string layout;
    std::string tag;
    bool defaultSource = false;

    std::unique_ptr<Site> spec;
    std::unordered_set<std::string> urlSegments;
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
    std::string name;
    std::string file;
    std::string urlSegment;
    std::string pageLayout;
    bool defaultPage = false;
    std::string xrefFile;
};

struct Layout {
    std::string name;
    size_t defPage = (size_t) -1;
    std::vector<Page> pages;
};

struct Site {
    Dim::Path configFile;
    Dim::Path gitRoot;

    std::string name;
    std::string outDir;

    bool github = false;
    std::string repoUrl;

    size_t defSource = (size_t) -1;
    std::vector<Source> sources;
    std::unordered_map<std::string, Layout> layouts;
    std::unordered_map<std::string, PageLayout> pageLayouts;
    std::unordered_map<std::string, Dim::CharBuf> outputs;
};


/****************************************************************************
*
*   Public API
*
***/

