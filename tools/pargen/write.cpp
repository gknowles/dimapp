// write.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static void writeRuleName(ostream & os, const string & name, bool capitalize);
static void writeElement(ostream & os, const Element & elem, bool inclPos);

//===========================================================================
ostream & operator<<(ostream & os, const Element & elem) {
    writeElement(os, elem, true);
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const set<Element> & rules) {
    for (auto && rule : rules) {
        os << "*   " << rule.name;
        os << " = " << rule;
        os << '\n';
    }
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StateElement & se) {
    os << *se.elem << '.' << se.rep;
    if (se.started)
        os << " (STARTED)";
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StateEvent & sv) {
    os << "on";
    writeRuleName(os, sv.elem->name, true);
    switch (sv.flags) {
    case Element::kOnChar: os << "Char"; break;
    case Element::kOnEnd: os << "End"; break;
    case Element::kOnStart: os << "Start"; break;
    }
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StatePosition & sp) {
    os << "    // sp";
    if (sp.recurse)
        os << " (RECURSE)";
    os << ':';
    for (auto && sv : sp.events)
        os << "\n    //  " << sv;
    if (!sp.delayedEvents.empty()) {
        os << "\n    //  --- delayed";
        for (auto && sv : sp.delayedEvents)
            os << "\n    //  " << sv;
    }
    if (!sp.events.empty() || !sp.delayedEvents.empty())
        os << "\n    //  ---";
    for (auto && se : sp.elems) {
        os << "\n    //  " << se;
    }
    os << "\n";
    return os;
}

//===========================================================================
static void writeElement(ostream & os, const Element & elem, bool inclPos) {
    if (elem.m != 1 || elem.n != 1) {
        if (elem.m)
            os << elem.m;
        os << '*';
        if (elem.n != kUnlimited)
            os << elem.n;
    }
    bool first = true;
    switch (elem.type) {
    case Element::kChoice:
        os << "( ";
        for (auto && e : elem.elements) {
            if (first) {
                first = false;
            } else {
                os << " / ";
            }
            writeElement(os, e, inclPos);
        }
        os << " )";
        break;
    case Element::kRule: os << elem.value; break;
    case Element::kSequence:
        os << "( ";
        for (auto && e : elem.elements) {
            if (first) {
                first = false;
            } else {
                os << " ";
            }
            writeElement(os, e, inclPos);
        }
        os << " )";
        break;
    case Element::kTerminal:
        os << "%x" << hex << (unsigned)(unsigned char)elem.value[0] << dec;
        break;
    }

    if (elem.flags || elem.recurse) {
        bool first = true;
        os << "  { ";
        if (elem.flags & Element::kOnStart) {
            first = false;
            os << "Start";
        }
        if (elem.flags & Element::kOnEnd) {
            if (!first)
                os << ", ";
            first = false;
            os << "End";
        }
        if (elem.flags & Element::kOnChar) {
            if (!first)
                os << ", ";
            first = false;
            os << "Char";
        }
		if (elem.recurse) {
            if (!first)
                os << ", ";
            first = false;
            os << "Function";
		}
        os << " }";
    }
    if (inclPos && elem.pos)
        os << '#' << elem.pos;
}

//===========================================================================
// Remove leading/trailing spaces and replace multiple spaces with one.
// Replace spaces, with NL followed by the prefix, if waiting until the next
// space would go past the maxWidth.
static void writeWordwrap(
    ostream & os,
    size_t & indent,
    const string & str,
    size_t maxWidth,
    const string & prefix) {
    const char * base = str.c_str();
    for (;;) {
        while (*base == ' ')
            base += 1;
        if (!*base)
            return;
        const char * ptr = strchr(base, ' ');
        if (!ptr) {
            ptr = str.c_str() + str.size();
            while (ptr >= base && ptr[-1] == ' ')
                ptr -= 1;
        }
        size_t len = ptr - base;
        if (indent + len >= maxWidth) {
            os << '\n' << prefix;
            indent = size(prefix);
        }
        if (*ptr) {
            os.write(base, len + 1);
            base = ptr + 1;
        } else {
            os.write(base, len);
            os << ' ';
            base = ptr;
        }
        indent += len + 1;
    }
}

