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
                    if (next[1].type != Element::kTerminal ||
                        next->value[0] + 1 != next[1].value[0]) {
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

    if (elem.flags || elem.function) {
        const struct {
            bool incl;
            string text;
        } tags[] = {
            { (elem.flags & Element::kOnStart) != 0, "Start" },
            { (elem.flags & Element::kOnEnd) != 0, "End" },
            { (elem.flags & Element::kOnChar) != 0, "Char" },
            { elem.function, "Function" },
            { !elem.eventName.empty(), "As=" + elem.eventName },
        };
        os << "  { ";
        bool first = true;
        for (auto && tag : tags) {
            if (tag.incl) {
                if (!first) os << ", ";
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
static void writeTerminals(ostream & os, const vector<bool> & terminals) {
    if (terminals.empty()) {
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
        if (root && (root->flags & Element::kOnEnd)) {
            writeEventCallback(os, root->name, Element::kOnEnd);
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
        if (!spt.second.empty())
            continue;
        const Element * elem = spt.first.elems.back().elem;
        if (elem == &ElementDone::s_elem)
            continue;
        assert(elem->type == Element::kRule);
        assert(elem->rule->function);
        if (call && elem->rule == call->elems.back().elem->rule &&
            spt.first.delayedEvents == call->delayedEvents) {
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
    ostream & os, const Grammar & rules, const Grammar & options) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    auto parserClass = options[kOptionApiParserClass];
    auto notifyClass = options[kOptionApiNotifyClass];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
// )" << options[kOptionApiCppFile]
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

//===========================================================================
)" << notifyClass
       << " * " << parserClass << "::notify (" << notifyClass
       << R"( * notify) {
    return notify;
}
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
    char ch;
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
static void
writeHeaderfile(ostream & os, const Grammar & rules, const Grammar & options) {
    time_t now = time(nullptr);
    tm tm;
    localtime_s(&tm, &now);
    auto parserClass = options[kOptionApiParserClass];
    auto notifyClass = options[kOptionApiNotifyClass];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
// )" << options[kOptionApiHeaderFile]
       << R"(
// Generated by pargen at )"
       << put_time(&tm, "%FT%T%z") << R"(
// clang-format off
#pragma once
)";
    if (*ns) {
        os << "\nnamespace " << ns << " {\n";
    }
    os << R"(
// forward declarations
class )"
       << notifyClass << R"(;


/****************************************************************************
*
*   )" << parserClass
       << R"(
*
***/

class )"
       << parserClass << R"( {
public:
    )" << parserClass
       << " (" << notifyClass << R"( * notify) : m_notify(notify) {}
    ~)" << parserClass
       << R"( () {}

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

    )" << notifyClass
       << R"( * notify () const { return m_notify; }

    // sets notify and returns its previous value
    )" << notifyClass
       << R"( * notify ()" << notifyClass << R"( * notify);

private:
)";
    bool hasFunctionRules = false;
    for (auto && elem : rules.rules()) {
        if (elem.function) {
            hasFunctionRules = true;
            os << "    bool state";
            writeRuleName(os, elem.name, true);
            os << " (const char *& src);\n";
        }
    }
    if (hasFunctionRules)
        os << '\n';
    os << 1 + R"(
    )" << notifyClass
       << R"( * m_notify{nullptr};
    size_t m_errpos{0};
};


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to process parse events
*
***/

class )"
       << notifyClass << R"( {
public:
    virtual ~)"
       << notifyClass << R"( () {}

    virtual bool onStart () { return true; }
    virtual bool onEnd () { return true; }
)";
    vector<pair<string, unsigned>> events;
    for (auto && elem : rules.rules()) {
        if (!elem.eventName.empty())
            continue;
        if (elem.flags &
            (Element::kOnStart | Element::kOnEnd | Element::kOnChar)) {
            ostringstream ostr;
            writeRuleName(ostr, elem.name, true);
            events.emplace_back(ostr.str(), elem.flags);
        }
    }
    if (!events.empty()) {
        os << '\n';
        sort(events.begin(), events.end());

        const struct {
            Element::Flags flag;
            const char * text;
        } sigs[] = {
            {Element::kOnStart, "Start (const char * ptr)"},
            {Element::kOnChar, "Char (char ch)"},
            {Element::kOnEnd, "End (const char * eptr)"},
        };
        for (auto && ev : events) {
            for (auto && sig : sigs) {
                if (ev.second & sig.flag) {
                    os << "    virtual bool on" << ev.first << sig.text
                       << " { return true; }\n";
                }
            }
        }
    }
    os << 1 + R"(
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
    const RunOptions & opts) {
    const string & rootname = src[kOptionRoot];
    logMsgInfo() << "parser: " << rootname;

    Grammar rules;
    if (!copyRules(rules, src, rootname, true))
        return;
    Element * root = rules.addChoiceRule(kOptionRoot, 1, 1);
    rules.addRule(root, rootname, 1, 1);

    if (!opts.includeCallbacks) {
        for (auto && elem : rules.rules()) {
            const_cast<Element &>(elem).flags = 0;
        }
    }

    normalize(rules);
    if (opts.markFunction)
        markFunction(rules, *root, opts.markFunction > 1);

    writeHeaderfile(hfile, rules, src);
    writeCppfileStart(cppfile, rules, src);

    unordered_set<State> states;
    buildStateTree(&states, *root, opts.buildStateTree, opts.dedupStateTree);
    writeFunction(cppfile, nullptr, states, src, opts.writeStatePositions);

    if (opts.writeFunctions) {
        for (auto && elem : rules.rules()) {
            if (elem.function) {
                buildStateTree(
                    &states, elem, opts.buildStateTree, opts.dedupStateTree);
                writeFunction(
                    cppfile, &elem, states, src, opts.writeStatePositions);
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
    const string & prefix) {
    streampos base = os.tellp();
    os << prefix << rule.name;
    os << " = ";
    size_t pos = os.tellp() - base;
    ostringstream raw;
    writeElement(raw, rule, false);
    string vpre = prefix + "    ";
    writeWordwrap(os, pos, raw.str(), maxWidth, vpre);
    os << '\n';
}
