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

    IJBuilder & object();
    IJBuilder & end(); // used to end both objects and arrays
    IJBuilder & array();

    IJBuilder & member(std::string_view name);

    template <typename T>
    IJBuilder & member(std::string_view name, T val) {
        member(name) << val;
        return *this;
    }

    // preformatted value
    IJBuilder & valueRaw(std::string_view val);

    IJBuilder & value(const char val[]);
    IJBuilder & value(std::string_view val);
    IJBuilder & value(bool val);
    IJBuilder & value(double val);
    IJBuilder & value(int64_t val);
    IJBuilder & value(uint64_t val);
    IJBuilder & value(std::nullptr_t);

    template<typename T>
    IJBuilder & value(const T & val);

protected:
    virtual void append(std::string_view text) = 0;
    virtual void append(char ch) = 0;
    virtual size_t size() const = 0;

private:
    void appendString(std::string_view val);
    IJBuilder & fail();

    enum State : int m_state;

    // objects are true, arrays are false
    std::vector<bool> m_stack;
};

//===========================================================================
template<typename T>
inline IJBuilder & IJBuilder::value(const T & val) {
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
        return value(string_view{t_os.str()});
    }
}

//===========================================================================
template <typename T>
inline IJBuilder & operator<<(IJBuilder & out, const T & val) {
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

    bool fail(const char errmsg[]);

    ITempHeap & heap() { return m_heap; }
    IJsonStreamNotify & notify() { return m_notify; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const;

private:
    IJsonStreamNotify & m_notify;
    unsigned m_line{0};
    const char * m_errmsg{nullptr};
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

    const char * filename() const { return m_filename; }
    JNode * root() { return m_root; }
    const JNode * root() const { return m_root; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const { return m_errpos; }

private:
    TempHeap m_heap;
    const char * m_filename{nullptr};
    JNode * m_root{nullptr};
    const char * m_errmsg{nullptr};
    size_t m_errpos{0};
};

IJBuilder & operator<<(IJBuilder & out, const JNode & node);

JNode::JType nodeType(const JNode * node);
std::string_view nodeName(const JNode * node);

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

inline JNodeIterator begin(const JNodeIterator & iter) { return iter; }
inline JNodeIterator end(const JNodeIterator & iter) { return {}; }

JNodeIterator nodes(JNode * node);

} // namespace
