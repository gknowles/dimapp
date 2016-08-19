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

//===========================================================================
ostream & operator<<(ostream & os, const Element & elem) {
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
            os << e;
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
            os << e;
        }
        os << " )";
        break;
    case Element::kTerminal:
        os << "%x" << hex << (unsigned)elem.value[0] << dec;
        break;
    }
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const set<Element> & rules) {
    for (auto && rule : rules) {
        os << "*   " << rule.name;
        if (rule.recurse)
            os << '*';
        os << " = " << rule << '\n';
    }
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StateElement & se) {
    os << *se.elem << '.' << se.rep;
    if (se.started)
        os << '+';
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StatePosition & sp) {
    os << "sp";
    if (sp.recurse)
        os << '*';
    os << ':';
    for (auto && se : sp.elems) {
        os << "\n  " << se;
    }
    if (!sp.events.empty())
        os << "\n  ---";
    for (auto && sv : sp.events) {
        os << "\n  On";
        writeRuleName(os, sv.elem->name, true);
        switch (sv.flags) {
        case Element::kOnChar: os << "Char"; break;
        case Element::kOnEnd: os << "End"; break;
        case Element::kOnStart: os << "Start"; break;
        }
    }
    os << "\n";
    return os;
}

//===========================================================================
// remove leading/trailing spaces and replace multiple spaces with one
// replace spaces, with NL followed by the prefix, if waiting until the next
//   space would go past the maxWidth
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
    if (rule.recurse)
        os << '*';
    os << " = ";
    size_t indent = os.tellp() - pos;
    ostringstream raw;
    raw << rule;
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
    os << "    switch (*ptr++) {\n";
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
static void writeParserState(
    ostream & os,
    const State & st,
    const Element * root,
    bool inclStatePositions) {
    os << "\nstate" << st.id << ": // " << st.name << "\n";
    if (inclStatePositions) {
        for (auto && sp : st.positions) {
            os << sp << '\n';
        }
    }

    if (st.name == kDoneElement) {
        os << "    return true;\n";
        return;
    }
    auto & elems = st.positions.begin()->elems;
    if (elems.size() == 1 && elems.front().elem == &ElementDone::s_elem) {
        os << "    goto state1;\n";
        return;
    }

    if (st.next[0] && root) {
        os << "    last = ptr;\n";
    }
    // for (auto && sv : st.events) {
    //    os << "    notify->On";
    //    writeRuleName(os, sv.elem->name, true);
    //    switch (sv.flags) {
    //    case Element::kOnChar: os << "Char(ptr[-1])"; break;
    //    case Element::kOnEnd: os << "End(ptr)"; break;
    //    case Element::kOnStart: os << "Start(ptr)"; break;
    //    }
    //    os << ";\n";
    //}
    // write switch case
    bool hasSwitch = writeSwitchCase(os, st);

    // write calls to recursive states
    bool hasCalls = false;
    for (auto && sp : st.positions) {
        const Element * elem = sp.elems.back().elem;
        if (elem->type != Element::kTerminal && elem != &ElementDone::s_elem) {
            hasCalls = true;
            assert(elem->type == Element::kRule);
            assert(elem->rule->recurse);
            os << "    if (state";
            writeRuleName(os, elem->rule->name, true);
            os << "(";
            if (hasSwitch)
                os << "--";
            os << "ptr)) {\n"
               << "        goto state" << st.next[256] << ";\n"
               << "    }\n";
        }
    }

    os << "    goto state0;\n";
}

//===========================================================================
static void writeCppfileStart(ostream & os, const set<Element> & rules) {
    os << 1 + R"(
// abnfsyntax.cpp - pargen
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   AbnfParser
*
*   Normalized ABNF of syntax (recursive rules are marked with asterisks):
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
    bool inclStatePositions) {
    os << 1 + R"(

//===========================================================================
bool AbnfParser::)";
    if (!root) {
        os << R"(checkSyntax (const char src[]) {
    const char * ptr = src;
    goto state2;

state0: // )"
           << kFailedElement << R"(
    return false;
)";
    } else {
        os << "state";
        writeRuleName(os, root->name, true);
        os << R"( (const char *& ptr) {
    const char * last{nullptr};
    goto state2;

state0: // )"
           << kFailedElement << R"(
    if (last) {
        ptr = last;
        return true;
    }
    return false;
)";
    }
    vector<const State *> states;
    states.resize(stateSet.size());
    for (auto && st : stateSet)
        states[st.id - 1] = &st;
    for (auto && st : states) {
        writeParserState(os, *st, root, inclStatePositions);
    }
    os << "}\n";
}

//===========================================================================
static void writeHeaderfile(ostream & os, const set<Element> & rules) {
    os << 1 + R"(
// abnfparser.h - pargen
// clang-format off
#pragma once


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to make process parsed results
*
***/

class IAbnfParserNotify {
public:
    virtual ~IAbnfParserNotify () {}

    virtual bool onAbnfStart () {}
    virtual bool onAbnfEnd () {}

)";
    for (auto && elem : rules) {
        if (elem.flags & Element::kOnStart) {
            os << "    virtual bool onAbnf";
            writeRuleName(os, elem.name, true);
            os << "Start (const char * ptr) {}\n";
        }
        if (elem.flags & Element::kOnChar) {
            os << "    virtual bool onAbnf";
            writeRuleName(os, elem.name, true);
            os << "Char (char ch) {}\n";
        }
        if (elem.flags & Element::kOnEnd) {
            os << "    virtual bool onAbnf";
            writeRuleName(os, elem.name, true);
            os << "End (const char * eptr) {}\n";
        }
    }
    os << 1 + R"(
};


/****************************************************************************
*
*   AbnfParser
*
***/

class AbnfParser {
public:
    AbnfParser (IAbnfParserNotify * notify) : m_notify(notify) {}
    ~AbnfParser () {}

    bool checkSyntax (const char src[]);

private:
)";
    for (auto && elem : rules) {
        if (elem.recurse) {
            os << "    bool state";
            writeRuleName(os, elem.name, true);
            os << " (const char *& src);\n";
        }
    }
    os << 1 + R"(

    IAbnfParserNotify * m_notify{nullptr};
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
static bool s_buildStateTree = true;
static bool s_writeStatePositions = true;

//===========================================================================
void writeParser(
    ostream & hfile,
    ostream & cppfile,
    const set<Element> & src,
    const string & root) {
    logMsgInfo() << "parser: " << root;

    set<Element> rules;
    copyRules(rules, src, root, true);
    Element * elem = addChoiceRule(rules, kRootElement, 1, 1);
    addRule(elem, root, 1, 1);

    normalize(rules);
    if (s_markRecursion)
        markRecursion(rules, *elem, s_resetRecursion);

    writeHeaderfile(hfile, rules);

    writeCppfileStart(cppfile, rules);

    unordered_set<State> states;
    buildStateTree(&states, rules, kRootElement, s_buildStateTree);
    writeFunction(cppfile, nullptr, rules, states, s_writeStatePositions);

    for (auto && elem : rules) {
        if (elem.recurse) {
            buildStateTree(&states, rules, elem.name, s_buildStateTree);
            writeFunction(cppfile, &elem, rules, states, s_writeStatePositions);
        }
    }
    cppfile << endl;
}
