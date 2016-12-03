// xml.h - dim xml
#pragma once

#include "config.h"

#include "charbuf.h"
#include "util.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Xml builder
*
***/

class IXBuilder {
public:
    struct ElemNameProxy {
        const char * ptr;
    };
    struct AttrNameProxy {
        const char * ptr;
    };

public:
    IXBuilder();
    virtual ~IXBuilder() {}

    virtual void clear();

    IXBuilder & start(const char name[]);
    IXBuilder & end();
    IXBuilder & startAttr(const char name[]);
    IXBuilder & endAttr();

    IXBuilder & elem(const char name[], const char text[] = nullptr);
    IXBuilder & attr(const char name[], const char text[]);

    IXBuilder & text(const char text[]);

protected:
    virtual void append(const char text[]) = 0;
    virtual void append(const char text[], size_t count) = 0;
    virtual void appendCopy(size_t pos, size_t count) = 0;
    virtual size_t size() const = 0;

private:
    template <bool isContent> void addText(const char text[]);
    IXBuilder & fail();

    enum State;
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
IXBuilder & operator<<(IXBuilder & out, char val);
IXBuilder & operator<<(IXBuilder & out, const char val[]);
IXBuilder & operator<<(IXBuilder & out, const std::string & val);

template <typename T>
inline IXBuilder & operator<<(IXBuilder & out, const T & val) {
    std::ostringstream os;
    os << val;
    return out.text(os.str().c_str());
}

inline IXBuilder &
operator<<(IXBuilder & out, IXBuilder & (*pfn)(IXBuilder &)) {
    return pfn(out);
}

inline IXBuilder &
operator<<(IXBuilder & out, const IXBuilder::ElemNameProxy & name) {
    return out.start(name.ptr);
}
inline IXBuilder::ElemNameProxy start(const char val[]) {
    return IXBuilder::ElemNameProxy{val};
}
inline IXBuilder & end(IXBuilder & out) {
    return out.end();
}

inline IXBuilder &
operator<<(IXBuilder & out, const IXBuilder::AttrNameProxy & name) {
    return out.startAttr(name.ptr);
}
inline IXBuilder::AttrNameProxy attr(const char val[]) {
    return IXBuilder::AttrNameProxy{val};
}
inline IXBuilder & endAttr(IXBuilder & out) {
    return out.endAttr();
}


class XBuilder : public IXBuilder {
public:
    XBuilder(CharBuf & buf)
        : m_buf(buf) {}
    void clear() override;

private:
    void append(const char text[]) override;
    void append(const char text[], size_t count) override;
    void appendCopy(size_t pos, size_t count) override;
    size_t size() const override;

    CharBuf & m_buf;
};


/****************************************************************************
*
*   Xml stream parser
*
***/

class XStreamParser;
namespace Detail {
class XmlBaseParser;
}

class IXStreamParserNotify {
public:
    virtual ~IXStreamParserNotify() {}
    virtual bool startDoc() = 0;
    virtual bool endDoc() = 0;

    virtual bool startElem(const char name[], size_t nameLen) = 0;
    virtual bool endElem() = 0;

    virtual bool attr(
        const char name[],
        size_t nameLen,
        const char value[],
        size_t valueLen) = 0;
    virtual bool text(const char value[], size_t valueLen) = 0;
};

class XStreamParser {
public:
    XStreamParser(IXStreamParserNotify & notify);
    ~XStreamParser();

    void clear();
    bool parse(char src[]);

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
*   Xml dom document
*
***/

struct XAttr;
struct XNode;

class XDocument {
public:
    XDocument();

    void clear();
    XNode * parse(char src[]);

    XNode * setRoot(const char elemName[], const char text[] = nullptr);
    XNode *
    addElem(XNode * parent, const char name[], const char text[] = nullptr);
    XAttr * addAttr(XNode * elem, const char name[], const char text[]);

    XNode * addText(XNode * parent, const char text[]);

    // Remove all child (not all descendent) text nodes and set the node
    // "value" to the concatenation of the removed text with leading
    // and trailing spaces removed *after* concatenation.
    void normalizeText(XNode * elem);

    ITempHeap & heap() { return m_heap; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const { return m_errpos; }

private:
    TempHeap m_heap;
    XNode * m_root{nullptr};
    const char * m_errmsg{nullptr};
    size_t m_errpos{0};
};

//===========================================================================
// Attr and node (element, comment, text, etc)
//===========================================================================
enum class XType {
    kInvalid,
    kElement,
    kText,
    kComment,
    kPI, // processing instruction
};


struct XAttr {
    const char * const name;
    const char * const value;
};
struct XNode {
    const char * const name;
    const char * const value;
};

IXBuilder & operator<<(IXBuilder & out, const XNode & elem);

XType nodeType(const XNode * elem);

void unlinkAttr(XAttr * attr);
void unlinkNode(XNode * node);

XNode * firstChild(
    XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
XNode * lastChild(
    XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
const XNode * firstChild(
    const XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
const XNode * lastChild(
    const XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);

XNode * nextSibling(
    XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
XNode * prevSibling(
    XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
const XNode * nextSibling(
    const XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);
const XNode * prevSibling(
    const XNode * elem,
    const char name[] = nullptr,
    XType type = XType::kInvalid);

//===========================================================================
// Node iteration
//===========================================================================
template <typename T> class XNodeIterator : public ForwardListIterator<T> {
    const char * m_name{nullptr};
    XType m_type{XType::kInvalid};

public:
    XNodeIterator(T * node, XType type, const char name[]);
    XNodeIterator operator++();
};

//===========================================================================
template <typename T>
XNodeIterator<T>::XNodeIterator(T * node, XType type, const char name[])
    : ForwardListIterator(node)
    , m_type(type)
    , m_name(name) {}

//===========================================================================
template <typename T> auto XNodeIterator<T>::operator++() -> XNodeIterator {
    m_current = nextSibling(m_current, m_name, m_type);
    return *this;
}

template <typename T> struct XNodeRange {
    XNodeIterator<T> m_first;
    XNodeIterator<T> begin() { return m_first; }
    XNodeIterator<T> end() { return {nullptr, XType::kInvalid, nullptr}; }
};
XNodeRange<XNode> elems(XNode * elem, const char name[] = nullptr);
XNodeRange<const XNode> elems(const XNode * elem, const char name[] = nullptr);

XNodeRange<XNode> nodes(XNode * elem, XType type = XType::kInvalid);
XNodeRange<const XNode>
nodes(const XNode * elem, XType type = XType::kInvalid);


//===========================================================================
// Attribute iteration
//===========================================================================
template <typename T> struct XAttrRange {
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
