// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// xml.h - dim xml
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"
#include "core/types.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   XML builder
*
***/

class IXBuilder {
public:
    struct ElemNameProxy {
        const char * name;
        const char * value;
    };
    struct AttrNameProxy {
        const char * name;
        const char * value;
    };

public:
    IXBuilder();
    virtual ~IXBuilder() = default;

    virtual void clear();

    IXBuilder & start(std::string_view name);
    IXBuilder & end();
    IXBuilder & startAttr(std::string_view name);
    IXBuilder & endAttr();

    IXBuilder & elem(std::string_view name);
    IXBuilder & elem(std::string_view name, std::string_view text);
    IXBuilder & attr(std::string_view name, std::string_view text);

    IXBuilder & text(std::string_view text);

protected:
    virtual void append(std::string_view text) = 0;
    virtual void appendCopy(size_t pos, size_t count) = 0;
    virtual size_t size() const = 0;

private:
    template <int N> void addRaw(const char (&text)[N]) {
        append({text, N - 1});
    }

    template <
        typename Char,
        typename = std::enable_if_t<std::is_same_v<Char, char>>
    >
    void addRaw(Char const * const & text) {
        append(text);
    }

    void addRaw(std::string_view text) {
        append(text);
    }

    template <bool isContent>
    void addText(const char text[], size_t count = -1);
    IXBuilder & fail();

    enum State : int;
    State m_state;
    struct Pos {
        size_t pos;
        size_t len;
    };
    std::vector<Pos> m_stack;
};

IXBuilder & operator<<(IXBuilder & out, int64_t val);
IXBuilder & operator<<(IXBuilder & out, uint64_t val);
IXBuilder & operator<<(IXBuilder & out, int val);
IXBuilder & operator<<(IXBuilder & out, unsigned val);
IXBuilder & operator<<(IXBuilder & out, float val);
IXBuilder & operator<<(IXBuilder & out, double val);
IXBuilder & operator<<(IXBuilder & out, long double val);
IXBuilder & operator<<(IXBuilder & out, char val);
IXBuilder & operator<<(IXBuilder & out, const char val[]);
IXBuilder & operator<<(IXBuilder & out, std::string_view val);

template <typename T>
inline IXBuilder & operator<<(IXBuilder & out, const T & val) {
    thread_local std::ostringstream t_os;
    t_os.clear();
    t_os.str({});
    t_os << val;
    return out.text(t_os.str());
}

inline IXBuilder & operator<<(
    IXBuilder & out,
    IXBuilder & (*pfn)(IXBuilder &)
) {
    return pfn(out);
}

inline IXBuilder & operator<<(
    IXBuilder & out,
    const IXBuilder::ElemNameProxy & e
) {
    return e.value ? out.elem(e.name, e.value) : out.start(e.name);
}
inline IXBuilder::ElemNameProxy start(
    const char name[],
    const char val[] = nullptr
) {
    return IXBuilder::ElemNameProxy{name, val};
}
inline IXBuilder & end(IXBuilder & out) {
    return out.end();
}
inline IXBuilder::ElemNameProxy elem(
    const char name[],
    const char val[] = nullptr
) {
    return IXBuilder::ElemNameProxy{name, val ? val : ""};
}

inline IXBuilder & operator<<(
    IXBuilder & out,
    const IXBuilder::AttrNameProxy & a
) {
    return a.value ? out.attr(a.name, a.value) : out.startAttr(a.name);
}
inline IXBuilder::AttrNameProxy attr(
    const char name[],
    const char val[] = nullptr
) {
    return IXBuilder::AttrNameProxy{name, val};
}
inline IXBuilder & endAttr(IXBuilder & out) {
    return out.endAttr();
}


//---------------------------------------------------------------------------
class XBuilder : public IXBuilder {
public:
    XBuilder(CharBuf * buf) : m_buf(*buf) {}
    void clear() override;

private:
    void append(std::string_view text) override;
    void appendCopy(size_t pos, size_t count) override;
    size_t size() const override;

    CharBuf & m_buf;
};


/****************************************************************************
*
*   XML stream parser
*
***/

namespace Detail {
class XmlBaseParser;
} // namespace

class IXStreamParserNotify {
public:
    virtual ~IXStreamParserNotify() = default;
    virtual bool startDoc() = 0;
    virtual bool endDoc() = 0;

    virtual bool startElem(const char name[], size_t nameLen) = 0;
    virtual bool endElem() = 0;

    virtual bool attr(
        const char name[],
        size_t nameLen,
        const char value[],
        size_t valueLen
    ) = 0;
    virtual bool text(const char value[], size_t valueLen) = 0;
};

