// cmdline.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Variables
*
***/

static CmdParser s_parser;
static thread_local CmdParser * s_currentParser;

//===========================================================================
static CmdParser * parser() {
    return s_currentParser ? s_currentParser : &s_parser;
}


/****************************************************************************
*
*   CmdParser::ValueBase
*
***/

//===========================================================================
CmdParser::ValueBase::ValueBase(
    CmdParser * p,
    const std::string & names,
    const std::string & refName,
    bool multiple,
    bool required,
    bool boolean)
    : m_names{names}
    , m_refName{refName}
    , m_bool{boolean}
    , m_multiple{multiple}
    , m_required(required) {
    if (!p)
        p = parser();
    p->add(*this);
}

//===========================================================================
CmdParser::ValueBase::~ValueBase() {}


/****************************************************************************
*
*   CmdParser
*
***/

//===========================================================================
void CmdParser::clear() {
    for (auto && val : m_shortNames) {
        val.second->m_explicit = false;
        val.second->resetValue();
    }
    for (auto && val : m_longNames) {
        val.second->m_explicit = false;
        val.second->resetValue();
    }
    for (auto && val : m_args) {
        val->m_explicit = false;
        val->resetValue();
    }
}

//===========================================================================
void CmdParser::add(ValueBase & opt) {
    istringstream is(opt.m_names);
    string name;
    while (is >> name) {
        if (name.size() == 1) {
            m_shortNames[name[0]] = &opt;
        } else {
            m_longNames[name] = &opt;
        }
    }
    if (!opt.m_refName.empty()) {
        if (!opt.m_required) {
            m_args.push_back(&opt);
        } else {
            auto where = find_if(m_args.begin(), m_args.end(), [](auto && val) {
                return !val->m_required;
            });
            m_args.insert(where, &opt);
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
    clear();

    // the 0th (name of this program) arg should always be present
    assert(argc && *argv);
    if (argc == 1)
        return true;

    unsigned pos = 0;
    bool moreOpts = true;
    argc -= 1;
    argv += 1;

    for (; argc; --argc, ++argv) {
        ValueBase * val{nullptr};
        const char * ptr = *argv;
        if (*ptr == '-' && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    os << "Unknown option: -" << *ptr << endl;
                    return false;
                }
                val = it->second;
                if (val->m_bool) {
                    parseValue(*val, "1");
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
            val = it->second;
            if (val->m_bool) {
                if (equal) {
                    os << "Unknown option: --" << key << "=" << endl;
                    return false;
                }
                parseValue(*val, hasPrefix ? "0" : "1");
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
        if (!parseValue(*m_args[pos], ptr)) {
            os << "Invalid " << m_args[pos]->m_refName << ": " << ptr;
            return false;
        }
        if (!m_args[pos]->m_multiple)
            pos += 1;
        continue;

    option_value:
        if (*ptr) {
            if (!parseValue(*val, ptr)) {
                os << "Invalid option value: " << ptr << endl;
                return false;
            }
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            os << "No value given for (" << val->m_names << ") option" << endl;
            return false;
        }
        if (!parseValue(*val, *argv)) {
            os << "Invalid option value: " << *argv << endl;
            return false;
        }
    }

    if (pos < size(m_args) && m_args[pos]->m_required) {
        os << "No value give for " << m_args[pos]->m_refName << endl;
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
bool Dim::cmdParse(size_t argc, char * argv[]) {
    return parser()->parse(cerr, argc, argv);
}