//===========================================================================
static void writeRule(
    ostream & os,
    const Element & rule,
    size_t maxWidth,
    const string & prefix) {
    streampos pos = os.tellp();
    os << prefix << rule.name;
    os << " = ";
    size_t indent = os.tellp() - pos;
    ostringstream raw;
    writeElement(raw, rule, false);
    string vpre = prefix + "    ";
    writeWordwrap(os, indent, raw.str(), maxWidth, vpre);
    os << '\n';
}

//===========================================================================
static void writeRuleName(ostream & os, const string & name, bool capitalize) {
    for (auto && ch : name) {
        if (ch == '-') {
            capitalize = true;
        } else {
            os << (capitalize ? (char)toupper(ch) : ch);
            capitalize = false;
        }
    }
}

//===========================================================================
static bool writeSwitchCase(ostream & os, const State & st) {
    if (st.next.empty())
        return false;
    struct NextState {
        unsigned char ch;
        unsigned state;
    };
    vector<NextState> cases;
    for (unsigned i = 0; i < 256; ++i) {
        if (st.next[i])
            cases.push_back({unsigned char(i), st.next[i]});
    }
    if (cases.empty())
        return false;

    sort(
        cases.begin(),
        cases.end(),
        [](const NextState & e1, const NextState & e2) {
            return 256 * e1.state + e1.ch < 256 * e2.state + e2.ch;
        });
    const unsigned kCaseColumns = 6;
    os << "    ch = *ptr++;\n";
    os << "    switch (ch) {\n";
    unsigned prev = cases.front().state;
    unsigned pos = 0;
    for (auto && ns : cases) {
        if (ns.state != prev) {
            if (pos % kCaseColumns != 0)
                os << '\n';
            os << "        goto state" << prev << ";\n";
            prev = ns.state;
            pos = 0;
        }
        if (pos % kCaseColumns == 0)
            os << "    ";
        os << "case ";
        if (ns.ch < ' ' || ns.ch >= 0x7f) {
            os << (unsigned)ns.ch;
        } else {
            os << '\'';
            if (ns.ch == '\'' || ns.ch == '\\')
                os << '\\';
            os << ns.ch << '\'';
        }
        os << ':';
        if (++pos % kCaseColumns == 0) {
            os << '\n';
        } else {
            os << ' ';
        }
    }
    if (pos % kCaseColumns != 0)
        os << '\n';

    os << "        goto state" << cases.back().state << ";\n"
       << "    }\n";
    return true;
}

//===========================================================================
static void writeEventCallback(
    ostream & os,
    const string & name,
    Element::Flags type,
    const char * args = nullptr,
    const string & prefix = "    ") {
    os << prefix << "m_notify->on";
    writeRuleName(os, name, true);
    switch (type) {
    case Element::kOnChar: os << "Char(" << (args ? args : "ch"); break;
    case Element::kOnEnd: os << "End(" << (args ? args : "ptr"); break;
    case Element::kOnStart: os << "Start(" << (args ? args : "ptr - 1"); break;
    }
    os << ");\n";
}

//===========================================================================
static void writeStateName(
    ostream & os, const string & name, size_t maxWidth, const string & prefix) {
    size_t space = maxWidth - size(prefix);
    size_t count = name.size();
    size_t pos = 0;
    size_t num = min({space, count});
    os << prefix;
    for (;;) {
        os.write(name.data() + pos, num);
        if (name[pos + num - 1] == '\\') {
            // extra space because a trailing backslash would cause the comment
            // to be extended to the next line by the c++ compiler
            os << ' ';
        }
        pos += num;
        count -= num;
        if (!count)
            break;
        os << '\n' << prefix << "  ";
        num = min({space - 2, count});
    }
    os << '\n';
}

