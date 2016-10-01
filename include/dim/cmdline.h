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
namespace CmdLine {

class Parser;
template <typename T> class Option;
template <typename T> class OptionVector;
template <typename T> class Argument;
template <typename T> class ArgumentVector;


/****************************************************************************
*
*   ValueBase
*
***/

class ValueBase {
public:
    ValueBase(
        Parser * parser,
        const std::string & names,
        const std::string & refName,
        bool multiple,
        bool boolean);
    virtual ~ValueBase();

    explicit operator bool() const { return m_explicit; }

protected:
    virtual bool parseValue(const std::string & value) = 0;
    virtual void resetValue() = 0;

private:
    friend class Parser;
    std::string m_names;
    std::string m_refName;
    bool m_explicit{false}; // the value was explicitly set
    bool m_bool{false};     // the value is a bool (no separate value)
    bool m_multiple{false}; // there can be multiple values
};


/****************************************************************************
*
*   Parser
*
***/

class Parser {
public:
    void clear();
    bool parse(size_t argc, char ** argv);

    template <typename T>
    void addOpt(T * value, const std::string & names, const T & def = {});
    template <typename T>
    void addOptVec(std::vector<T> * values, const std::string & names);
    template <typename T>
    void addArg(T * value, const std::string & name, const T & def = {});
    template <typename T>
    void addArgVec(std::vector<T> * values, const std::string & name);

    template <typename T>
    Option<T> & addOpt(const std::string & names, const T & def = {});
    template <typename T>
    OptionVector<T> & addOptVec(const std::string & names);
    template <typename T>
    Argument<T> & addArg(const std::string & name, const T & def = {});
    template <typename T>
    ArgumentVector<T> & addArgVec(const std::string & name);

    void add(ValueBase & opt);

private:
    bool parseValue(ValueBase & val, const char ptr[]);

