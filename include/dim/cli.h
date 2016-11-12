// cli.h - dim cli
//
// For documentation and examples follow the links at:
// https://github.com/gknowles/dimcli

#pragma once

#include "config.h"

#include "util.h"

#include <cassert>
#include <experimental/filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Dim {

// forward declarations
class Cli;


/****************************************************************************
*
*   Exit codes
*
***/

// These mirror the program exit codes defined in <sysexits.h>
enum {
    kExitOk = 0,        // EX_OK
    kExitUsage = 64,    // EX_USAGE     - bad command line
    kExitSoftware = 70, // EX_SOFTWARE  - internal software error
};


/****************************************************************************
*
*   CliBase
*
***/

class CliBase {
public:
    class ArgBase;
    template <typename A, typename T> class ArgShim;
    template <typename T> class Arg;
    template <typename T> class ArgVec;

    struct ArgMatch;
    template <typename T> struct Value;
    template <typename T> struct ValueVec;

    class Group;

protected:
    template <typename A> auto & getProxy(A & arg) { return arg.m_proxy; }
};


/****************************************************************************
*
*   CliBase::Group
*
***/

class CliBase::Group : public CliBase {
public:
    Group(Cli & cli, const std::string & name);

    // assignment only changes the contents, not the owner
    Group & operator=(const Group & from);

    //-----------------------------------------------------------------------
    // Configuration

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Group & title(const std::string & desc);

    // Option groups are sorted by key, defaults to group name
    Group & sortKey(const std::string & key);

    template <
        typename T,
        typename U,
        typename = enable_if<is_convertible<U, T>::value>::type>
        Arg<T> & arg(T * value, const std::string & keys, const U & def);

    template <typename T> Arg<T> & arg(T * value, const std::string & keys);

    template <typename T>
    ArgVec<T> &
        argVec(std::vector<T> * values, const std::string & keys, int nargs = -1);

    template <typename T>
    Arg<T> & arg(const std::string & keys, const T & def = {});

    template <typename T>
    ArgVec<T> & argVec(const std::string & keys, int nargs = -1);

    template <
        typename T,
        typename U,
        typename = enable_if<is_convertible<U, T>::value>::type>
        Arg<T> & arg(Arg<T> & value, const std::string & keys, const U & def);

    template <typename T>
    Arg<T> & arg(Arg<T> & value, const std::string & keys);

    template <typename T>
    ArgVec<T> &
        argVec(ArgVec<T> & values, const std::string & keys, int nargs = -1);

    //-----------------------------------------------------------------------
    // Queries
    const std::string & title() const { return m_title; }
    const std::string & sortKey() const { return m_sortKey; }

private:
    Cli & m_cli;
    std::string m_name;
    std::string m_title;
    std::string m_sortKey;
};

//===========================================================================
template <typename T, typename U, typename>
inline CliBase::Group::Arg<T> &
CliBase::Group::arg(T * value, const std::string & keys, const U & def) {
    auto proxy = m_cli.getProxy<Arg<T>, Value<T>>(value);
    auto ptr = std::make_unique<Arg<T>>(proxy, keys, def);
    return m_cli.addArg(std::move(ptr)).group(m_name);
}

//===========================================================================
template <typename T>
inline CliBase::Group::Arg<T> &
CliBase::Group::arg(T * value, const std::string & keys) {
    return arg(value, keys, T{});
}

//===========================================================================
template <typename T>
inline CliBase::Group::ArgVec<T> & CliBase::Group::argVec(
    std::vector<T> * values,
    const std::string & keys,
    int nargs) {
    auto proxy = m_cli.getProxy<ArgVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<ArgVec<T>>(proxy, keys, nargs);
    return m_cli.addArg(std::move(ptr)).group(m_name);
}

//===========================================================================
template <typename T, typename U, typename>
inline CliBase::Group::Arg<T> & CliBase::Group::arg(
    CliBase::Group::Arg<T> & alias,
    const std::string & keys,
    const U & def) {
    return arg(&*alias, keys, def);
}

//===========================================================================
template <typename T>
inline CliBase::Group::Arg<T> &
CliBase::Group::arg(Arg<T> & alias, const std::string & keys) {
    return arg(&*alias, keys, T{});
}

//===========================================================================
template <typename T>
inline CliBase::Group::ArgVec<T> & CliBase::Group::argVec(
    ArgVec<T> & alias,
    const std::string & keys,
    int nargs) {
    return argVec(&*alias, keys, nargs);
}

