// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - xsdgen
#pragma once


/****************************************************************************
*
*   Schema
*
***/

struct EnumValue {
    std::string name;
    std::string desc;
};

struct Attr {
    enum class Content {
        kInvalid,
        kBool,
        kEnum,
        kString,
        kUnsigned,
    };
    enum class Column {
        kInvalid,
        kName,
        kContent,
        kDesc,
        kRequire,
    };
    struct Note {
        Column column{Column::kInvalid};
        std::string text;
    };

    std::string name;
    Content content{Content::kInvalid};
    std::vector<EnumValue> enumValues; // only if content == kEnum
    bool require{false};
    std::string desc;
    std::string overview;
    std::vector<Note> notes;
};

struct Element {
    enum class Content {
        kInvalid,
        kBool,
        kEnum,
        kString,
        kUnsigned,
        kXml,
    };
    enum class Column {
        kInvalid,
        kName,
        kContent,
        kDesc,
        kQuantity,
        kDefault,
    };
    struct Note {
        Column column{Column::kInvalid};
        std::string text;
    };

    std::string name;
    Content content{Content::kInvalid};
    std::vector<EnumValue> enumValues; // only if content == kEnum
    bool require{false};
    bool single{false};
    std::string desc;
    std::string overview;
    std::vector<Attr> attrs;
    std::vector<Note> notes;
    std::vector<Element *> elems;
};

struct Schema {
    std::string xmlns;
    std::string overview;
    Element root;
    std::unordered_map<std::string, Dim::XNode*> contentById;
    std::unordered_map<Dim::XNode *, Element> elementByNode;

    Dim::Path xsdFile;
};


/****************************************************************************
*
*   Public API
*
***/

bool writeXsd(Dim::CharBuf * out, const Schema & schema);

Attr::Content convert(Element::Content ec);

void updateXmlFile(const Dim::Path & name, const Dim::CharBuf & content);
