// Copyright Glen Knowles 2017 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// json.h - dim json
#pragma once

#include "cppconf/cppconf.h"

#include "core/core.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   Json builder
*
***/

class IJBuilder {
public:
    IJBuilder();
    virtual ~IJBuilder() = default;

    virtual void clear();

    IJBuilder & array();
    IJBuilder & object();

    // used to end arrays, objects, and compound strings
    IJBuilder & end();

    IJBuilder & member(std::string_view name);

    template <typename T>
    IJBuilder & member(std::string_view name, T val) {
        member(name) << val;
        return *this;
    }

    // preformatted value
    IJBuilder & valueRaw(std::string_view val);

    IJBuilder & value(char const val[]);
    IJBuilder & value(std::string_view val);
    IJBuilder & value(char val);
    IJBuilder & value(bool val);
    IJBuilder & value(double val);
    IJBuilder & value(int64_t val);
    IJBuilder & value(uint64_t val);
    IJBuilder & value(std::nullptr_t);

    template<typename T>
    IJBuilder & value(T const & val);

    // Starts a compound string value, following calls to value() append to it,
    // this continues until end() is called.
    IJBuilder & startValue();

    enum class Type : int {
        kInvalid,
        kArray,
        kObject,
        kMember,
        kValue,
        kText,
    };
    struct StateReturn {
        Type next;
        Type parent;
    };
    StateReturn state() const;

protected:
    virtual void append(std::string_view text) = 0;
    virtual void append(char ch) = 0;
    virtual size_t size() const = 0;

private:
    void addString(std::string_view val);
    IJBuilder & fail();

    enum State : int m_state;

    // objects are true, arrays are false
    std::vector<bool> m_stack;
};

//===========================================================================
template<typename T>
inline IJBuilder & IJBuilder::value(T const & val) {
    if constexpr (std::is_convertible_v<T, uint64_t>
        && !std::is_same_v<T, uint64_t>
    ) {
        return value(uint64_t(val));
    } else if constexpr (std::is_convertible_v<T, int64_t>
        && !std::is_same_v<T, int64_t>
    ) {
        return value(int64_t(val));
    } else if constexpr (std::is_convertible_v<T, double>
        && !std::is_same_v<T, double>
    ) {
        return value(double(val));
    } else {
        thread_local std::ostringstream t_os;
        t_os.clear();
        t_os.str({});
        t_os << val;
        return value(std::string_view{t_os.str()});
    }
}

//===========================================================================
template <typename T>
inline IJBuilder & operator<<(IJBuilder & out, T const & val) {
    return out.value(val);
}

//===========================================================================
inline IJBuilder & operator<<(
    IJBuilder & out,
    IJBuilder & (*pfn)(IJBuilder &)
) {
    return pfn(out);
}

//===========================================================================
inline IJBuilder & end(IJBuilder & out) {
    return out.end();
}

//---------------------------------------------------------------------------
class JBuilder : public IJBuilder {
public:
    JBuilder(CharBuf * buf)
        : m_buf(*buf) {}
    void clear() override;

private:
    void append(std::string_view text) override;
    void append(char ch) override;
    size_t size() const override;

    CharBuf & m_buf;
};


/****************************************************************************
*
*   Json stream parser
*
***/

namespace Detail {
class JsonParser;
} // namespace

class IJsonStreamNotify {
public:
    virtual ~IJsonStreamNotify() = default;
    virtual bool startDoc() = 0;
    virtual bool endDoc() = 0;

    virtual bool startArray(std::string_view name) = 0;
    virtual bool endArray() = 0;
    virtual bool startObject(std::string_view name) = 0;
    virtual bool endObject() = 0;

    virtual bool value(std::string_view name, std::string_view val) = 0;
    virtual bool value(std::string_view name, double val) = 0;
    virtual bool value(std::string_view name, bool val) = 0;
    virtual bool value(std::string_view name, nullptr_t) = 0;
};

class JsonStream {
public:
    JsonStream(IJsonStreamNotify * notify);
    ~JsonStream();

    void clear();
    bool parseMore(char src[]);

    bool fail(char const errmsg[]);

    ITempHeap & heap() { return m_heap; }
    IJsonStreamNotify & notify() { return m_notify; }

    char const * errmsg() const { return m_errmsg; }
    size_t errpos() const;

private:
    IJsonStreamNotify & m_notify;
    unsigned m_line{0};
    char const * m_errmsg{};
    TempHeap m_heap;
    Detail::JsonParser * m_base;
};


/****************************************************************************
*
*   Json dom document
*
***/

struct JNode : ListBaseLink<> {
    enum JType : int8_t {
        kInvalid,
        kObject,
        kArray,
        kString,
        kNumber,
        kBoolean,
        kNull,
    };

    JType type;
    union {
        std::string_view sval;
        double nval;
        bool bval;
        Dim::List<JNode> vals;
    };

protected:
    JNode(JType type);
    ~JNode();
};

class JDocument {
public:
    void clear();
    JNode * parse(char src[], std::string_view filename = {});

    void setRoot(JNode * root);

    JNode * addArray(JNode * parent, std::string_view name = {});
    JNode * addObject(JNode * parent, std::string_view name = {});
    JNode * addValue(
        JNode * parent,
        std::string_view val,
        std::string_view name = {}
    );
    JNode * addValue(JNode * parent, double val, std::string_view name = {});
    JNode * addValue(JNode * parent, bool val, std::string_view name = {});
    JNode * addValue(JNode * parent, nullptr_t, std::string_view name = {});

    ITempHeap & heap() { return m_heap; }

    char const * filename() const { return m_filename; }
    JNode * root() { return m_root; }
    JNode const * root() const { return m_root; }

    char const * errmsg() const { return m_errmsg; }
    size_t errpos() const { return m_errpos; }

private:
    TempHeap m_heap;
    char const * m_filename{};
    JNode * m_root{};
    char const * m_errmsg{};
    size_t m_errpos{0};
};

IJBuilder & operator<<(IJBuilder & out, JNode const & node);

JNode::JType nodeType(JNode const * node);
std::string_view nodeName(JNode const * node);

JDocument * document(JNode * node);
JNode * parent(JNode * node);

void unlinkNode(JNode * node);

//===========================================================================
// Node iteration
//===========================================================================
class JNodeIterator : public ForwardListIterator<JNode> {
public:
    JNodeIterator();
    JNodeIterator(JNode * node);
    JNodeIterator operator++();
};

inline JNodeIterator begin(JNodeIterator const & iter) { return iter; }
inline JNodeIterator end(JNodeIterator const & iter) { return {}; }

JNodeIterator nodes(JNode * node);

} // namespace