//===========================================================================
template <typename T>
inline CliBase::Group::Arg<T> &
CliBase::Group::arg(const std::string & keys, const T & def) {
    return arg<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
inline CliBase::Group::ArgVec<T> &
CliBase::Group::argVec(const std::string & keys, int nargs) {
    return argVec<T>(nullptr, keys, nargs);
}


/****************************************************************************
*
*   Cli
*
***/

class Cli : public CliBase::Group {
public:
    Cli();

    //-----------------------------------------------------------------------
    // Configuration

    // Add --version argument that shows "${progName.stem()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Arg<bool> &
        versionArg(const std::string & ver, const std::string & progName = {});

    // Create a new option group, that you can then start stuffing args into.
    Group & group(const std::string & name);
    const Group & group(const std::string & name) const;

    // Enabled by default, reponse file expansion replaces arguments of the 
    // form "@file" with the contents of the file.
    void responseFiles(bool enable = true);

    //-----------------------------------------------------------------------
    // Parsing

    bool parse(size_t argc, char * argv[]);
    bool parse(std::ostream & os, size_t argc, char * argv[]);

    // "args" is non-const so response files can be expanded in place
    bool parse(std::vector<std::string> & args);
    bool parse(std::ostream & os, std::vector<std::string> & args);

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(const std::string & cmdline);
    // Copy array of pointers into vector of args
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    // Copy array of wchar_t pointers into vector of utf-8 encoded args
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);
    // Create vector of pointers suitable for use with argc/argv APIs, includes
    // trailing null, so use "size() - 1" for argc. The return values point
    // into the source string vector and are only valid until that vector is
    // resized or destroyed.
    static std::vector<const char *>
        toPtrArgv(const std::vector<std::string> & args);

    // Parse according to glib conventions, based on the UNIX98 shell spec
    static std::vector<std::string> toGlibArgv(const std::string & cmdline);
    // Parse using GNU conventions, same rules as buildargv()
    static std::vector<std::string> toGnuArgv(const std::string & cmdline);
    // Parse using Windows rules
    static std::vector<std::string> toWindowsArgv(const std::string & cmdline);

    void resetValues();

    // Intended for use from return statements in action callbacks. Sets
    // exit code (to EX_USAGE) and error msg, then returns false.
    bool badUsage(const std::string & msg) { return fail(kExitUsage, msg); }

    //-----------------------------------------------------------------------
    // Inspection after parsing

    int exitCode() const { return m_exitCode; };
    const std::string & errMsg() const { return m_errMsg; }

    // Program name received in argv[0]
    const std::string & progName() const { return m_progName; }

    // writeHelp & writeUsage return the current exitCode()
    int writeHelp(std::ostream & os, const std::string & progName = {}) const;
    int writeUsage(std::ostream & os, const std::string & progName = {}) const;

    void writePositionals(std::ostream & os) const;
    void writeOptions(std::ostream & os) const;

private:
    friend class CliBase::Group;

    bool defaultAction(ArgBase & arg, const std::string & val);

    void addLongName(
        const std::string & name,
        ArgBase * arg,
        bool invert,
        bool optional);
    void addArgName(const std::string & name, ArgBase * arg);
    void addArg(std::unique_ptr<ArgBase> ptr);

    template <typename Arg, typename Value, typename Ptr>
    std::shared_ptr<Value> getProxy(Ptr * ptr);
    template <typename A> A & addArg(std::unique_ptr<A> ptr);

    bool parseAction(
        ArgBase & out,
        const std::string & name,
        int pos,
        const char src[]);
    bool fail(int code, const std::string & msg);

    std::string optionList(ArgBase & arg) const;
    std::string optionList(ArgBase & arg, bool disableOptions) const;

    struct ArgName {
        ArgBase * arg;
        bool invert;      // set to false instead of true (only for bools)
        bool optional;    // value doesn't have to be present? (non-bools only)
        std::string name; // name of argument (only for positionals)
    };
    std::list<std::unique_ptr<ArgBase>> m_args;
    std::map<char, ArgName> m_shortNames;
    std::map<std::string, ArgName> m_longNames;
    std::vector<ArgName> m_argNames;

    std::map<std::string, Group> m_groups;
    bool m_responseFiles{true};

    int m_exitCode{0};
    std::string m_errMsg;
    std::string m_progName;
};

//===========================================================================
template <typename Arg, typename Value, typename Ptr>
inline std::shared_ptr<Value> Cli::getProxy(Ptr * ptr) {
    if (ptr) {
        for (auto && a : m_args) {
            auto ap = dynamic_cast<Arg *>(a.get());
            if (ap && &**ap == ptr)
                return CliBase::getProxy<Arg>(*ap);
        }
    }

    // Since there was no pre-existing proxy to raw value, create new proxy.
    return std::make_shared<Value>(ptr);
}

//===========================================================================
template <typename A> inline A & Cli::addArg(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.action(&Cli::defaultAction);
    addArg(std::unique_ptr<ArgBase>(ptr.release()));
    return opt;
}


/****************************************************************************
*
*   CliBase::ArgBase
*
***/

