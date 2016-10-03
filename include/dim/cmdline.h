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

class CmdParser;
template <typename T> class CmdOpt;
template <typename T> class CmdOptVec;
template <typename T> class CmdArg;
template <typename T> class CmdArgVec;


/****************************************************************************
*
*   CmdParser
*
***/

class CmdParser {
public:
    class ValueBase {
    public:
        ValueBase(
            const std::string & names,
            const std::string & refName,
            bool multiple,
            bool required,
            bool boolean);
        virtual ~ValueBase() {}

        // for positionals - name of argument
        // for options - name of the option that populated the value,
        //      or an empty string if it wasn't populated.
        const std::string & name() const { return m_refName; }

        explicit operator bool() const { return m_explicit; }

    protected:
        virtual bool parseValue(const std::string & value) = 0;
        virtual void resetValue() = 0;

    private:
        friend class CmdParser;
        std::string m_names;
        std::string m_refName;
        bool m_explicit{false}; // the value was explicitly set
        bool m_bool{false};     // the value is a bool (no separate value)
        bool m_multiple{false}; // there can be multiple values
        bool m_required{false}; // argument is required (only for positionals)
    };

public:
    void resetValues();
    bool parse(std::ostream & os, size_t argc, char ** argv);

    template <typename T>
    CmdOpt<T> &
    addOpt(T * value, const std::string & names, const T & def = {});
    template <typename T>
    CmdOptVec<T> &
    addOptVec(std::vector<T> * values, const std::string & names);
    template <typename T>
    CmdArg<T> & addArg(
        T * value,
        const std::string & name,
        const T & def = {},
        bool required = false);
    template <typename T>
    CmdArgVec<T> & addArgVec(
        std::vector<T> * values,
        const std::string & name,
        bool required = false);

    template <typename T>
    CmdOpt<T> & addOpt(const std::string & names, const T & def = {});
    template <typename T> CmdOptVec<T> & addOptVec(const std::string & names);
    template <typename T>
    CmdArg<T> &
    addArg(const std::string & name, const T & def = {}, bool required = false);
    template <typename T>
    CmdArgVec<T> & addArgVec(const std::string & name, bool required = false);

private:
    template <typename Opt> Opt & add(std::unique_ptr<Opt> ptr);
    void addValue(std::unique_ptr<ValueBase> opt);
    bool parseValue(ValueBase & val, const char ptr[]);

    struct ValueKey {
        ValueBase * val;
        bool invert; // set to false instead of true (only for bools)
    };
    std::list<std::unique_ptr<ValueBase>> m_values;
    std::map<char, ValueKey> m_shortNames;
    std::map<std::string, ValueKey> m_longNames;
    std::vector<ValueKey> m_args;
};

//===========================================================================
template <typename Opt> Opt & CmdParser::add(std::unique_ptr<Opt> ptr) {
    auto & opt = *ptr;
    addValue(std::move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
CmdOpt<T> &
CmdParser::addOpt(T * value, const std::string & names, const T & def) {
    return add(std::make_unique<CmdOpt<T>>(value, names, def));
}

//===========================================================================
template <typename T>
CmdOptVec<T> &
CmdParser::addOptVec(std::vector<T> * values, const std::string & names) {
    return add(std::make_unique<CmdOptVec<T>>(values, names));
}

//===========================================================================
template <typename T>
CmdArg<T> & CmdParser::addArg(
    T * value, const std::string & name, const T & def, bool required) {
    return add(std::make_unique<CmdArg<T>>(value, name, def, required));
}

//===========================================================================
template <typename T>
CmdArgVec<T> & CmdParser::addArgVec(
    std::vector<T> * values, const std::string & name, bool required) {
    return add(std::make_unique<CmdArgVec<T>>(values, name, required));
}

//===========================================================================
template <typename T>
CmdOpt<T> & CmdParser::addOpt(const std::string & names, const T & def) {
    return add(std::make_unique<CmdOpt<T>>(nullptr, names, def));
}

//===========================================================================
template <typename T>
CmdOptVec<T> & CmdParser::addOptVec(const std::string & names) {
    return add(std::make_unique<CmdOptVec<T>>(nullptr, names));
}

//===========================================================================
template <typename T>
CmdArg<T> &
CmdParser::addArg(const std::string & name, const T & def, bool required) {
    return add(std::make_unique<CmdArg<T>>(nullptr, name, def, required));
}

//===========================================================================
template <typename T>
CmdArgVec<T> & CmdParser::addArgVec(const std::string & name, bool required) {
    return add(std::make_unique<CmdArgVec<T>>(nullptr, name, required));
}


/****************************************************************************
*
*   CmdOpt
*
***/

template <typename T> class CmdOpt : public CmdParser::ValueBase {
public:
    CmdOpt(T * value, const std::string & names, const T & def = T{});
    CmdOpt(const CmdOpt &) = delete;
    CmdOpt & operator=(const CmdOpt &) = delete;

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
inline CmdOpt<T>::CmdOpt(T * value, const std::string & names, const T & def)
    : CmdParser::ValueBase{names,
                           {},
                           false,
                           false,
                           std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool CmdOpt<T>::parseValue(const std::string & value) {
    return stringTo(*m_value, value);
}

//===========================================================================
template <typename T> inline void CmdOpt<T>::resetValue() {
    *m_value = m_defValue;
}


/****************************************************************************
*
*   CmdOptVec
*
***/

template <typename T> class CmdOptVec : public CmdParser::ValueBase {
public:
    CmdOptVec(std::vector<T> * values, const std::string & names);

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
inline CmdOptVec<T>::CmdOptVec(
    std::vector<T> * values, const std::string & names)
    : CmdParser::ValueBase{names, {}, true, false, std::is_same<T, bool>::value}
    , m_values(values ? values : &m_internal) {}

//===========================================================================
template <typename T>
inline bool CmdOptVec<T>::parseValue(const std::string & value) {
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void CmdOptVec<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   CmdArg
*
***/

template <typename T> class CmdArg : public CmdParser::ValueBase {
public:
    CmdArg(
        T * value,
        const std::string & name,
        const T & def = T{},
        bool required = false);

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
inline CmdArg<T>::CmdArg(
    T * value, const std::string & name, const T & def, bool required)
    : CmdParser::ValueBase{{},
                           name,
                           false,
                           required,
                           std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool CmdArg<T>::parseValue(const std::string & value) {
    return stringTo(*m_value, value);
}

//===========================================================================
template <typename T> inline void CmdArg<T>::resetValue() {
    *m_value = m_defValue;
}


/****************************************************************************
*
*   CmdArgVec
*
***/

template <typename T> class CmdArgVec : public CmdParser::ValueBase {
public:
    CmdArgVec(
        std::vector<T> * values,
        const std::string & name,
        bool required = false);

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
inline CmdArgVec<T>::CmdArgVec(
    std::vector<T> * values, const std::string & name, bool required)
    : CmdParser::ValueBase{{},
                           name,
                           true,
                           required,
                           std::is_same<T, bool>::value}
    , m_values{values ? values : &m_internal} {}

//===========================================================================
template <typename T>
inline bool CmdArgVec<T>::parseValue(const std::string & value) {
    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void CmdArgVec<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   General
*
***/

std::ostream & operator<<(std::ostream & os, const CmdParser::ValueBase & val);

} // namespace