    std::list<std::unique_ptr<ValueBase>> m_values;
    std::map<char, ValueBase *> m_shortNames;
    std::map<std::string, ValueBase *> m_longNames;
    std::vector<ValueBase *> m_args;
};

//===========================================================================
template <typename T>
void Parser::addOpt(T * value, const std::string & names, const T & def) {
    auto ptr = std::make_unique<Option<T>>(this, value, names, def);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void Parser::addOptVec(std::vector<T> * values, const std::string & names) {
    auto ptr = std::make_unique<OptionVector<T>>(this, values, names);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void Parser::addArg(T * value, const std::string & name, const T & def) {
    auto ptr = std::make_unique<Argument<T>>(this, value, name, def);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void Parser::addArgVec(std::vector<T> * values, const std::string & name) {
    auto ptr = std::make_unique<ArgumentVector<T>>(this, value, name);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
Option<T> & Parser::addOpt(const std::string & names, const T & def) {
    auto ptr = std::make_unique<Option<T>>(this, nullptr, names, def);
    auto & opt = *ptr;
    m_values.push_back(move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
OptionVector<T> & Parser::addOptVec(const std::string & names) {
    auto ptr = std::make_unique<OptionVector<T>>(this, nullptr, names);
    auto & opts = *ptr;
    m_values.push_back(move(ptr));
    return opts;
}

//===========================================================================
template <typename T>
Argument<T> & Parser::addArg(const std::string & name, const T & def) {
    auto ptr = std::make_unique<Argument<T>>(this, nullptr, name, def);
    auto & arg = *ptr;
    m_values.push_back(move(ptr));
    return arg;
}

//===========================================================================
template <typename T>
ArgumentVector<T> & Parser::addArgVec(const std::string & name) {
    auto ptr = std::make_unique<ArgumentVector<T>>(this, nullptr, name);
    auto & args = *ptr;
    m_values.push_back(move(ptr));
    return args;
}


/****************************************************************************
*
*   Option
*
***/

template <typename T> class Option : public ValueBase {
public:
    Option(const std::string & names, const T & def = T{});
    Option(
        Parser * parser,
        T * value,
        const std::string & names,
        const T & def = T{});
    Option(const Option &) = delete;
    Option & operator=(const Option &) = delete;

    T & operator*() { return *m_value; }
    T * operator->() { return m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    T * m_value;
    T m_internal;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Option<T>::Option(const std::string & names, const T & def)
    : Option(nullptr, nullptr, names, def) {}

//===========================================================================
template <typename T>
inline Option<T>::Option(
    Parser * p, T * value, const std::string & names, const T & def)
    : ValueBase{p, names, {}, false, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Option<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> *m_value) ||
        !(interpreter >> std::ws).eof()) {
        *m_value = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T> inline void Option<T>::resetValue() {
    *m_value = m_defValue;
}


/****************************************************************************
*
*   OptionVector
*
***/

template <typename T> class OptionVector : public ValueBase {
public:
    OptionVector(const std::string & names);
    OptionVector(
        Parser * p, std::vector<T> * values, const std::string & names);

    std::vector<T> & operator*() { return *m_values; }
    std::vector<T> * operator->() { return m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    std::vector<T> * m_values;
    std::vector<T> m_internal;
};

//===========================================================================
template <typename T>
inline OptionVector<T>::OptionVector(const std::string & names)
    : OptionVector(nullptr, nullptr, names) {}

//===========================================================================
template <typename T>
inline OptionVector<T>::OptionVector(
    Parser * p, std::vector<T> * values, const std::string & names)
    : ValueBase{p, names, {}, true, std::is_same<T, bool>::value}
    , m_values(values ? values : &m_internal) {}

//===========================================================================
template <typename T>
inline bool OptionVector<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    T tmp;
    if (!(interpreter << value) || !(interpreter >> tmp) ||
        !(interpreter >> std::ws).eof()) {
        return false;
    }
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void OptionVector<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   Argument
*
***/

template <typename T> class Argument : public ValueBase {
public:
    Argument(const std::string & name, const T & def = T{});
    Argument(
        Parser * p, T * value, const std::string & name, const T & def = T{});

    T & operator*() { return *m_value; }
    T * operator->() { return m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    T * m_value;
    T m_internal;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Argument<T>::Argument(const std::string & name, const T & def)
    : Argument(nullptr, nullptr, name, def) {}

//===========================================================================
template <typename T>
inline Argument<T>::Argument(
    Parser * p, T * value, const std::string & name, const T & def)
    : ValueBase{p, {}, name, false, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Argument<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> *m_value) ||
        !(interpreter >> std::ws).eof()) {
        *m_value = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T> inline void Argument<T>::resetValue() {
    *m_value = m_defValue;
}


/****************************************************************************
*
*   ArgumentVector
*
***/

template <typename T> class ArgumentVector : public ValueBase {
public:
    ArgumentVector(const std::string & name);
    ArgumentVector(
        Parser * p, std::vector<T> * values, const std::string & name);

    std::vector<T> & operator*() { return *m_values; }
    std::vector<T> * operator->() { return m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    std::vector<T> * m_values;
    std::vector<T> m_internal;
};

//===========================================================================
template <typename T>
inline ArgumentVector<T>::ArgumentVector(const std::string & name)
    : ArgumentVector(nullptr, nullptr, name) {}

//===========================================================================
template <typename T>
inline ArgumentVector<T>::ArgumentVector(
    Parser * p, std::vector<T> * values, const std::string & name)
    : ValueBase{p, {}, name, true, std::is_same<T, bool>::value}
    , m_values{values ? values : &m_internal} {}

//===========================================================================
template <typename T>
inline bool ArgumentVector<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    T tmp;
    if (!(interpreter << value) || !(interpreter >> tmp) ||
        !(interpreter >> std::ws).eof()) {
        return false;
    }
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void ArgumentVector<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   General
*
***/

bool parseOptions(size_t argc, char * argv[]);

} // namespace
} // namespace
