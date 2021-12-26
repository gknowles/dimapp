// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// xsdgen.cpp - xsdgen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const VersionInfo kVersion = { 1, 0 };


/****************************************************************************
*
*   Variables
*
***/

constexpr TokenTable::Token s_elemContents[] = {
    { (int) Element::Content::kBool, "bool" },
    { (int) Element::Content::kEnum, "enum" },
    { (int) Element::Content::kString, "string" },
    { (int) Element::Content::kUnsigned, "unsigned" },
    { (int) Element::Content::kXml, "xml" },
};
const TokenTable s_elemContentTbl{s_elemContents};

constexpr TokenTable::Token s_elemColumns[] = {
    { (int) Element::Column::kName, "name" },
    { (int) Element::Column::kContent, "content" },
    { (int) Element::Column::kDesc, "desc" },
    { (int) Element::Column::kQuantity, "quantity" },
    { (int) Element::Column::kDefault, "default" },
};
const TokenTable s_elemColumnTbl{s_elemColumns};

constexpr TokenTable::Token s_attrContents[] = {
    { (int) Attr::Content::kBool, "bool" },
    { (int) Attr::Content::kEnum, "enum" },
    { (int) Attr::Content::kString, "string" },
    { (int) Attr::Content::kUnsigned, "unsigned" },
};
const TokenTable s_attrContentTbl{s_attrContents};

constexpr TokenTable::Token s_attrColumns[] = {
    { (int) Attr::Column::kName, "name" },
    { (int) Attr::Column::kContent, "content" },
    { (int) Attr::Column::kDesc, "desc" },
    { (int) Attr::Column::kRequire, "require" },
};
const TokenTable s_attrColumnTbl{s_attrColumns};


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static bool loadEnums(vector<EnumValue> * enums, XNode * root) {
    unordered_set<string> names;
    for (auto & e : *enums)
        names.insert(e.name);
    for (auto && e : elems(root, "Enum")) {
        auto * ptr = (EnumValue *) nullptr;
        auto name = attrValue(&e, "name", "");
        if (names.insert(name).second) {
            ptr = &enums->emplace_back();
        } else {
            auto i = find_if(
                enums->begin(),
                enums->end(),
                [&](auto & a){ return a.name == name; }
            );
            ptr = &*i;
        }
        ptr->name = name;
        if (ptr->name.empty()) {
            logMsgError() << "Missing Attr/Enum/@name attribute";
            return false;
        }
        ptr->desc = attrValue(&e, "desc", "");
    }
    return true;
}

//===========================================================================
static bool loadNote(Schema * schema, Attr::Note * note, XNode * root) {
    note->column = s_attrColumnTbl.find(
        attrValue(root, "column", ""),
        Attr::Column::kInvalid
    );
    if (note->column == Attr::Column::kInvalid) {
        logMsgError() << "Missing or invalid Attr/Note/@column attribute";
        return false;
    }
    note->text = attrValue(root, "text", "");
    if (note->text.empty()) {
        logMsgError() << "Missing Attr/Note/@text attribute";
        return false;
    }
    return true;
}