class CliBase::ArgBase {
public:
    ArgBase(const std::string & keys, bool boolean);
    virtual ~ArgBase() {}

    // Name of the argument that populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual const std::string & from() const = 0;

    // Absolute position in argv[] of the argument that populated the value,
    // or an empty string if it wasn't populated. For vectors, it's what
    // populated the last value.
    virtual int pos() const = 0;

    // set to passed in default
    virtual void reset() = 0;

    // parses the string into the value, returns false on error
    virtual bool parseValue(const std::string & value) = 0;

    // set to (or add to vec) value for missing optional
    virtual void unspecifiedValue() = 0;

    // number of values, non-vecs are always 1
    virtual size_t size() const = 0;

protected:
    virtual bool parseAction(Cli & cli, const std::string & value) = 0;
    virtual void set(const std::string & name, int pos) = 0;

    template <typename T> void setValueName();

    std::string m_group;
    std::string m_desc;
    std::string m_valueDesc;

    // Are multiple values are allowed, and how many there can be (-1 for
    // unlimited).
    bool m_multiple{false};
    int m_nargs{1};

    // the value is a bool on the command line (no separate value)?
    bool m_bool{false};

    bool m_flagValue{false};
    bool m_flagDefault{false};

private:
    friend class Cli;
    std::string m_names;
};

//===========================================================================
template <typename T> inline void CliBase::ArgBase::setValueName() {
    if (is_integral<T>::value) {
        m_valueDesc = "NUM";
    } else if (is_convertible<T, std::string>::value) {
        m_valueDesc = "STRING";
    } else {
        m_valueDesc = "VALUE";
    }
}

//===========================================================================
template <>
inline void
CliBase::ArgBase::setValueName<std::experimental::filesystem::path>() {
    m_valueDesc = "FILE";
}


/****************************************************************************
*
*   CliBase::ArgShim
*
***/

