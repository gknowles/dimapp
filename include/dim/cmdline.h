// cmdline.h - dim services
#pragma once

#include "dim/config.h"

#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Dim {


/****************************************************************************
*
*   CmdParser
*
***/

class CmdParser {
public:
    class ValueBase;
    template <typename T> class ValueShim;
    template <typename T> class Value;
    template <typename T> class ValueVec;

public:
    void resetValues();
    bool parse(std::ostream & os, size_t argc, char ** argv);

    template <typename T>
    Value<T> & arg(T * value, const std::string & keys, const T & def = {});
    template <typename T>
    ValueVec<T> & argVec(std::vector<T> * values, const std::string & keys);

    template <typename T>
    Value<T> & arg(const std::string & keys, const T & def = {});
    template <typename T> ValueVec<T> & argVec(const std::string & keys);

private:
    void addKey(const std::string & name, ValueBase * val);
    void addValue(std::unique_ptr<ValueBase> ptr);
    bool parseValue(ValueBase & out, const char src[]);

    struct ValueKey {
        ValueBase * val;
        bool invert;      // set to false instead of true (only for bools)
        bool optional;    // value doesn't have to be present? (non-bools only)
        std::string name; // name of argument (only for positionals)
    };
    std::list<std::unique_ptr<ValueBase>> m_values;
    std::map<char, ValueKey> m_shortNames;
    std::map<std::string, ValueKey> m_longNames;
    std::vector<ValueKey> m_args;
};

//===========================================================================
template <typename T>
CmdParser::Value<T> &
CmdParser::arg(T * value, const std::string & keys, const T & def) {
    auto ptr = std::make_unique<Value<T>>(value, keys, def);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
CmdParser::ValueVec<T> &
CmdParser::argVec(std::vector<T> * values, const std::string & keys) {
    auto ptr = std::make_unique<ValueVec<T>>(values, keys);
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
CmdParser::Value<T> & CmdParser::arg(const std::string & keys, const T & def) {
    return arg<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
CmdParser::ValueVec<T> & CmdParser::argVec(const std::string & keys) {
    return argVec<T>(nullptr, keys);
}


/****************************************************************************
*
*   CmdParser::ValueBase
*
***/

class CmdParser::ValueBase {
public:
    ValueBase(const std::string & keys, bool multiple, bool boolean);
    virtual ~ValueBase() {}

    // name of the argument that populated the value, or an empty 
    // string if it wasn't populated.
    const std::string & name() const { return m_refName; }

    explicit operator bool() const { return m_explicit; }

protected:
    virtual bool parseValue(const std::string & value) = 0;
    virtual void resetValue() = 0;       // set to passed in default
    virtual void unspecifiedValue() = 0; // set to default constructed
    std::string m_desc;

private:
    friend class CmdParser;
    std::string m_names;
    std::string m_refName;
    bool m_explicit{false}; // the value was explicitly set
    bool m_bool{false};     // the value is a bool (no separate value)
    bool m_multiple{false}; // there can be multiple values
};


/****************************************************************************
*
*   CmdParser::ValueShim
*
***/

template <typename T> class CmdParser::ValueShim : public ValueBase {
public:
    using ValueBase::ValueBase;
    ValueShim(const ValueShim &) = delete;
    ValueShim & operator=(const ValueShim &) = delete;

    T & desc(const std::string & val) {
        m_desc = val;
        return static_cast<T &>(*this);
    }
};


/****************************************************************************
*
*   CmdParser::Value
*
***/

template <typename T> class CmdParser::Value : public ValueShim<Value<T>> {
public:
    Value(T * value, const std::string & keys, const T & def = T{});

    T & operator*() { return *m_value; }
    T * operator->() { return m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override;

    T * m_value;
    T m_internal;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline CmdParser::Value<T>::Value(
    T * value, const std::string & keys, const T & def)
    : ValueShim<Value>{keys, false, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool CmdParser::Value<T>::parseValue(const std::string & value) {
    return stringTo(*m_value, value);
}

//===========================================================================
template <typename T> inline void CmdParser::Value<T>::resetValue() {
    *m_value = m_defValue;
}

//===========================================================================
template <typename T> inline void CmdParser::Value<T>::unspecifiedValue() {
    *m_value = {};
}


/****************************************************************************
*
*   CmdParser::ValueVec
*
***/

template <typename T>
class CmdParser::ValueVec : public ValueShim<ValueVec<T>> {
public:
    ValueVec(std::vector<T> * values, const std::string & keys);

    std::vector<T> & operator*() { return *m_values; }
    std::vector<T> * operator->() { return m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;
    void unspecifiedValue() override {}

    std::vector<T> * m_values;
    std::vector<T> m_internal;
};

//===========================================================================
template <typename T>
inline CmdParser::ValueVec<T>::ValueVec(
    std::vector<T> * values, const std::string & keys)
    : ValueShim<ValueVec>{keys, true, std::is_same<T, bool>::value}
    , m_values(values ? values : &m_internal) {}

//===========================================================================
template <typename T>
inline bool CmdParser::ValueVec<T>::parseValue(const std::string & value) {
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void CmdParser::ValueVec<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   General
*
***/

std::ostream & operator<<(std::ostream & os, const CmdParser::ValueBase & val);

} // namespace
