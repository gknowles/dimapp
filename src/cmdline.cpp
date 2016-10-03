// cmdline.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   CmdParser::ValueBase
*
***/

//===========================================================================
CmdParser::ValueBase::ValueBase(
    const std::string & names,
    const std::string & refName,
    bool multiple,
    bool required,
    bool boolean)
    : m_names{names}
    , m_refName{refName}
    , m_bool{boolean}
    , m_multiple{multiple}
    , m_required(required) {}


/****************************************************************************
*
*   CmdParser
*
***/

//===========================================================================
void CmdParser::resetValues() {
    for (auto && kv : m_shortNames) {
        auto val = kv.second.val;
        val->m_explicit = false;
        val->m_refName.clear();
        val->resetValue();
    }
    for (auto && kv : m_longNames) {
        auto val = kv.second.val;
        val->m_explicit = false;
        val->m_refName.clear();
        val->resetValue();
    }
    for (auto && v : m_args) {
        auto val = v.val;
        val->m_explicit = false;
        val->resetValue();
    }
}

//===========================================================================
void CmdParser::addValue(std::unique_ptr<ValueBase> ptr) {
    auto & opt = *ptr;
    m_values.push_back(std::move(ptr));
    istringstream is(opt.m_names);
    string name;
    ValueKey key = {&opt, false};
    while (is >> name) {
        if (name.size() == 1) {
            m_shortNames[name[0]] = key;
        } else if (name.size() == 2 && name[0] == '!' && opt.m_bool) {
            m_shortNames[name[1]] = {&opt, true};
        } else if (name[0] == '!' && opt.m_bool) {
            m_longNames[name.data() + 1] = {&opt, true};
        } else {
            m_longNames[name] = key;
        }
    }
    if (!opt.m_refName.empty()) {
        if (!opt.m_required) {
            m_args.push_back(key);
        } else {
            auto where =
                find_if(m_args.begin(), m_args.end(), [](auto && vkey) {
                    return !vkey.val->m_required;
                });
            m_args.insert(where, key);
        }
    }
}

//===========================================================================
bool CmdParser::parseValue(ValueBase & val, const char ptr[]) {
    val.m_explicit = true;
    return val.parseValue(ptr);
}

//===========================================================================
bool CmdParser::parse(ostream & os, size_t argc, char ** argv) {
    resetValues();

    // the 0th (name of this program) arg should always be present
    assert(argc && *argv);
    if (argc == 1)
        return true;

    unsigned pos = 0;
    bool moreOpts = true;
    argc -= 1;
    argv += 1;

    for (; argc; --argc, ++argv) {
        ValueKey vkey;
        const char * ptr = *argv;
        if (*ptr == '-' && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    os << "Unknown option: -" << *ptr << endl;
                    return false;
                }
                vkey = it->second;
                vkey.val->m_refName = string("-") + *ptr;
                if (vkey.val->m_bool) {
                    parseValue(*vkey.val, vkey.invert ? "0" : "1");
                    continue;
                }
                ptr += 1;
                goto option_value;
            }
            if (!*ptr) {
                continue;
            }
            ptr += 1;
            if (!*ptr) {
                // bare "--" found, all remaining args are positional
                moreOpts = false;
                continue;
            }
            string key;
            const char * equal = strchr(ptr, '=');
            if (equal) {
                key.assign(ptr, equal);
                ptr = equal + 1;
            } else {
                key = ptr;
                ptr = "";
            }
            auto it = m_longNames.find(key);
            bool hasPrefix = false;
            while (it == m_longNames.end()) {
                if (key.size() > 3 && key.compare(0, 3, "no-") == 0) {
                    hasPrefix = true;
                    it = m_longNames.find(key.data() + 3);
                    continue;
                }
                os << "Unknown option: --" << key << endl;
                return false;
            }
            vkey = it->second;
            vkey.val->m_refName = "--" + key;
            if (vkey.val->m_bool) {
                if (equal) {
                    os << "Unknown option: --" << key << "=" << endl;
                    return false;
                }
                parseValue(*vkey.val, (hasPrefix ^ vkey.invert) ? "0" : "1");
                continue;
            } else if (hasPrefix) {
                os << "Unknown option: --" << key << endl;
                return false;
            }
            goto option_value;
        }

        // argument
        if (pos >= size(m_args)) {
            os << "Unexpected argument: " << ptr << endl;
            return false;
        }
        vkey = m_args[pos];
        if (!parseValue(*vkey.val, ptr)) {
            os << "Invalid " << vkey.val->m_refName << ": " << ptr;
            return false;
        }
        if (!vkey.val->m_multiple)
            pos += 1;
        continue;

    option_value:
        if (*ptr) {
            if (!parseValue(*vkey.val, ptr)) {
                os << "Invalid option value: " << ptr << endl;
                return false;
            }
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            os << "No value given for " << vkey.val->m_refName << endl;
            return false;
        }
        if (!parseValue(*vkey.val, *argv)) {
            os << "Invalid option value: " << *argv << endl;
            return false;
        }
    }

    if (pos < size(m_args) && m_args[pos].val->m_required) {
        os << "No value given for " << m_args[pos].val->m_refName << endl;
        return false;
    }
    return true;
}


/****************************************************************************
*
*   Public Interface
*
***/

//===========================================================================
ostream & Dim::operator<<(ostream & os, const CmdParser::ValueBase & val) {
    return os << val.name();
}
