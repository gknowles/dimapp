// cmdline.h - dim services
#pragma once

#include "dim/config.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace Dim {
namespace CmdLine {

class Parser;


/****************************************************************************
*
*   ValueBase
*
***/

class ValueBase {
public:
    ValueBase(
        const std::string & names,
        const std::string & refName,
        bool multiple,
        bool boolean);
    virtual ~ValueBase();

    explicit operator bool () const { return m_explicit; }

protected:
    virtual bool parseValue(const std::string & value) = 0;
    virtual void resetValue () = 0;

private:
    friend Parser;
    std::string m_names;
    std::string m_refName;
    bool m_explicit{false}; // the value was explicitly set
    bool m_bool{false};     // the value is a bool (no separate value)
    bool m_multiple{false}; // there can be multiple values
};


/****************************************************************************
*
*   Option
*
***/

template <typename T> class Option : public ValueBase {
public:
    Option(
        const std::string & names,
        const T & def = T{});

    T & operator* () { return m_value; }
    T * operator-> () { return &m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    T m_value;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Option<T>::Option(
    const std::string & names, const T & def)
    : ValueBase{names, "", false, false}
    , m_value{def} 
    , m_defValue{def} {}

//===========================================================================
template <>
inline Option<bool>::Option(
    const std::string & names, const bool & def)
    : ValueBase{names, "", false, true}
    , m_value{def} 
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Option<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> m_value) ||
        !(interpreter >> std::ws).eof()) {
        m_value = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
inline void Option<T>::resetValue() {
    m_value = m_defValue;
}


/****************************************************************************
*
*   OptionVector
*
***/

template <typename T> class OptionVector : public ValueBase {
public:
    OptionVector(const std::string & names);

    std::vector<T> & operator* () { return m_values; }
    std::vector<T> & operator-> () { return &m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    std::vector<T> m_values; // the value
};

//===========================================================================
template <typename T>
inline OptionVector<T>::OptionVector(const std::string & names)
    : ValueBase{names, "", true, false} {}

//===========================================================================
template <>
inline OptionVector<bool>::OptionVector(const std::string & names)
    : ValueBase{names, "", true, true} {}

//===========================================================================
template <typename T>
inline bool OptionVector<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    T tmp;
    if (!(interpreter << value) || !(interpreter >> tmp) ||
        !(interpreter >> std::ws).eof()) {
        return false;
    }
    m_values.push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T>
inline void OptionVector<T>::resetValue() {
    m_values.clear();
}


/****************************************************************************
*
*   Argument
*
***/

template <typename T> class Argument : public ValueBase {
public:
    Argument(
        const std::string & name,
        const T & def = T{});

    T & operator* () { return m_value; }
    T * operator-> () { return &m_value; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    T m_value;
    T m_defValue;
};

//===========================================================================
template <typename T>
inline Argument<T>::Argument(
    const std::string & name, const T & def)
    : ValueBase{"", name, false, false}
    , m_value{def} 
    , m_defValue{def} {}

//===========================================================================
template <typename T>
inline bool Argument<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    if (!(interpreter << value) || !(interpreter >> m_value) ||
        !(interpreter >> std::ws).eof()) {
        m_value = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
inline void Argument<T>::resetValue() {
    m_value = m_defValue;
}


/****************************************************************************
*
*   ArgumentVector
*
***/

template <typename T> class ArgumentVector : public ValueBase {
public:
    ArgumentVector(const std::string & name);

    std::vector<T> & operator* () { return m_values; }
    std::vector<T> & operator-> () { return &m_values; }

private:
    bool parseValue(const std::string & value) override;
    void resetValue() override;

    std::vector<T> m_values; // the value
};

//===========================================================================
template <typename T>
inline ArgumentVector<T>::ArgumentVector(const std::string & name)
    : ValueBase{"", name, true, false} {}

//===========================================================================
template <typename T>
inline bool ArgumentVector<T>::parseValue(const std::string & value) {
    std::stringstream interpreter;
    T tmp;
    if (!(interpreter << value) || !(interpreter >> tmp) ||
        !(interpreter >> std::ws).eof()) {
        return false;
    }
    m_values.push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T>
inline void ArgumentVector<T>::resetValue() {
    m_values.clear();
}


/****************************************************************************
*
*   General
*
***/

bool parseOptions(size_t argc, char ** argv);

} // namespace
} // namespace
