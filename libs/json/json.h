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
    JBuilder(CharBuf & buf)
        : m_buf(buf) {}
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
class JsonBaseParser;
} // namespace

class IJsonStreamNotify {
public:
    virtual ~IJsonStreamNotify() = default;
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

class JsonStream {
public:
    JsonStream(IJsonStreamNotify & notify);
    ~JsonStream();

    void clear();
    bool parse(char src[]);

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
    Detail::JsonBaseParser * m_base;
};


/****************************************************************************
*
*   Json dom document
*
***/

struct JValue;

class JDocument {
public:
    JDocument();

    void clear();
    JValue * parse(char src[], std::string_view filename = {});

    JValue * setRoot(const char name[], const char text[] = nullptr);
    JValue * addValue(
        JValue * parent,
        const char name[],
        const char text[] = nullptr
    );

    ITempHeap & heap() { return m_heap; }

    const char * filename() const { return m_filename; }
    JValue * root() { return m_root; }
    const JValue * root() const { return m_root; }

    const char * errmsg() const { return m_errmsg; }
    size_t errpos() const { return m_errpos; }

private:
    TempHeap m_heap;
    const char * m_filename{nullptr};
    JValue * m_root{nullptr};
    const char * m_errmsg{nullptr};
    size_t m_errpos{0};
};

//===========================================================================
// Types (array, object, string, bool, etc)
//===========================================================================
enum class JType {
    kInvalid,
    kObject,
    kArray,
    kString,
    kNumber,
    kBoolean,
};

struct JArray {
};
struct JObject {
};

IJBuilder & operator<<(IJBuilder & out, const JValue & elem);

JDocument * document(JValue * node);

JType valueType(const JValue * node);

void unlinkNode(JValue * node);

} // namespace