template <typename A, typename T> class CliBase::ArgShim : public ArgBase {
public:
    ArgShim(const std::string & keys, bool boolean);
    ArgShim(const ArgShim &) = delete;
    ArgShim & operator=(const ArgShim &) = delete;

    // Set group under which this argument will show up in the help text.
    A & group(const std::string & val);

    // Set desciption to associate with the argument in writeHelp()
    A & desc(const std::string & val);

    // Set name of meta-variable in writeHelp. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & valueDesc(const std::string & val);

    // Allows the default to be changed after the arg has been created.
    A & defaultValue(const T & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(const T & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // a default will be choosen for you.
    A & flagValue(bool isDefault = false);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - call cli.badUsage() with an error message if there's a problem
    //  - return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use arg.from() and arg.pos() to get the argument name that the
    // value was attached to on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide a std::istream
    // extraction (>>) or assignment from std::string operator and the
    // default action will pick it up.
    using ActionFn = bool(Cli & cli, A & arg, const std::string & src);
    A & action(std::function<ActionFn> fn);

protected:
    bool parseAction(Cli & cli, const std::string & value) override;

    std::function<ActionFn> m_action;
    T m_implicitValue{};
    T m_defValue{};
};

//===========================================================================
template <typename A, typename T>
inline CliBase::ArgShim<A, T>::ArgShim(const std::string & keys, bool boolean)
    : ArgBase(keys, boolean) {
    setValueName<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool
CliBase::ArgShim<A, T>::parseAction(Cli & cli, const std::string & val) {
    auto self = static_cast<A *>(this);
    return m_action(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::group(const std::string & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::desc(const std::string & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::valueDesc(const std::string & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::defaultValue(const T & val) {
    m_defValue = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::implicitValue(const T & val) {
    if (m_bool) {
        // they don't have separate values, just their presence/absence
        assert(!m_bool && "bool argument values are never implicit");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::flagValue(bool isDefault) {
    auto self = static_cast<A *>(this);
    m_flagValue = true;
    if (!self->m_proxy->m_defFlagArg) {
        self->m_proxy->m_defFlagArg = self;
        m_flagDefault = true;
    } else if (isDefault) {
        self->m_proxy->m_defFlagArg->m_flagDefault = false;
        self->m_proxy->m_defFlagArg = self;
        m_flagDefault = true;
    } else {
        m_flagDefault = false;
    }
    m_bool = true;
    return *self;
}

//===========================================================================
template <typename A, typename T>
inline A & CliBase::ArgShim<A, T>::action(std::function<ActionFn> fn) {
    m_action = fn;
    return static_cast<A &>(*this);
}


/****************************************************************************
*
*   CliBase::ArgMatch
*
***/

struct CliBase::ArgMatch {
    // name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // member of argv[] that populated the value or 0 if it wasn't.
    int pos{0};
};


/****************************************************************************
*
*   CliBase::Value
*
***/

template <typename T> struct CliBase::Value {
    // where the value came from
    ArgMatch m_match;

    // the value was explicitly set
    bool m_explicit{false};

    // points to the arg with the default flag value
    Arg<T> * m_defFlagArg{nullptr};

    T * m_value{nullptr};
    T m_internal;

    Value(T * value)
        : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   CliBase::Arg
*
***/

template <typename T> class CliBase::Arg : public ArgShim<Arg<T>, T> {
public:
    Arg(std::shared_ptr<Value<T>> value,
        const std::string & keys,
        const T & def = {});

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // True if the value was populated from the command line and while the
    // value may be the same as the default it wasn't simply left that way.
    explicit operator bool() const { return m_proxy->m_explicit; }

    // ArgBase
    const std::string & from() const override { return m_proxy->m_match.name; }
    int pos() const override { return m_proxy->m_match.pos; }
    void reset() override;
    bool parseValue(const std::string & value) override;
    void unspecifiedValue() override;
    size_t size() const override;

private:
    friend class CliBase;
    void set(const std::string & name, int pos) override;

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
inline CliBase::Arg<T>::Arg(
    std::shared_ptr<Value<T>> value,
    const std::string & keys,
    const T & def)
    : ArgShim<Arg, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value} {
    m_defValue = def;
}

//===========================================================================
template <typename T>
inline void CliBase::Arg<T>::set(const std::string & name, int pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = pos;
    m_proxy->m_explicit = true;
}

//===========================================================================
template <typename T> inline void CliBase::Arg<T>::reset() {
    if (!m_flagValue || m_flagDefault)
        *m_proxy->m_value = m_defValue;
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline bool CliBase::Arg<T>::parseValue(const std::string & value) {
    if (m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            *m_proxy->m_value = m_defValue;
        return true;
    }

    return stringTo(*m_proxy->m_value, value);
}

//===========================================================================
template <typename T> inline void CliBase::Arg<T>::unspecifiedValue() {
    *m_proxy->m_value = m_implicitValue;
}

//===========================================================================
template <typename T> inline size_t CliBase::Arg<T>::size() const {
    return 1;
}


/****************************************************************************
*
*   CliBase::ValueVec
*
***/

template <typename T> struct CliBase::ValueVec {
    // where the values came from
    std::vector<ArgMatch> m_matches;

    // points to the arg with the default flag value
    ArgVec<T> * m_defFlagArg{nullptr};

    std::vector<T> * m_values{nullptr};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   CliBase::ArgVec
*
***/

template <typename T> class CliBase::ArgVec : public ArgShim<ArgVec<T>, T> {
public:
    ArgVec(
        std::shared_ptr<ValueVec<T>> values,
        const std::string & keys,
        int nargs);

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    // True if values where added from the command line
    explicit operator bool() const { return !m_proxy->m_values->empty(); }

    // ArgBase
    const std::string & from() const override { return from(size() - 1); }
    int pos() const override { return pos(size() - 1); }
    void reset() override;
    bool parseValue(const std::string & value) override;
    void unspecifiedValue() override;
    size_t size() const override;

    // Information about a specific member of the vector of values
    const std::string & from(size_t index) const;
    int pos(size_t index) const;

private:
    friend class CliBase;
    void set(const std::string & name, int pos) override;

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;
};

//===========================================================================
template <typename T>
inline CliBase::ArgVec<T>::ArgVec(
    std::shared_ptr<ValueVec<T>> values,
    const std::string & keys,
    int nargs)
    : ArgShim<ArgVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values) {
    m_multiple = true;
    m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline void CliBase::ArgVec<T>::set(const std::string & name, int pos) {
    ArgMatch match;
    match.name = name;
    match.pos = pos;
    m_proxy->m_matches.push_back(match);
}

//===========================================================================
template <typename T>
inline const std::string & CliBase::ArgVec<T>::from(size_t index) const {
    return index >= size() ? m_empty : m_proxy->m_matches[index].name;
}

//===========================================================================
template <typename T> inline int CliBase::ArgVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

//===========================================================================
template <typename T> inline void CliBase::ArgVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline bool CliBase::ArgVec<T>::parseValue(const std::string & value) {
    if (m_flagValue) {
        bool flagged;
        if (!stringTo(flagged, value))
            return false;
        if (flagged)
            m_proxy->m_values->push_back(m_defValue);
        return true;
    }

    T tmp;
    if (!stringTo(tmp, value))
        return false;
    m_proxy->m_values->push_back(std::move(tmp));
    return true;
}

//===========================================================================
template <typename T> inline void CliBase::ArgVec<T>::unspecifiedValue() {
    m_proxy->m_values->push_back(m_implicitValue);
}

//===========================================================================
template <typename T> inline size_t CliBase::ArgVec<T>::size() const {
    return m_proxy->m_values->size();
}

} // namespace