//===========================================================================
static bool loadAttr(
    Schema * schema,
    Attr * attr,
    XNode * root,
    bool final
) {
    *attr = {};
    if (auto ref = attrValue(root, "ref")) {
        auto e = schema->contentById[ref];
        if (!e) {
            logMsgError() << "No definition for Attr/@ref = '" << ref << "'";
            return false;
        }
        loadAttr(schema, attr, e, false);
    }
    if (auto ptr = attrValue(root, "name"))
        attr->name = ptr;
    attr->content = s_attrContentTbl.find(
        attrValue(root, "content", ""),
        attr->content
    );
    if (attr->content == Attr::Content::kEnum
        && !loadEnums(&attr->enumValues, root)
    ) {
        return false;
    }
    attr->require = attrValue(root, "require", attr->require);
    if (auto ptr = attrValue(root, "desc"))
        attr->desc = ptr;
    if (auto ptr = text(firstChild(root, "Overview")))
        attr->overview = ptr;
    for (auto && e : elems(root, "Note")) {
        auto & note = attr->notes.emplace_back();
        if (!loadNote(schema, &note, &e))
            return false;
    }

    if (final) {
        if (attr->name.empty()) {
            logMsgError() << "Missing Attr/@name attribute";
            return false;
        }
        if (attr->content == Attr::Content::kInvalid) {
            logMsgError() << "Missing Attr[@name='" << attr->name << "']"
                << "/@content attribute";
            return false;
        }
        if (attr->content == Attr::Content::kEnum
            && attr->enumValues.empty()
            && attr->require
        ) {
            logMsgError() << "Missing Attr[@name='" << attr->name << "']"
                "/Enum elements";
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadNote(Schema * schema, Element::Note * note, XNode * root) {
    note->column = s_elemColumnTbl.find(
        attrValue(root, "column", ""),
        Element::Column::kInvalid
    );
    if (note->column == Element::Column::kInvalid) {
        logMsgError() << "Missing or invalid Element/Note/@column attribute";
        return false;
    }
    note->text = attrValue(root, "text", "");
    if (note->text.empty()) {
        logMsgError() << "Missing Element/Note/@text attribute";
        return false;
    }
    return true;
}

//===========================================================================
static bool loadElem(
    Schema * schema,
    Element * elem,
    XNode * root,
    bool final
) {
    *elem = {};
    elem->content = Element::Content::kXml;

    if (auto ref = attrValue(root, "ref")) {
        auto e = schema->contentById[ref];
        if (!e) {
            logMsgError() << "No definition for Element/@ref = '" << ref << "'";
            return false;
        }
        loadElem(schema, elem, e, false);
    }

    if (auto ptr = attrValue(root, "name"))
        elem->name = ptr;
    elem->content = s_elemContentTbl.find(
        attrValue(root, "content", ""),
        elem->content
    );
    if (elem->content == Element::Content::kEnum
        && !loadEnums(&elem->enumValues, root)
    ) {
        return false;
    }
    elem->require = attrValue(root, "require", elem->require);
    elem->single = attrValue(root, "single", elem->single);
    if (auto ptr = attrValue(root, "desc"))
        elem->desc = ptr;
    if (auto ptr = text(firstChild(root, "Overview")))
        elem->overview = ptr;
    for (auto && e : elems(root, "Attr")) {
        auto & attr = elem->attrs.emplace_back();
        if (!loadAttr(schema, &attr, &e, true))
            return false;
    }
    for (auto && e : elems(root, "Note")) {
        auto & note = elem->notes.emplace_back();
        if (!loadNote(schema, &note, &e))
            return false;
    }
    if (firstChild(root, "Element")
        && elem->content != Element::Content::kXml
    ) {
        logMsgError() << "Element[@name='" << elem->name << "']/@content "
            << "conflicts with child Element nodes";
        return false;
    }
    for (auto && e : elems(root, "Element")) {
        if (auto i = schema->elementByNode.find(&e);
            i != schema->elementByNode.end()
        ) {
            elem->elems.push_back(&i->second);
            continue;
        }
        auto & child = schema->elementByNode[&e];
        elem->elems.push_back(&child);
        if (!loadElem(schema, &child, &e, true))
            return false;
    }

    if (final) {
        if (elem->name.empty()) {
            logMsgError() << "Missing Element/@name attribute";
            return false;
        }
        if (elem->content == Element::Content::kInvalid) {
            logMsgError() << "Missing Element[@name='" << elem->name << "']"
                "/@content attribute";
            return false;
        }
        if (elem->content == Element::Content::kEnum
            && elem->enumValues.empty()
            && elem->require
        ) {
            logMsgError() << "Missing Element[@name='" << elem->name << "']"
                << "/Enum elements";
            return false;
        }
    }
    return true;
}

//===========================================================================
static bool loadIds(Schema * schema, XNode * root) {
    for (auto && e : elems(root, "Element")) {
        if (auto id = attrValue(&e, "id")) {
            if (!schema->contentById.insert({id, &e}).second) {
                logMsgError() << "Multiple definitions for "
                    "Element/@id = '" << id << "'";
                return false;
            }
        }
        if (!loadIds(schema, &e))
            return false;
    }
    return true;
}

//===========================================================================
static bool loadContents(Schema * schema, XNode * root) {
    for (auto && content : elems(root, "Content")) {
        auto id = string{attrValue(&content, "id", "")};
        if (!schema->contentById.insert({id, &content}).second) {
            logMsgError() << "Multiple definitions for "
                "Content/@id = '" << id << "'";
            return false;
        }
        if (!loadIds(schema, &content))
            return false;
    }
    return true;
}

//===========================================================================
static bool loadSchema(Schema * schema, XNode * root) {
    *schema = {};
    auto e = firstChild(root, "Output");
    if (auto xmlns = attrValue(e, "xsdNamespace"); !xmlns) {
        logMsgError() << "Missing required attribute: "
            << "Schema/Output/@xsdNamespace";
        return false;
    } else {
        schema->xmlns = xmlns;
    }
    if (auto e = firstChild(root, "Overview"))
        schema->overview = e->value;
    e = firstChild(root, "Root");
    if (!e) {
        logMsgError() << "Missing require element: Schema/Root";
        return false;
    }
    return loadContents(schema, root)
        && loadIds(schema, e)
        && loadElem(schema, &schema->root, e, true);
}


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(int argc, char * argv[]) {
    Cli cli;
    cli.helpNoArgs();
    auto & specfile = cli.opt<Path>("[xsd spec]")
        .desc("Xsd specification to process, extension defaults "
            "to '.xsdgen.xml'");
    if (!cli.parse(argc, argv))
        return appSignalUsageError();

    specfile->defaultExt("xsdgen.xml");
    string content;
    if (!fileLoadBinaryWait(&content, *specfile))
        return appSignalUsageError(EX_DATAERR);
    XDocument doc;
    auto root = doc.parse(content.data(), *specfile);
    if (!root || doc.errmsg()) {
        logParseError("Parsing failed", *specfile, doc.errpos(), content);
        return appSignalShutdown(EX_DATAERR);
    }

    Schema schema;
    if (!loadSchema(&schema, root))
        return appSignalShutdown(EX_DATAERR);
    schema.xsdFile = *specfile;
    if (schema.xsdFile.extension() == ".xml")
        schema.xsdFile.setExt("");
    schema.xsdFile.setExt(".xsd");

    CharBuf buf;
    if (!writeXsd(&buf, schema))
        return appSignalShutdown(EX_DATAERR);

    updateXmlFile(schema.xsdFile, buf);

    appSignalShutdown(EX_OK);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    return appRun(app, argc, argv, kVersion, "xsdgen");
}

//===========================================================================
Attr::Content convert(Element::Content ec) {
    return s_attrContentTbl.find(
        s_elemContentTbl.findName(ec, ""),
        Attr::Content::kInvalid
    );
}

//===========================================================================
bool compareXmlContent(const Path & name, const CharBuf & content) {
    auto nc = toString(content);
    XDocument ndoc;
    auto nroot = ndoc.parse(nc.data());
    if (!nroot)
        return false;
    nc = toString(*nroot);

    string ocontent;
    if (!fileLoadBinaryWait(&ocontent, name))
        return false;
    XDocument odoc;
    auto oroot = odoc.parse(ocontent.data());
    if (!oroot)
        return false;
    ocontent = toString(*oroot);

    return nc == ocontent;
}

//===========================================================================
void updateXmlFile(const Path & name, const CharBuf & content) {
    cout << name << "... " << flush;
    bool exists = fileExists(name);
    if (exists && compareXmlContent(name, content)) {
        cout << "unchanged" << endl;
        return;
    }

    auto f = fileOpen(name, File::fReadWrite | File::fCreat | File::fTrunc);
    if (!f)
        return appSignalShutdown(EX_DATAERR);
    fileWriteWait(f, 0, content.data(), content.size());
    fileClose(f);
    ConsoleScopedAttr color(kConsoleNote);
    cout << (exists ? "UPDATED" : "CREATED") << endl;
}
