// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
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
ostream & operator<<(ostream & os, const Grammar & rules) {
    for (auto && rule : rules.rules()) {
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
    if (se.recurse)
        os << " (RECURSE)";
    return os;
}

//===========================================================================
ostream & operator<<(ostream & os, const StateEvent & sv) {
    os << "on";
    writeRuleName(os, sv.elem->name, true);
    switch (sv.flags) {
    case Element::fOnChar: os << "Char"; break;
    case Element::fOnEnd: os << "End"; break;
    case Element::fOnStart: os << "Start"; break;
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
    switch (elem.type) {
    case Element::kChoice: {
        if (elem.elements.size() == 1) {
            writeElement(os, elem.elements.front(), inclPos);
            break;
        }
        os << "( ";
        bool first = true;
        auto cur = elem.elements.begin();
        auto last = elem.elements.end();
        for (; cur != last; ++cur) {
            if (first) {
                first = false;
            } else {
                os << " / ";
            }
            writeElement(os, *cur, inclPos);
            if (cur->type == Element::kTerminal) {
                auto next = cur;
                for (; next + 1 != last; ++next) {
                    if (next[1].type != Element::kTerminal
                        || next->value[0] + 1 != next[1].value[0]
                    ) {
                        break;
                    }
                }
                if (cur != next) {
                    cur = next;
                    os << '-' << hex << (unsigned)(unsigned char)cur->value[0]
                       << dec;
                }
            }
        }
        os << " )";
    } break;
    case Element::kRule: os << elem.value; break;
    case Element::kSequence: {
        os << "( ";
        auto cur = elem.elements.begin();
        auto last = elem.elements.end();
        writeElement(os, *cur++, inclPos);
        for (; cur != last; ++cur) {
            os << " ";
            writeElement(os, *cur, inclPos);
        }
        os << " )";
    } break;
    case Element::kTerminal:
        os << "%x" << hex << (unsigned)(unsigned char)elem.value[0] << dec;
        break;
    }

    if (elem.flags) {
        const struct {
            bool incl;
            string text;
        } tags[] = {
            {(elem.flags & Element::fOnStart) != 0, "Start"},
            {(elem.flags & Element::fOnEnd) != 0, "End"},
            {(elem.flags & Element::fOnChar) != 0, "Char"},
            {(elem.flags & Element::fFunction) != 0, "Function"},
            {!elem.eventName.empty(), "As " + elem.eventName},
        };
        os << "  { ";
        bool first = true;
        for (auto && tag : tags) {
            if (tag.incl) {
                if (!first)
                    os << ", ";
                first = false;
                os << tag.text;
            }
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
    size_t & pos,
    const string & str,
    size_t maxWidth,
    const string & prefix
) {
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
        if (pos + len >= maxWidth) {
            os << '\n' << prefix;
            pos = size(prefix);
        }
        if (*ptr) {
            os.write(base, len + 1);
            base = ptr + 1;
        } else {
            os.write(base, len);
            os << ' ';
            base = ptr;
        }
        pos += len + 1;
    }
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
    map<unsigned, unsigned char> stateKeys;
    for (unsigned i = 0; i < 256; ++i) {
        if (unsigned next = st.next[i]) {
            auto ch = (unsigned char)i;
            auto ii = stateKeys.equal_range(next);
            if (ii.first == ii.second)
                stateKeys.insert(ii.first, make_pair(next, ch));
            cases.push_back({ch, next});
        }
    }
    if (cases.empty())
        return false;

    sort(
        cases.begin(),
        cases.end(),
        [&stateKeys](const NextState & e1, const NextState & e2) {
            return 256 * stateKeys[e1.state] + e1.ch
                < 256 * stateKeys[e2.state] + e2.ch;
        }
    );
    const unsigned kCaseColumns = 6;
    os << "    ch = *ptr++;\n";
    os << "    switch (ch) {\n";
    unsigned prev = cases.front().state;
    unsigned pos = 0;
    for (auto && ns : cases) {
        if (ns.state != prev) {
            if (pos % kCaseColumns)
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
    if (pos % kCaseColumns)
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
    const string & prefix = "    "
) {
    os << prefix << "if (!on";
    writeRuleName(os, name, true);
    switch (type) {
    case Element::fOnChar: os << "Char(" << (args ? args : "ch"); break;
    case Element::fOnEnd: os << "End(" << (args ? args : "ptr"); break;
    case Element::fOnStart: os << "Start(" << (args ? args : "ptr - 1"); break;
    }
    os << "))\n";
    os << prefix << "    goto state0;\n";
}

//===========================================================================
static void writeStateName(
    ostream & os,
    const string & name,
    size_t maxWidth,
    const string & prefix
) {
    size_t space = maxWidth - size(prefix);
    assert(space > 4);
    size_t count = name.size();
    size_t pos = 0;
    size_t num = min({space, count});
    os << prefix;
    for (;;) {
        // don't end a line with ' ', use ^x20 or adjust fit
        if (name[pos + num - 1] == ' ') {
            for (;;) {
                if (num + 3 < space) {
                    os.write(name.data() + pos, num - 1);
                    os << "^x20";
                    goto NEXT_LINE;
                }
                num -= 1;
                if (name[pos + num - 1] != ' ')
                    break;
            }
        }
        os.write(name.data() + pos, num);
        if (name[pos + num - 1] == '\\') {
            // extra space because a trailing backslash would cause the comment
            // to be extended to the next line by the c++ compiler
            os << ' ';
        }

    NEXT_LINE:
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
static void writeTerminals(ostream & os, const bitset<256> & terminals) {
    if (terminals.none()) {
        os << "< >\n";
        return;
    }
    assert(terminals.size() == 256);

    os << "< ";
    unsigned ch = 0;
    for (; ch < 256; ++ch) {
        if (terminals[ch])
            break;
    }
    assert(ch < 256);

    os << "%x" << hex << ch << dec;
    for (;;) {
        unsigned next = ch + 1;
        for (; next < 256; ++next) {
            if (!terminals[next])
                break;
        }
        if (next != ch + 1)
            os << '-' << hex << next - 1 << dec;
        ch = next;
        for (;; ++ch) {
            if (ch == 256) {
                os << " >\n";
                return;
            }
            if (terminals[ch])
                break;
        }
        os << " / %x" << hex << ch << dec;
    }
}

//===========================================================================
static void writeParserState(
    ostream & os,
    const State & st,
    const Element * root,
    bool inclStatePositions
) {
    os << "\nstate" << st.id << ":\n";
    vector<string> aliases = st.aliases;
    if (st.name.empty()) {
        aliases.push_back(to_string(st.id) + ":");
    } else {
        aliases.push_back(to_string(st.id) + ": " + st.name);
    }
    sort(aliases.begin(), aliases.end(), [](auto && a, auto && b) {
        return strtoul(a.c_str(), nullptr, 10)
            < strtoul(b.c_str(), nullptr, 10);
    });
    for (auto && name : aliases)
        writeStateName(os, name, 79, "    // ");
    if (inclStatePositions) {
        os << "    // positions: " << st.positions.size() << "\n\n";
        for (auto && spt : st.positions) {
            os << spt.first << "    //  ";
            writeTerminals(os, spt.second);
            os << '\n';
        }
    }

    // os << "    std::cout << " << st.id << " << ' ' << *ptr << std::endl;\n";

    // execute notifications
    for (auto && sv : st.positions.begin()->first.events) {
        writeEventCallback(os, sv.elem->name, Element::Flags(sv.flags));
    }

    if (st.name == kDoneStateName) {
        if (root && (root->flags & Element::fOnEnd)) {
            writeEventCallback(os, root->name, Element::fOnEnd);
        }
        os << "    return true;\n";
        return;
    }
    auto & elems = st.positions.begin()->first.elems;
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
    const StatePosition * call = nullptr;
    for (auto && spt : st.positions) {
        if (spt.second.any())
            continue;
        const Element * elem = spt.first.elems.back().elem;
        if (elem == &ElementDone::s_elem)
            continue;
        assert(elem->type == Element::kRule);
        assert(elem->rule->flags & Element::fFunction);
        if (call && elem->rule == call->elems.back().elem->rule
            && spt.first.delayedEvents == call->delayedEvents) {
            continue;
        }
        call = &spt.first;
        for (auto && sv : spt.first.delayedEvents) {
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

    os << "    goto state0;\n";
}

//===========================================================================
static void writeCppfileStart(
    ostream & os,
    const Grammar & rules,
    const Grammar & options
) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    auto parserClass = options[kOptionApiParserClass];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
// )" << options[kOptionApiParserCpp]
       << R"(
// Generated by pargen at )"
       << put_time(&tm, "%FT%T%z") << R"(
// clang-format off
#include "pch.h"
#pragma hdrstop
)";
    if (*ns) {
        os << "\nusing namespace " << ns << ";\n";
    }
    os << R"(

/****************************************************************************
*
*   )" << parserClass
       << R"(
*
*   Normalized ABNF of syntax:
)";

    for (auto && elem : rules.rules()) {
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
    const unordered_set<State> & stateSet,
    const Grammar & options,
    bool inclStatePositions) {
    auto parserClass = options[kOptionApiParserClass];
    os << 1 + R"(

//===========================================================================
// Parser function covering:
//  - )"
       << stateSet.size() + 1 << R"( states
bool )" << parserClass
       << "::";
    if (!root) {
        os << R"(parse (const char src[]) {
    const char * ptr = src;
    unsigned char ch;
    goto state2;

state0: 
    // )" << kFailedStateName
           << R"(
    m_errpos = ptr - src - 1;
    return false;
)";
    } else {
        os << "state";
        writeRuleName(os, root->name, true);
        os << R"( (const char *& ptr) {
    const char * last{nullptr};
    unsigned char ch;
)";
        if (root->flags & Element::fOnStart) {
            writeEventCallback(os, root->name, Element::fOnStart, "ptr");
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
    ostream & os, 
    const Grammar & rules, 
    const Grammar & options
) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    auto parserClass = options[kOptionApiParserClass];
    auto baseClass = options[kOptionApiBaseClass];
    auto baseHeader = options[kOptionApiBaseHeader];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
// )" << options[kOptionApiParserHeader]
       << R"(
// Generated by pargen at )"
       << put_time(&tm, "%FT%T%z") << R"(
// clang-format off
#pragma once

#include ")" << baseHeader << "\"\n";
    if (*ns) {
        os << "\nnamespace " << ns << " {\n";
    }
    os << R"(

/****************************************************************************
*
*   )" << parserClass
       << R"(
*
***/

class )"
       << parserClass << " : public " << baseClass << R"( {
public:
    using )" << baseClass << "::" << baseClass << R"(;

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

private:
)";
    bool hasFunctionRules = false;
    for (auto && elem : rules.rules()) {
        if (elem.flags & Element::fFunction) {
            hasFunctionRules = true;
            os << "    bool state";
            writeRuleName(os, elem.name, true);
            os << " (const char *& src);\n";
        }
    }
    if (hasFunctionRules)
        os << '\n';
    os << 1 + R"(
    // Events
    bool onStart ();
    bool onEnd ();
)";

    map<string, unsigned> events;
    ostringstream ostr;
    for (auto && elem : rules.rules()) {
        if (elem.flags
            & (Element::fOnStart | Element::fOnEnd | Element::fOnChar)
        ) {
            ostr.clear();
            ostr.str("");
            if (elem.eventName.empty()) {
                writeRuleName(ostr, elem.name, true);
            } else {
                writeRuleName(ostr, elem.eventName, true);
            }
            events[ostr.str()] |= elem.flags;
        }
    }
    if (!events.empty()) {
        os << '\n';
        const struct {
            Element::Flags flag;
            const char * text;
        } sigs[] = {
            {Element::fOnStart, "Start (const char * ptr)"},
            {Element::fOnChar, "Char (char ch)"},
            {Element::fOnEnd, "End (const char * eptr)"},
        };
        for (auto && ev : events) {
            for (auto && sig : sigs) {
                if (ev.second & sig.flag) {
                    os << "    bool on" << ev.first << sig.text << ";\n";
                }
            }
        }
    }

    os << R"(
    // Data members
    size_t m_errpos{0};
};
)";
    if (*ns) {
        os << "\n} // namespace\n";
    }
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void writeParser(
    ostream & hfile,
    ostream & cppfile,
    const Grammar & src,
    const RunOptions & opts
) {
    const string & rootname = src[kOptionRoot];
    logMsgDebug() << "parser: " << rootname;

    Grammar rules;
    if (!copyRules(rules, src, rootname, true))
        return;
    Element * root = rules.addChoiceRule(kOptionRoot, 1, 1);
    rules.addRule(root, rootname, 1, 1);

    if (!opts.includeCallbacks) {
        for (auto && elem : rules.rules()) {
            const_cast<Element &>(elem).flags &= ~Element::fCallbackFlags;
        }
    }

    normalize(rules);
    functionTags(rules, *root, opts.resetFunctions, opts.markFunctions);

    writeHeaderfile(hfile, rules, src);
    writeCppfileStart(cppfile, rules, src);

    unordered_set<State> states;
    buildStateTree(
        &states,
        *root,
        opts.buildStateTree,
        opts.dedupStateTree,
        opts.stateTreeDepthLimit
    );
    writeFunction(cppfile, nullptr, states, src, opts.writeStatePositions);

    if (opts.writeFunctions) {
        for (auto && elem : rules.rules()) {
            if (elem.flags & Element::fFunction) {
                buildStateTree(
                    &states,
                    elem,
                    opts.buildStateTree,
                    opts.dedupStateTree,
                    opts.stateTreeDepthLimit
                );
                writeFunction(
                    cppfile, 
                    &elem, 
                    states, 
                    src, 
                    opts.writeStatePositions
                );
            }
        }
    }
    cppfile << endl;
}

//===========================================================================
void writeRule(
    ostream & os,
    const Element & rule,
    size_t maxWidth,
    string_view prefix
) {
    streampos base = os.tellp();
    os << prefix << rule.name;
    os << " = ";
    size_t pos = os.tellp() - base;
    ostringstream raw;
    writeElement(raw, rule, false);
    auto vpre = string(prefix) + "    ";
    writeWordwrap(os, pos, raw.str(), maxWidth, vpre);
    os << '\n';
}
