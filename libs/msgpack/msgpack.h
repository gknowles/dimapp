// Copyright Glen Knowles 2018 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//
// json.h - dim msgpack
#pragma once

#include "cppconf/cppconf.h"

#include "core/charbuf.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility> // std::pair
#include <vector>

namespace Dim::MsgPack {


/****************************************************************************
*
*   MsgPack builder
*
***/

enum Format : uint8_t;

class IBuilder {
public:
    IBuilder();
    virtual ~IBuilder() = default;

    virtual void clear();

    IBuilder & array(size_t count);
    IBuilder & map(size_t count);

    IBuilder & element(std::string_view key);

    template <typename T>
    IBuilder & element(std::string_view key, T val) {
        return element(key).value(val);
    }

    // preformatted value
    IBuilder & valueRaw(std::string_view val);

    IBuilder & value(const char val[]);
    IBuilder & value(std::string_view val);
    IBuilder & value(bool val);
    IBuilder & value(double val);
    IBuilder & value(int64_t val);
    IBuilder & value(uint64_t val);
    IBuilder & value(std::nullptr_t);

    template<typename T>
    IBuilder & value(const T & val);

    size_t depth() const { return m_stack.size(); }

    template<typename T>
    IBuilder & operator<<(const T & val);
    IBuilder & operator<<(IBuilder & (*pfn)(IBuilder &));

protected:
    virtual void append(std::string_view text) = 0;
    virtual void append(char ch) = 0;
    virtual size_t size() const = 0;

private:
    void appendString(std::string_view val);
    bool pop();

    enum class State : int;
    State m_state = {};
    unsigned m_remaining{0};
    std::vector<std::pair<State,unsigned>> m_stack;
};

//===========================================================================
template<typename T>
inline IBuilder & IBuilder::value(const T & val) {
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
        return value(t_os.view());
    }
}

//===========================================================================
template<typename T>
inline IBuilder & IBuilder::operator<<(const T & val) {
    return value(val);
}

//===========================================================================
inline IBuilder & IBuilder::operator<<(
    IBuilder & (*pfn)(IBuilder &)
) {
    return pfn(*this);
}

//---------------------------------------------------------------------------
class Builder : public IBuilder {
public:
    Builder(CharBuf * buf) : m_buf(*buf) {}
    void clear() override;

private:
    void append(std::string_view text) override;
    void append(char ch) override;
    size_t size() const override;

    CharBuf & m_buf;
};


/****************************************************************************
*
*   MsgPack stream parser
*
***/

class IParserNotify {
public:
    virtual ~IParserNotify() = default;

    virtual bool startArray(size_t length) = 0;
    virtual bool startMap(size_t length) = 0;

    // Strings longer than 31 bytes are returned via zero or more valuePrefix
    // calls followed by a final call to value.
    virtual bool valuePrefix(std::string_view val, bool first) = 0;
    virtual bool value(std::string_view val) = 0;

    // Numbers are encoded in the most compact format possible, this means
    // that they could come out in a different format than how they went in.
    // For example, a double going in could come out as an int, as long as it
    // was exactly equivalent.
    virtual bool value(double val) = 0;
    virtual bool value(int64_t val) = 0;
    virtual bool value(uint64_t val) = 0;

    virtual bool value(bool val) = 0;
    virtual bool value(std::nullptr_t) = 0;
};

class StreamParser {
public:
    StreamParser(IParserNotify * notify);
    explicit operator bool() const { return m_errmsg.empty(); }

    void clear();

    // invalid_argument - EINVAL
    // operation_in_progress - EINPROGRESS
    std::error_code parse(size_t * used, std::string_view src);

    bool fail(std::string_view errmsg);

    IParserNotify & notify() { return m_notify; }

    std::string_view errmsg() const { return m_errmsg; };
    size_t errpos() const;

private:
    std::error_code invalid(size_t pos);
    std::error_code inProgress(size_t pos);

    std::error_code notifyArray(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    std::error_code notifyMap(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    template<typename T>
    std::error_code notifyInt(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    std::error_code notifyFloat(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    std::error_code notifyStr(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    std::error_code notifyExt(
        size_t * pos,
        std::string_view src,
        size_t width
    );
    std::error_code notifyFixExt(
        size_t * pos,
        std::string_view src,
        size_t width
    );

    IParserNotify & m_notify;
    std::string m_errmsg;
    size_t m_pos{0};
    CharBuf m_buf;

    Format m_fmt{};
    unsigned m_bytes{0};    // bytes remaining in current object
    int m_objects{1};  // objects remaining in current document
};

} // namespace