//===========================================================================
static void writeParserState(
    ostream & os,
    const State & st,
    const Element * root,
    bool inclStatePositions) {
    os << "\nstate" << st.id << ":\n";
    vector<string> aliases = st.aliases;
    aliases.push_back(to_string(st.id) + ": " + st.name);
    sort(aliases.begin(), aliases.end(), [](auto && a, auto && b) {
        return strtoul(a.c_str(), nullptr, 10) <
               strtoul(b.c_str(), nullptr, 10);
    });
    for (auto && name : aliases)
        writeStateName(os, name, 79, "    // ");
    if (inclStatePositions) {
        os << "    // positions: " << st.positions.size() << "\n\n";
        for (auto && sp : st.positions) {
            os << sp << '\n';
        }
    }

    // execute notifications
    for (auto && sv : st.positions.begin()->events) {
        writeEventCallback(os, sv.elem->name, Element::Flags(sv.flags));
    }

    if (st.name == kDoneStateName) {
        if (root && (root->flags & Element::kOnEnd)) {
            writeEventCallback(os, root->name, Element::kOnEnd);
        }
        os << "    return true;\n";
        return;
    }
    auto & elems = st.positions.begin()->elems;
    if (elems.size() == 1 && elems.front().elem == &ElementDone::s_elem) {
        os << "    goto state1;\n";
        return;
    }

    if (root && !st.next.empty() && st.next[0]) {
        os << "    last = ptr;\n";
    }

    // write switch case
    bool hasSwitch = writeSwitchCase(os, st);

    // write calls to independent sub-state parsers
    bool hasCalls = false;
    for (auto && sp : st.positions) {
        const Element * elem = sp.elems.back().elem;
        if (elem->type != Element::kTerminal && elem != &ElementDone::s_elem) {
            hasCalls = true;
            assert(elem->type == Element::kRule);
            assert(elem->rule->recurse);
            for (auto && sv : sp.delayedEvents) {
                writeEventCallback(os, sv.elem->name, Element::Flags(sv.flags));
            }
            os << "    if (state";
            writeRuleName(os, elem->rule->name, true);
            os << "(";
            if (hasSwitch)
                os << "--";
            unsigned id = st.next.empty() ? 0 : st.next[256];
            os << "ptr)) {\n"
               << "        goto state" << id << ";\n"
               << "    }\n";
        }
    }

    os << "    goto state0;\n";
}

//===========================================================================
static void writeCppfileStart(
    ostream & os, const set<Element> & rules, const set<Element> & options) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    os << 1 + R"(
// )" << getOptionString(options, kOptionApiCppFile)
       << R"(
// Generated by pargen at )"
       << put_time(&tm, "%FT%T%z") << R"(
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   )" << getOptionString(options, kOptionApiPrefix)
       << R"(Parser
*
*   Normalized ABNF of syntax:
)";

    for (auto && elem : rules) {
        writeRule(os, elem, 79, "*   ");
    }

    os << 1 + R"(
*
***/
)";
}

//===========================================================================
static void writeFunction(
    ostream & os,
    const Element * root,
    const set<Element> & rules,
    const unordered_set<State> & stateSet,
    const set<Element> & options,
    bool inclStatePositions) {
    string prefix = getOptionString(options, kOptionApiPrefix);
    os << 1 + R"(

//===========================================================================
bool )" << prefix
       << "Parser::";
    if (!root) {
        os << R"(parse (const char src[]) {
    const char * ptr = src;
    char ch;
    goto state2;

state0: 
    // )" << kFailedStateName
           << R"(
    return false;
)";
    } else {
        os << "state";
        writeRuleName(os, root->name, true);
        os << R"( (const char *& ptr) {
    const char * last{nullptr};
    char ch;
)";
        if (root->flags & Element::kOnStart) {
            writeEventCallback(os, root->name, Element::kOnStart, "ptr");
        }
        os << R"(    goto state2;

state0:
    // )" << kFailedStateName
           << R"(
    if (last) {
        ptr = last;
        goto state1;
    }
    return false;
)";
    }
    vector<const State *> states;
    states.resize(stateSet.size());
    for (auto && st : stateSet) {
        if (st.id) {
            if (st.id > states.size())
                states.resize(st.id);
            states[st.id - 1] = &st;
        }
    }
    for (auto && st : states) {
        if (st)
            writeParserState(os, *st, root, inclStatePositions);
    }
    os << "}\n";
}

