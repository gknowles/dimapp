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

//===========================================================================
ostream &operator<<(ostream &os, const Element &elem) {
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
        for (auto &&e : elem.elements) {
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
        for (auto &&e : elem.elements) {
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
ostream &operator<<(ostream &os, const set<Element> &rules) {
    for (auto &&rule : rules) {
        os << "*   " << rule.name;
        if (rule.recurse)
            os << '*';
        os << " = " << rule << '\n';
    }
    return os;
}

//===========================================================================
ostream &operator<<(ostream &os, const StateElement &se) {
    os << *se.elem << '.' << se.rep;
    return os;
}

//===========================================================================
ostream &operator<<(ostream &os, const StatePosition &sp) {
    os << "sp";
    if (sp.recurse)
        os << '*';
    os << ':';
    for (auto &&se : sp.elems) {
        os << "\n  " << se;
    }
    os << "\n";
    return os;
}

//===========================================================================
// remove leading/trailing spaces and replace multiple spaces with one
// replace spaces, with NL followed by the prefix, if waiting until the next
//   space would go past the maxWidth
static void writeWordwrap(
    ostream &os,
    size_t &indent,
    const string &str,
    size_t maxWidth,
    const string &prefix) {
    const char *base = str.c_str();
    for (;;) {
        while (*base == ' ')
            base += 1;
        if (!*base)
            return;
        const char *ptr = strchr(base, ' ');
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
    ostream &os, const Element &rule, size_t maxWidth, const string &prefix) {
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
static void writeRuleName(ostream &os, const string &name, bool capitalize) {
    for (auto &&ch : name) {
        if (ch == '-') {
            capitalize = true;
        } else {
            os << (capitalize ? (char)toupper(ch) : ch);
            capitalize = false;
        }
    }
}

//===========================================================================
static bool writeSwitchCase(ostream &os, const State &st) {
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
        [](const NextState &e1, const NextState &e2) {
            return 256 * e1.state + e1.ch < 256 * e2.state + e2.ch;
        });
    const unsigned kCaseColumns = 6;
    os << "    switch (*ptr++) {\n";
    unsigned prev = cases.front().state;
    unsigned pos = 0;
    for (auto &&ns : cases) {
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
static void
writeParserState(ostream &os, const State &st, bool inclStatePositions) {
    os << "\nstate" << st.id << ": // " << st.name << "\n";
    if (inclStatePositions) {
        for (auto &&sp : st.positions) {
            os << sp << '\n';
        }
    }

    if (st.name == kDoneStateName) {
        os << "    return true;\n";
        return;
    }

    // write switch case
    bool hasSwitch = writeSwitchCase(os, st);

    // write calls to recursive states
    bool hasCalls = false;
    for (auto &&sp : st.positions) {
        const Element *elem = sp.elems.back().elem;
        if (elem->type != Element::kTerminal && elem != &ElementDone::s_elem) {
            hasCalls = true;
            assert(elem->type == Element::kRule && elem->rule->recurse);
            os << "    if (state";
            writeRuleName(os, elem->rule->name, true);
            os << "(";
            if (hasSwitch)
                os << "--";
            os << "ptr)) {\n"
               << "        goto state1;\n"
               << "    }\n";
        }
    }

    os << "    goto state0;\n";
}

//===========================================================================
static void writeCppfileStart(ostream &os, const set<Element> &rules) {
    os << 1 + R"(
// abnfsyntax.cpp - pargen
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   AbnfParser
*
*   Normalized ABNF of syntax being checked (recursive rules are marked 
*       with asterisks):
)";

    for (auto &&elem : rules) {
        writeRule(os, elem, 79, "*   ");
    }

    os << 1 + R"(
*
***/
)";
}

//===========================================================================
static void writeFunction(
    ostream &os,
    const Element *root,
    const set<Element> &rules,
    const unordered_set<State> &stateSet,
    bool inclStatePositions) {
    os << 1 + R"(

//===========================================================================
)";
    if (!root) {
        os << 1 + R"(
bool AbnfParser::checkSyntax (const char src[]) {
    const char * ptr = src;
)";
    } else {
        os << "bool AbnfParser::state";
        writeRuleName(os, root->name, true);
        os << "(const char *& ptr) {\n";
    }
    os << 1 + R"(
    goto state2;

state0: // )"
       << kFailedStateName << R"(
    return false;
)";
    vector<const State *> states;
    states.resize(stateSet.size());
    for (auto &&st : stateSet)
        states[st.id - 1] = &st;
    for (auto &&st : states) {
        writeParserState(os, *st, inclStatePositions);
    }
    os << "}\n";
}

//===========================================================================
static void writeHeaderfile(ostream &os, const set<Element> &rules) {
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

    virtual bool startDoc () {}
    virtual bool endDoc () {}
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
    for (auto &&elem : rules) {
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

static bool s_markRecursion = false;
static bool s_buildStateTree = true;
static bool s_writeStatePositions = true;

//===========================================================================
void writeParser(
    ostream &hfile,
    ostream &cppfile,
    const set<Element> &src,
    const string &root) {
    logMsgInfo() << "parser: " << root;

    set<Element> rules;
    copyRules(rules, src, root, true);
    Element *elem = addChoiceRule(rules, kRootElement, 1, 1);
    addRule(elem, root, 1, 1);
    normalize(rules);
    if (s_markRecursion)
        findRecursion(rules, *elem);

    writeHeaderfile(hfile, rules);

    writeCppfileStart(cppfile, rules);

    unordered_set<State> states;
    buildStateTree(&states, rules, kRootElement, s_buildStateTree);
    writeFunction(cppfile, nullptr, rules, states, s_writeStatePositions);

    for (auto &&elem : rules) {
        if (elem.recurse) {
            buildStateTree(&states, rules, elem.name, s_buildStateTree);
            writeFunction(cppfile, &elem, rules, states, s_writeStatePositions);
        }
    }
    cppfile << endl;
}
