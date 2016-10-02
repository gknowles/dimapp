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
            CmdParser * parser,
            const std::string & names,
            const std::string & refName,
            bool multiple,
            bool required,
            bool boolean);
        virtual ~ValueBase();

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
        bool m_invert{false};   // sets false instead of true (only for bools)
        bool m_multiple{false}; // there can be multiple values
        bool m_required{false}; // argument is required (only for positionals)
    };

public:
    void clear();
    bool parse(std::ostream & os, size_t argc, char ** argv);

    template <typename T>
    void addOpt(T * value, const std::string & names, const T & def = {});
    template <typename T>
    void addOptVec(std::vector<T> * values, const std::string & names);
    template <typename T>
    void addRequired(T * value, const std::string & name, const T & def = {});
    template <typename T>
    void addRequiredVec(std::vector<T> * values, const std::string & name);
    template <typename T>
    void addOptional(T * value, const std::string & name, const T & def = {});
    template <typename T>
    void addOptionalVec(std::vector<T> * values, const std::string & name);

    template <typename T>
    CmdOpt<T> & addOpt(const std::string & names, const T & def = {});
    template <typename T> CmdOptVec<T> & addOptVec(const std::string & names);
    template <typename T>
    CmdArg<T> & addRequired(const std::string & name, const T & def = {});
    template <typename T> 
    CmdArgVec<T> & addRequiredVec(const std::string & name);
    template <typename T>
    CmdArg<T> & addOptional(const std::string & name, const T & def = {});
    template <typename T> 
    CmdArgVec<T> & addOptionalVec(const std::string & name);

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
void CmdParser::addOpt(T * value, const std::string & names, const T & def) {
    auto ptr = std::make_unique<CmdOpt<T>>(this, value, names, def);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void CmdParser::addOptVec(std::vector<T> * values, const std::string & names) {
    auto ptr = std::make_unique<CmdOptVec<T>>(this, values, names);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void CmdParser::addRequired(T * value, const std::string & name, const T & def) {
    auto ptr = std::make_unique<CmdArg<T>>(this, value, name, def, true);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void CmdParser::addRequiredVec(std::vector<T> * values, const std::string & name) {
    auto ptr = std::make_unique<CmdArgVec<T>>(this, value, name, true);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void CmdParser::addOptional(T * value, const std::string & name, const T & def) {
    auto ptr = std::make_unique<CmdArg<T>>(this, value, name, def, false);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
void CmdParser::addOptionalVec(std::vector<T> * values, const std::string & name) {
    auto ptr = std::make_unique<CmdArgVec<T>>(this, value, name, false);
    m_values.push_back(move(ptr));
}

//===========================================================================
template <typename T>
CmdOpt<T> & CmdParser::addOpt(const std::string & names, const T & def) {
    auto ptr = std::make_unique<CmdOpt<T>>(this, nullptr, names, def);
    auto & opt = *ptr;
    m_values.push_back(move(ptr));
    return opt;
}

//===========================================================================
template <typename T>
CmdOptVec<T> & CmdParser::addOptVec(const std::string & names) {
    auto ptr = std::make_unique<CmdOptVec<T>>(this, nullptr, names);
    auto & opts = *ptr;
    m_values.push_back(move(ptr));
    return opts;
}

//===========================================================================
template <typename T>
CmdArg<T> & CmdParser::addRequired(const std::string & name, const T & def) {
    auto ptr = std::make_unique<CmdArg<T>>(this, nullptr, name, def, true);
    auto & arg = *ptr;
    m_values.push_back(move(ptr));
    return arg;
}

//===========================================================================
template <typename T>
CmdArgVec<T> & CmdParser::addRequiredVec(const std::string & name) {
    auto ptr = std::make_unique<CmdArgVec<T>>(this, nullptr, name, true);
    auto & args = *ptr;
    m_values.push_back(move(ptr));
    return args;
}

//===========================================================================
template <typename T>
CmdArg<T> & CmdParser::addOptional(const std::string & name, const T & def) {
    auto ptr = std::make_unique<CmdArg<T>>(this, nullptr, name, def, false);
    auto & arg = *ptr;
    m_values.push_back(move(ptr));
    return arg;
}

//===========================================================================
template <typename T>
CmdArgVec<T> & CmdParser::addOptionalVec(const std::string & name) {
    auto ptr = std::make_unique<CmdArgVec<T>>(this, nullptr, name, false);
    auto & args = *ptr;
    m_values.push_back(move(ptr));
    return args;
}


/****************************************************************************
*
*   CmdOpt
*
***/

template <typename T> class CmdOpt : public CmdParser::ValueBase {
public:
    CmdOpt(const std::string & names, const T & def = T{});
    CmdOpt(
        CmdParser * parser,
        T * value,
        const std::string & names,
        const T & def = T{});
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
inline CmdOpt<T>::CmdOpt(const std::string & names, const T & def)
    : CmdOpt(nullptr, nullptr, names, def) {}

//===========================================================================
template <typename T>
inline CmdOpt<T>::CmdOpt(
    CmdParser * p, T * value, const std::string & names, const T & def)
    : CmdParser::ValueBase{p, names, {}, false, false, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool CmdOpt<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> *m_value) ||
        !(interpreter >> std::ws).eof()) {
        *m_value = {};
        return false;
    }
    return true;
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
    CmdOptVec(const std::string & names);
    CmdOptVec(
        CmdParser * p, std::vector<T> * values, const std::string & names);

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
inline CmdOptVec<T>::CmdOptVec(const std::string & names)
    : CmdOptVec(nullptr, nullptr, names) {}

//===========================================================================
template <typename T>
inline CmdOptVec<T>::CmdOptVec(
    CmdParser * p, std::vector<T> * values, const std::string & names)
    : CmdParser::ValueBase{p, names, {}, true, false, std::is_same<T, bool>::value}
    , m_values(values ? values : &m_internal) {}

//===========================================================================
template <typename T>
inline bool CmdOptVec<T>::parseValue(const std::string & value) {
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
        const std::string & name, 
        const T & def = T{}, 
        bool required = false);
    CmdArg(
        CmdParser * p,
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
inline CmdArg<T>::CmdArg(const std::string & name, const T & def, bool required)
    : CmdArg(nullptr, nullptr, name, def, required) {}

//===========================================================================
template <typename T>
inline CmdArg<T>::CmdArg(
    CmdParser * p, T * value, const std::string & name, const T & def, bool required)
    : CmdParser::ValueBase{p, {}, name, false, required, std::is_same<T, bool>::value}
    , m_value{value ? value : &m_internal}
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool CmdArg<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> *m_value) ||
        !(interpreter >> std::ws).eof()) {
        *m_value = {};
        return false;
    }
    return true;
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
    CmdArgVec(const std::string & name, bool required = false);
    CmdArgVec(
        CmdParser * p, 
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
inline CmdArgVec<T>::CmdArgVec(const std::string & name, bool required)
    : CmdArgVec(nullptr, nullptr, name, required) {}

//===========================================================================
template <typename T>
inline CmdArgVec<T>::CmdArgVec(
    CmdParser * p, std::vector<T> * values, const std::string & name, bool required)
    : CmdParser::ValueBase{p, {}, name, true, required, std::is_same<T, bool>::value}
    , m_values{values ? values : &m_internal} {}

//===========================================================================
template <typename T>
inline bool CmdArgVec<T>::parseValue(const std::string & value) {
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
template <typename T> inline void CmdArgVec<T>::resetValue() {
    m_values->clear();
}


/****************************************************************************
*
*   General
*
***/

bool cmdParse(size_t argc, char * argv[]);

} // namespace
