// xml.h - dim xml
#pragma once

#include "dim/config.h"

#include "dim/charbuf.h"
#include "dim/util.h"

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
inline IXBuilder & end(IXBuilder & out) {
    return out.end();
}
inline IXBuilder & endAttr(IXBuilder & out) {
    return out.endAttr();
}

inline IXBuilder &
operator<<(IXBuilder & out, const IXBuilder::ElemNameProxy & name) {
    return out.start(name.ptr);
}
inline IXBuilder::ElemNameProxy start(const char val[]) {
    return IXBuilder::ElemNameProxy{val};
}

inline IXBuilder &
operator<<(IXBuilder & out, const IXBuilder::AttrNameProxy & name) {
    return out.startAttr(name.ptr);
}
inline IXBuilder::AttrNameProxy attr(const char val[]) {
    return IXBuilder::AttrNameProxy{val};
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
    virtual bool StartDoc() = 0;
    virtual bool EndDoc() = 0;

    virtual bool StartElem(const char name[], size_t nameLen) = 0;
    virtual bool EndElem() = 0;

    virtual bool Attr(
        const char name[],
        size_t nameLen,
        const char value[],
        size_t valueLen) = 0;
    virtual bool Text(const char value[], size_t valueLen) = 0;
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

private:
    IXStreamParserNotify & m_notify;
    unsigned m_line{0};
    const char * m_errMsg{nullptr};
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

    ITempHeap & heap() { return m_heap; }

private:
    TempHeap m_heap;
    XNode * m_root{nullptr};
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
    template <typename T> class Iterator;

    const char * const name;
    const char * const value;
};

IXBuilder & operator<<(IXBuilder & out, const XNode & elem);

XType type(const XNode * elem);

XNode * firstChild(XNode * elem, const char name[] = nullptr);
XNode * lastChild(XNode * elem, const char name[] = nullptr);
const XNode * firstChild(const XNode * elem, const char name[] = nullptr);
const XNode * lastChild(const XNode * elem, const char name[] = nullptr);

XNode * nextSibling(XNode * elem, const char name[]);
XNode * prevSibling(XNode * elem, const char name[]);
const XNode * nextSibling(const XNode * elem, const char name[]);
const XNode * prevSibling(const XNode * elem, const char name[]);

template <typename T> class XNode::Iterator : public ForwardListIterator<T> {
    const char * m_name{nullptr};

public:
    Iterator(T * node, const char name[]);
    Iterator operator++();
};


template <typename T> struct XElemRange {
    XNode::Iterator<T> m_first;
    XNode::Iterator<T> begin() { return m_first; }
    XNode::Iterator<T> end() { return {nullptr, nullptr}; }
};
XElemRange<XNode> elems(XNode * elem, const char name[] = nullptr);
XElemRange<const XNode> elems(const XNode * elem, const char name[] = nullptr);

//===========================================================================
template <typename T>
XNode::Iterator<T>::Iterator(T * node, const char name[])
    : ForwardListIterator(node)
    , m_name(name) {}

//===========================================================================
template <typename T> auto XNode::Iterator<T>::operator++() -> Iterator {
    m_current = nextSibling(m_current, m_name);
    return *this;
}


template <typename T> struct XAttrRange {
    ForwardListIterator<T> m_first;
    ForwardListIterator<T> begin() { return m_first; }
    ForwardListIterator<T> end() { return {nullptr}; }
};
XAttrRange<XAttr> attrs(XNode * elem);
XAttrRange<const XAttr> attrs(const XNode * elem);

template <>
auto ForwardListIterator<XAttr>::operator++() -> ForwardListIterator &;
template <>
auto ForwardListIterator<const XAttr>::operator++() -> ForwardListIterator &;

} // namespace
