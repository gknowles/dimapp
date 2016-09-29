// cmdline.cpp - dim services
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;
using namespace Dim::CmdLine;


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Parser
*
***/

class Dim::CmdLine::Parser {
public:
    void clear();
    void add(ValueBase & opt);
    bool parse(size_t argc, char ** argv);

private:
    map<char, ValueBase*> m_shortNames;
    map<string, ValueBase*> m_longNames;
    vector<ValueBase*> m_args;
};
static Parser s_parser;
static thread_local Parser * s_currentParser;

//===========================================================================
void Parser::clear() {
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
void Parser::add(ValueBase & opt) {
    istringstream is(opt.m_names);
    string name;
    while (is >> name) {
        if (name.size() == 1) {
            m_shortNames[name[0]] = &opt;
        } else {
            m_longNames[name] = &opt;
        }
    }
    if (!opt.m_refName.empty()) 
        m_args.push_back(&opt);
}

//===========================================================================
bool Parser::parse(size_t argc, char ** argv) {
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
        const char * ptr;
        if (**argv == '-' && moreOpts) {
            ptr = *argv + 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = m_shortNames.find(*ptr);
                if (it == m_shortNames.end()) {
                    logMsgError() << "Unknown option: -" << *ptr;
                    return false;
                }
                val = it->second;
                val->m_explicit = true;
                if (val->m_bool) {
                    val->parseValue("1");
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
            if (const char * equal = strchr(ptr, '=')) {
                key.assign(ptr, equal);
                ptr = equal + 1;
            } else {
                key = ptr;
                ptr = "";
            }
            auto it = m_longNames.find(key);
            if (it == m_longNames.end()) {
                logMsgError() << "Unknown option: --" << key;
                return false;
            }
            val = it->second;
            goto option_value;
        }

        // argument
        pos += 1;
        continue;

    option_value:
        if (*ptr) {
            if (!val->parseValue(ptr)) {
                logMsgError() << "Invalid option value: " << ptr;
                return false;
            }
            continue;
        }
        argc -= 1;
        argv += 1;
        if (!argc) {
            logMsgError() << "No value given for (" << val->m_names 
                << ") option";
            return false;
        }
        if (!val->parseValue(*argv)) {
            logMsgError() << "Invalid option value: " << *argv;
            return false;
        }
    }
    return true;
}

//===========================================================================
static Parser * parser() {
    return s_currentParser ? s_currentParser : &s_parser;
}


/****************************************************************************
*
*   ValueBase
*
***/

//===========================================================================
ValueBase::ValueBase(
    const std::string & names,
    const std::string & refName,
    bool multiple,
    bool boolean
) 
    : m_names{names}
    , m_refName{refName}
    , m_bool{boolean}
    , m_multiple{multiple}
{
    parser()->add(*this);
}

//===========================================================================
ValueBase::~ValueBase() {}


/****************************************************************************
*
*   Public Interface
*
***/

//===========================================================================
bool Dim::CmdLine::parseOptions(size_t argc, char * argv[]) {
    return parser()->parse(argc, argv);
}
