// Copyright Glen Knowles 2018 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//
// writexsd.cpp - xsdgen
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

struct XsdInfo {
    unordered_set<const void *> processed;
    unordered_map<string_view, unsigned> nameCounts;
    unordered_map<const Element *, string> elemNames;
    unordered_map<const Attr *, string> attrNames;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void writeDesc(IXBuilder * out, const string & desc) {
   if (!desc.empty())
        out->start("annotation").elem("documentation", desc).end();
}

//===========================================================================
template<typename T>
static string typeName(
    XsdInfo * info,
    const T & type,
    bool onlyIfNew
) {
    if (onlyIfNew && !info->processed.insert(&type).second)
        return {};

    unordered_map<const T *, string> * names;
    if constexpr (is_same_v<T, Element>) {
        names = &info->elemNames;
    } else {
        names = &info->attrNames;
    }
    if (auto i = names->find(&type); i != names->end())
        return i->second;

    auto name = "t"s;
    name += type.name;
    if (auto num = ++info->nameCounts[type.name]; num > 1) {
        name += '-';
        name += StrFrom<unsigned>(num);
    }
    (*names)[&type] = name;
    return name;
}

//===========================================================================
static void writeEnumRestriction(
    IXBuilder * out,
    vector<EnumValue> const & enumValues
) {
    out->start("restriction")
        .attr("base", "string");
    for (auto && en : enumValues) {
        out->start("enumeration")
            .attr("value", en.name);
        writeDesc(out, en.desc);
        out->end();
    }
    out->end();
}

//===========================================================================
static void writeSimpleRestriction(
    IXBuilder * out,
    string_view type
) {
    out->start("restriction")
        .attr("base", type)
        .end();
}

//===========================================================================
static void writeRestriction(
    IXBuilder * out,
    Attr::Content content,
    vector<EnumValue> const & enumValues
) {
    switch (content) {
    case Attr::Content::kInvalid:
        assert(!"Must have content definition");
    case Attr::Content::kBool:
        writeSimpleRestriction(out, "boolean");
        break;
    case Attr::Content::kEnum:
        writeEnumRestriction(out, enumValues);
        break;
    case Attr::Content::kString:
        writeSimpleRestriction(out, "string");
        break;
    case Attr::Content::kUnsigned:
        writeSimpleRestriction(out, "unsignedInt");
        break;
    }
}

//===========================================================================
static void writeSimpleType(
    IXBuilder * out,
    Attr::Content content,
    vector<EnumValue> const & enumValues
) {
    out->start("simpleType");
    writeRestriction(out, content, enumValues);
    out->end();
}

//===========================================================================
static void writeAttr(IXBuilder * out, XsdInfo * info, const Attr & attr) {
    out->start("attribute")
        .attr("name", attr.name);
    if (attr.require)
        out->attr("use", "required");
    writeDesc(out, attr.desc);
    writeSimpleType(out, attr.content, attr.enumValues);
    out->end();
}

//===========================================================================
static void writeElem(IXBuilder * out, XsdInfo * info, const Element & elem) {
    out->start("element")
        .attr("name", elem.name)
        .startAttr("type")
            .text("def:").text(typeName(info, elem, false))
            .endAttr();
    if (!elem.require)
        out->attr("minOccurs", "0");
    if (!elem.single)
        out->attr("maxOccurs", "unbounded");
    out->end();
}

//===========================================================================
static void writeEnumType(
    IXBuilder * out,
    const string & name,
    vector<EnumValue> const & enumValues
) {
    out->start("simpleType")
        .attr("name", name);
    writeEnumRestriction(out, enumValues);
    out->end();
}

//===========================================================================
static void startSimpleExtension(IXBuilder * out, string_view baseType) {
    out->start("simpleContent")
        .start("extension")
        .attr("base", baseType);
}

//===========================================================================
static void writeElemType(
    IXBuilder * out,
    XsdInfo * info,
    const Schema & schema,
    const Element & elem
) {
    auto name = typeName(info, elem, true);
    if (name.empty())
        return;
    string enumTypeName;

    auto & elems = elem.elems;
    out->start("complexType");
    out->attr("name", name);
    writeDesc(out, elem.desc);
    if (elems.empty()) {
        switch (elem.content) {
        case Element::Content::kInvalid:
            assert(!"No content type");
        case Element::Content::kBool:
            startSimpleExtension(out, "boolean");
            out->attr("base", "boolean");
            break;
        case Element::Content::kEnum:
            enumTypeName = "def:"s + name + "-enum";
            startSimpleExtension(out, enumTypeName);
            break;
        case Element::Content::kString:
            startSimpleExtension(out, "string");
            break;
        case Element::Content::kUnsigned:
            startSimpleExtension(out, "unsignedInt");
            break;
        case Element::Content::kXml:
            out->start("complexContent")
                .start("extension")
                .attr("base", "anyType");
            break;
        }
    } else if (elems.size() == 1) {
        out->start("sequence");
        writeElem(out, info, *elems.front());
        out->end();
    } else {
        auto i = find_if(
            elems.begin(),
            elems.end(),
            [](auto & a) { return !a->single; }
        );
        if (i == elems.end()) {
            out->start("all");
            for (auto && e : elems)
                writeElem(out, info, *e);
            out->end();
        } else {
            out->start("choice")
                .attr("minOccurs", "0")
                .attr("maxOccurs", "unbounded");
            for (auto && e : elems)
                writeElem(out, info, *e);
            out->end();
        }
    }
    for (auto && a : elem.attrs)
        writeAttr(out, info, a);
    if (elems.empty())
        out->end().end();
    out->end();

    if (elem.content == Element::Content::kEnum)
        writeEnumType(out, enumTypeName, elem.enumValues);

    //for (auto && a : elem.attrs)
    //    writeAttrType(out, info, schema, &a);
    for (auto && e : elem.elems)
        writeElemType(out, info, schema, *e);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
bool writeXsd(CharBuf * out, const Schema & schema) {
    out->append("<!--\n")
        .append(schema.xsdFile.filename()).pushBack('\n')
        .append("Generated by xsdgen ").append(kVersion)
            .append(" at ").append(Time8601Str().set().view()).pushBack('\n')
        .append("-->\n");

    XBuilder bld(out);
    bld.start("schema")
        .attr("xmlns", "http://www.w3.org/2001/XMLSchema")
        .attr("xmlns:def", schema.xmlns)
        .attr("targetNamespace", schema.xmlns)
        .attr("elementFormDefault", "qualified");
    bld.start("element")
        .attr("name", schema.root.name)
        .attr("type", "def:tSchema")
        .end();

    XsdInfo info;
    writeElemType(&bld, &info, schema, schema.root);

    bld.end();
    return true;
}