class XStreamParser {
public:
    XStreamParser(IXStreamParserNotify * notify);
    ~XStreamParser();

    void clear();

    // Parses a document without first clearing the temp heap
    bool parseMore(char src[]);

    bool fail(const char errmsg[]);

    ITempHeap & heap() { return m_heap; }
    IXStreamParserNotify & notify() { return m_notify; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const;

private:
    IXStreamParserNotify & m_notify;
    unsigned m_line{0};
    const char * m_errmsg{nullptr};
    TempHeap m_heap;
    Detail::XmlBaseParser * m_base;
};


/****************************************************************************
*
*   XML DOM document
*
***/

struct XAttr;
struct XNode;

class XDocument {
public:
    void clear();
    XNode * parse(char src[], std::string_view filename = {});

    XNode * setRoot(const char name[], const char text[] = nullptr);
    XNode * addElem(
        XNode * parent,
        const char name[],
        const char text[] = nullptr
    );
    XAttr * addAttr(XNode * elem, const char name[], const char text[]);

    XNode * addText(XNode * parent, const char text[]);

    // Remove all child (not all descendant) text nodes and set the node
    // "value" to the concatenation of the removed text with leading
    // and trailing spaces removed *after* concatenation.
    void normalizeText(XNode * elem);

    ITempHeap & heap() { return m_heap; }

    const char * filename() const { return m_filename; }
    XNode * root() { return m_root; }
    const XNode * root() const { return m_root; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const { return m_errpos; }

private:
    TempHeap m_heap;
    const char * m_filename{nullptr};
    XNode * m_root{nullptr};
    const char * m_errmsg{nullptr};
    size_t m_errpos{0};
};

//===========================================================================
// Nodes (element, comment, text, etc) and attributes
//===========================================================================
enum class XType {
    kInvalid,
    kElement,
    kText,
    kComment,
    kPI, // processing instruction
};

struct XNode {
    const char * const name;
    const char * const value;
};
struct XAttr {
    const char * const name;
    const char * const value;
};

IXBuilder & operator<<(IXBuilder & out, const XNode & elem);
std::string toString(const XNode & elem);

XDocument * document(XNode * node);
XDocument * document(XAttr * attr);

XType nodeType(const XNode * node);

void unlinkAttr(XAttr * attr);
void unlinkNode(XNode * node);

XNode * firstChild(
    XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
const XNode * firstChild(
    const XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
XNode * lastChild(
    XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
const XNode * lastChild(
    const XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);

XNode * nextSibling(
    XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
const XNode * nextSibling(
    const XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
XNode * prevSibling(
    XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);
const XNode * prevSibling(
    const XNode * elem,
    std::string_view name = {},
    XType type = XType::kInvalid
);

const char * text(const XNode * elem, const char def[] = nullptr);

XAttr * attr(XNode * elem, std::string_view name);
const XAttr * attr(const XNode * elem, std::string_view name);

const char * attrValue(
    const XNode * elem,
    std::string_view name,
    const char def[] = nullptr
);
bool attrValue(
    const XNode * elem,
    std::string_view name,
    bool def
);

//===========================================================================
// Node iteration
//===========================================================================
template <typename T>
class XNodeIterator : public ForwardListIterator<T> {
    std::string_view m_name;
    XType m_type{XType::kInvalid};

public:
    XNodeIterator(T * node, XType type, std::string_view name);
    XNodeIterator operator++();
};

template <typename T>
struct XNodeRange {
    XNodeIterator<T> m_first;
    XNodeIterator<T> begin() { return m_first; }
    XNodeIterator<T> end() { return {nullptr, XType::kInvalid, {}}; }
};
XNodeRange<XNode> elems(XNode * elem, std::string_view name = {});
XNodeRange<const XNode> elems(const XNode * elem, std::string_view name = {});

XNodeRange<XNode> nodes(XNode * elem, XType type = XType::kInvalid);
XNodeRange<const XNode> nodes(
    const XNode * elem,
    XType type = XType::kInvalid
);

//===========================================================================
// Attribute iteration
//===========================================================================
template <typename T>
struct XAttrRange {
    ForwardListIterator<T> m_first;
    ForwardListIterator<T> begin() { return m_first; }
    ForwardListIterator<T> end() { return {nullptr}; }
};
template <>
auto ForwardListIterator<XAttr>::operator++() -> ForwardListIterator &;
template <>
auto ForwardListIterator<const XAttr>::operator++() -> ForwardListIterator &;

XAttrRange<XAttr> attrs(XNode * elem);
XAttrRange<const XAttr> attrs(const XNode * elem);

} // namespace