//===========================================================================
static void writeHeaderfile(
    ostream & os, const set<Element> & rules, const set<Element> & options) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    string prefix = getOptionString(options, kOptionApiPrefix);
    os << 1 + R"(
// )" << getOptionString(options, kOptionApiHeaderFile)
       << R"(
// Generated by pargen at )"
       << put_time(&tm, "%FT%T%z") << R"(
// clang-format off
#pragma once


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to make process parsed results
*
***/

class I)"
       << prefix << R"(ParserNotify {
public:
    virtual ~I)"
       << prefix << R"(ParserNotify () {}

    virtual bool onStart () { return true; }
    virtual bool onEnd () { return true; }

)";
    for (auto && elem : rules) {
        if (elem.flags & Element::kOnStart) {
            os << "    virtual bool on";
            writeRuleName(os, elem.name, true);
            os << "Start (const char * ptr) { return true; }\n";
        }
        if (elem.flags & Element::kOnChar) {
            os << "    virtual bool on";
            writeRuleName(os, elem.name, true);
            os << "Char (char ch) { return true; }\n";
        }
        if (elem.flags & Element::kOnEnd) {
            os << "    virtual bool on";
            writeRuleName(os, elem.name, true);
            os << "End (const char * eptr) { return true; }\n";
        }
    }
    os << 1 + R"(
};


/****************************************************************************
*
*   )" << prefix
       << R"(Parser
*
***/

class )"
       << prefix << R"(Parser {
public:
    )" << prefix
       << "Parser (I" << prefix
       << R"(ParserNotify * notify) : m_notify(notify) {}
    ~)" << prefix
       << R"(Parser () {}

    bool parse (const char src[]);

private:
)";
    bool hasRecurseRules = false;
    for (auto && elem : rules) {
        if (elem.recurse) {
            hasRecurseRules = true;
            os << "    bool state";
            writeRuleName(os, elem.name, true);
            os << " (const char *& src);\n";
        }
    }
    if (hasRecurseRules)
        os << '\n';
    os << 1 + R"(
    I)" << prefix
       << R"(ParserNotify * m_notify{nullptr};
};
)";
}


/****************************************************************************
*
*   Internal API
*
***/

static bool s_resetRecursion = false;
static bool s_markRecursion = true;
static bool s_excludeCallbacks = false;
static bool s_buildStateTree = true;
static bool s_dedupStateTree = true;
static bool s_writeStatePositions = false;
static bool s_buildRecurseFunctions = true;

//===========================================================================
void writeParser(ostream & hfile, ostream & cppfile, const set<Element> & src) {
    const string & root = getOptionString(src, kOptionRoot);
    logMsgInfo() << "parser: " << root;

    set<Element> rules;
    if (!copyRules(rules, src, root, true))
        return;
    Element * elem = addChoiceRule(rules, kOptionRoot, 1, 1);
    addRule(elem, root, 1, 1);

    if (s_excludeCallbacks) {
        for (auto && elem : rules) {
            const_cast<Element &>(elem).flags = 0;
        }
    }

    normalize(rules);
    if (s_markRecursion)
        markRecursion(rules, *elem, s_resetRecursion);

    writeHeaderfile(hfile, rules, src);
    writeCppfileStart(cppfile, rules, src);

    unordered_set<State> states;
    buildStateTree(
        &states, rules, kOptionRoot, s_buildStateTree, s_dedupStateTree);
    writeFunction(cppfile, nullptr, rules, states, src, s_writeStatePositions);

    if (s_buildRecurseFunctions) {
        for (auto && elem : rules) {
            if (elem.recurse) {
                buildStateTree(
                    &states,
                    rules,
                    elem.name,
                    s_buildStateTree,
                    s_dedupStateTree);
                writeFunction(
                    cppfile, &elem, rules, states, src, s_writeStatePositions);
            }
        }
    }
    cppfile << endl;
}
