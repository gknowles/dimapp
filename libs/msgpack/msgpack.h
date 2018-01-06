// Copyright Glen Knowles 2018.
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

namespace Dim {


/****************************************************************************
*
*   MsgPack builder
*
***/

class IMsgBuilder {
public:
    IMsgBuilder();
    virtual ~IMsgBuilder() = default;

    virtual void clear();

    IMsgBuilder & array(size_t count);
    IMsgBuilder & map(size_t count);

    IMsgBuilder & element(std::string_view key);

    template <typename T>
    IMsgBuilder & element(std::string_view key, T val) {
        element(key) << val;
        return *this;
    }

    // preformatted value
    IMsgBuilder & valueRaw(std::string_view val);

    IMsgBuilder & value(const char val[]);
    IMsgBuilder & value(std::string_view val);
    IMsgBuilder & value(bool val);
    IMsgBuilder & value(double val);
    IMsgBuilder & ivalue(int64_t val);
    IMsgBuilder & uvalue(uint64_t val);
    IMsgBuilder & value(std::nullptr_t);

protected:
    virtual void append(std::string_view text) = 0;
    virtual void append(char ch) = 0;
    virtual size_t size() const = 0;

private:
    void appendString(std::string_view val);
    bool pop();

    enum class State : int m_state;
    unsigned m_remaining{0};

    // maps are true, arrays are false
    std::vector<std::pair<State,unsigned>> m_stack;
};

IMsgBuilder & operator<<(IMsgBuilder & out, std::string_view val);
IMsgBuilder & operator<<(IMsgBuilder & out, bool val);
IMsgBuilder & operator<<(IMsgBuilder & out, double val);
IMsgBuilder & operator<<(IMsgBuilder & out, int64_t val);
IMsgBuilder & operator<<(IMsgBuilder & out, uint64_t val);
IMsgBuilder & operator<<(IMsgBuilder & out, std::nullptr_t);

template <typename T, typename =
    std::enable_if_t<!std::is_integral_v<T> && !std::is_enum_v<T>>
>
inline IMsgBuilder & operator<<(IMsgBuilder & out, const T & val) {
    thread_local std::ostringstream t_os;
    t_os.clear();
    t_os.str({});
    t_os << val;
    return out.value(t_os.str());
}

inline IMsgBuilder &
operator<<(IMsgBuilder & out, IMsgBuilder & (*pfn)(IMsgBuilder &)) {
    return pfn(out);
}

//---------------------------------------------------------------------------
class MsgBuilder : public IMsgBuilder {
public:
    MsgBuilder(CharBuf & buf)
        : m_buf(buf) {}
    void clear() override;

private:
    void append(std::string_view text) override;
    void append(char ch) override;
    size_t size() const override;

    CharBuf & m_buf;
};


} // namespace
