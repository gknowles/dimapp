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
    virtual size_t size() = 0;

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
    return out.text(os.str());
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

private:
    void append(const char text[]) override;
    void append(const char text[], size_t count) override;
    void appendCopy(size_t pos, size_t count) override;
    size_t size() override;

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
    bool m_failed{false};
    TempHeap m_heap;
    Detail::XmlBaseParser * m_base;
};


/****************************************************************************
*
*   Xml dom parser
*
***/

struct XAttr;
struct XElem;

class XParser {
public:
    XParser();

    void clear();
    XElem * parse(char src[]);

    XElem * setRoot(const char elemName[], const char text[] = nullptr);
    XElem *
    addElem(XElem * parent, const char name[], const char text[] = nullptr);
    XAttr * addAttr(XElem * elem, const char name[], const char text[]);

    ITempHeap & heap() { return m_heap; }

private:
    TempHeap m_heap;
    XElem * m_root{nullptr};
};

//===========================================================================
// XAttr
//===========================================================================
struct XAttr {
    const char * m_name{nullptr};
    const char * m_value{nullptr};
    XAttr * m_next{nullptr};
    XAttr * m_prev{nullptr};
};

//===========================================================================
// XElem
//===========================================================================
struct XElem {
    template <typename T> class Iterator;

    const char * m_name{nullptr};
    const char * m_value{nullptr};
    XElem * m_next{nullptr};
    XElem * m_prev{nullptr};
};

XElem * prevSibling(XElem * elem, const char name[]);
XElem * nextSibling(XElem * elem, const char name[]);
const XElem * prevSibling(const XElem * elem, const char name[]);
const XElem * nextSibling(const XElem * elem, const char name[]);

template <typename T> class XElem::Iterator : public ForwardListIterator<T> {
    const char * m_name{nullptr};

public:
    Iterator(T * node, const char name[]);
    Iterator operator++();
};

template <typename T> struct XElemRange {
    XElem::Iterator<T> begin();
    XElem::Iterator<T> end();
};
XElemRange<XElem> elems(XElem * elem, const char name[] = nullptr);
XElemRange<const XElem> elems(const XElem * elem, const char name[] = nullptr);

template <typename T> struct XAttrRange {
    ForwardListIterator<T> begin();
    ForwardListIterator<T> end();
};
XAttrRange<XAttr> attrs(XElem * elem);
XAttrRange<const XAttr> attrs(const XElem * elem);

//===========================================================================
template <typename T>
XElem::Iterator<T>::Iterator(T * node, const char name[])
    : ForwardListIterator(node)
    , m_name(name) {}

//===========================================================================
template <typename T> auto XElem::Iterator<T>::operator++() -> Iterator {
    m_current = nextSibling(m_current, m_name);
    return *this;
}

} // namespace
