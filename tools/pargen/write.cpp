// Copyright Glen Knowles 2016 - 2024.
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
    switch (sv.flags.value()) {
    case Element::fOnChar: os << "Char"; break;
    case Element::fOnCharW: os << "CharW"; break;
    case Element::fOnEnd: os << "End"; break;
    case Element::fOnEndW: os << "EndW"; break;
    case Element::fOnStart: os << "Start"; break;
    case Element::fOnStartW: os << "StartW"; break;
    default:
        assert(0);
    }
    if (sv.distance > 0)
        os << "(-" << sv.distance << ")";
    return os;
}

//===========================================================================
static void writeSp(ostream & os, const StatePosition & sp, int pos) {
    os << "    // ===== #" << pos;
    if (sp.recurseSe) {
        os << " (RECURSE "
            << sp.recurseSe + 1 << ". "
            << "#" << sp.recursePos + 1 << ")";
    }
    for (auto && sv : sp.events)
        os << "\n    // " << sv;
    if (!sp.delayedEvents.empty()) {
        os << "\n    // --- delayed";
        for (auto && sv : sp.delayedEvents)
            os << "\n    // " << sv;
    }
    if (!sp.events.empty() || !sp.delayedEvents.empty())
        os << "\n    // ---";
    int sepos = 0;
    for (auto && se : sp.elems) {
        os << "\n    // " << ++sepos << ". " << se;
    }
    os << "\n";
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
    case Element::kRule:
        os << elem.value;
        if (elem.value == kDoneRuleName) {
            if (&elem == &ElementDone::s_consume) {
                os << "(consume)";
            } else {
                assert(&elem == &ElementDone::s_abort);
                os << "(abort)";
            }
        }
        break;
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

    if (elem.flags.any(Element::fEvents | Element::fFunction)) {
        struct {
            bool incl;
            string text;
        } const tags[] = {
            {elem.flags.any(Element::fOnStart), "Start"},
            {elem.flags.any(Element::fOnStartW), "Start+"},
            {elem.flags.any(Element::fOnEnd), "End"},
            {elem.flags.any(Element::fOnEndW), "End+"},
            {elem.flags.any(Element::fOnChar), "Char"},
            {elem.flags.any(Element::fOnCharW), "Char+"},
            {elem.flags.any(Element::fFunction), "Function"},
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
    bool startOfLine = true;
    for (;;) {
        while (*base == ' ')
            base += 1;
        if (!*base)
            return;
        const char * ptr = strchr(base, ' ');
        if (!ptr)
            ptr = str.c_str() + str.size();
        size_t len = ptr - base;
        if (pos + len + !startOfLine > maxWidth) {
            os << '\n' << prefix;
            pos = size(prefix);
            startOfLine = true;
        }
        if (!startOfLine)
            os << ' ';
        os.write(base, len);
        base = ptr + (bool) *ptr;
        pos += len + 1;
        startOfLine = false;
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
static bool writeSwitchCase(
    ostream & os,
    const State & st,
    bool setLast,
    bool hasEvents
) {
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
            if (setLast && i == 0 && st.next[0] == 1)
                next = 0;
            auto ii = stateKeys.equal_range(next);
            if (ii.first == ii.second)
                stateKeys.insert(ii.first, make_pair(next, ch));
            cases.push_back({ch, next});
        }
    }
    if (cases.empty())
        return false;

    ranges::sort(
        cases,
        [&stateKeys](const NextState & e1, const NextState & e2) {
            return 256 * stateKeys[e1.state] + e1.ch
                < 256 * stateKeys[e2.state] + e2.ch;
        }
    );
    if (setLast) {
        if (st.next[0]) {
            os << "    last = ptr;\n";
        } else if (hasEvents) {
            os << "    last = nullptr;\n";
        }
    }
    os << "    ch = *ptr++;\n";

    const unsigned kCaseColumns = 6;
    os << "    switch (ch) {\n";
    unsigned prev = cases.front().state;
    unsigned pos = 0;
    for (unsigned i = 0; i < cases.size(); ++i) {
        auto && ns = cases[i];
        if (ns.state != prev) {
            if (pos % kCaseColumns)
                os << '\n';
            os << "        goto STATE_" << prev << ";\n";
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
        } else if (i + 1 < cases.size() && cases[i + 1].state == ns.state) {
            os << ' ';
        }
    }
    if (pos % kCaseColumns)
        os << '\n';

    os << "        goto STATE_" << cases.back().state << ";\n"
       << "    }\n";
    return true;
}

//===========================================================================
static void writeEventCallback(
    ostream & os,
    const string & name,
    EnumFlags<Element::Flags> type,
    const char * args = nullptr,
    const string & prefix = "    "
) {
    os << prefix << "if (!on";
    writeRuleName(os, name, true);
    type &= Element::fEvents;
    switch (type.value()) {
    case Element::fOnChar: os << "Char("; break;
    case Element::fOnCharW: os << "Char(" << (args ? args : "ch"); break;
    case Element::fOnEnd: os << "End("; break;
    case Element::fOnEndW: os << "End(" << (args ? args : "ptr"); break;
    case Element::fOnStart: os << "Start("; break;
    case Element::fOnStartW:
        os << "Start(" << (args ? args : "ptr - 1");
        break;
    default:
        assert(0);
    }
    os << "))\n";
    os << prefix << "    goto STATE_0;\n";
}

//===========================================================================
static void writeEventCallback(ostream & os, const StateEvent & sv) {
    ostringstream oargs;
    if (sv.distance) {
        oargs << "ptr";
        switch (sv.flags.value()) {
        case Element::fOnChar: break;
        case Element::fOnCharW:
            oargs << "[-"s << sv.distance + 1 << ']';
            break;
        case Element::fOnEnd: break;
        case Element::fOnEndW: oargs << " - " << sv.distance; break;
        case Element::fOnStart: break;
        case Element::fOnStartW: oargs << " - " << sv.distance + 1; break;
        default:
            assert(0);
        }
    }
    auto args = oargs.str();
    writeEventCallback(
        os,
        sv.elem->name,
        sv.flags,
        args.empty() ? nullptr : args.c_str()
    );
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
        // Don't end a line with ' ' or '\', use ^x20, ^x5c, or adjust fit
        //  - spaces at the end of a line can't be seen
        //  - trailing backslash causes the c++ compiler to extend the
        //    comment to the next line
        char ch = name[pos + num - 1];
        while (ch == ' ' || ch == '\\') {
            if (num + 3 < space) {
                os.write(name.data() + pos, num - 1);
                os << (ch == ' ' ? "^x20" : "^x5c");
                goto NEXT_LINE;
            }
            num -= 1;
            ch = name[pos + num - 1];
        }
        os.write(name.data() + pos, num);

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
    os << "\nSTATE_" << st.id << ":\n";
    vector<string> aliases = st.aliases;
    if (st.name.empty()) {
        aliases.push_back(toString(st.id) + ":");
    } else {
        aliases.push_back(toString(st.id) + ": " + st.name);
    }
    ranges::sort(aliases, [](auto && a, auto && b) {
        return strtoul(a.c_str(), nullptr, 10)
            < strtoul(b.c_str(), nullptr, 10);
    });
    for (auto && name : aliases)
        writeStateName(os, name, 79, "    // ");
    if (inclStatePositions) {
        os << "    // positions: " << st.positions.size() << "\n\n";
        int pos = 0;
        for (auto && spt : st.positions) {
            writeSp(os, spt.first, ++pos);
            os << "    // ";
            writeTerminals(os, spt.second);
            os << '\n';
        }
    }

    // execute events
    auto & events = st.positions.begin()->first.events;
    for (auto && sv : events) {
        writeEventCallback(os, sv);
    }

    if (st.name == kDoneStateName) {
        if (root) {
            auto flags = root->flags & Element::fEndEvents;
            if (flags.any())
                writeEventCallback(os, root->name, flags.value());
        }
        os << "    return true;\n";
        return;
    }
    auto & elems = st.positions.begin()->first.elems;
    if (elems.size() == 1 && elems.front().elem->value == kDoneRuleName) {
        if (elems.front().elem == &ElementDone::s_abort) {
            os << "    ptr -= 1;\n";
        }
        os << "    goto STATE_1;\n";
        return;
    }

    // write switch case
    bool hasSwitch = writeSwitchCase(os, st, root, !events.empty());

    // write calls to independent sub-state parsers
    const StatePosition * call = nullptr;
    for (auto && spt : st.positions) {
        if (spt.second.any())
            continue;
        const Element * elem = spt.first.elems.back().elem;
        if (elem->value == kDoneRuleName)
            continue;
        assert(elem->type == Element::kRule);
        assert(elem->rule->flags.any(Element::fFunction));
        if (call
            && elem->rule == call->elems.back().elem->rule
            && spt.first.delayedEvents == call->delayedEvents
        ) {
            continue;
        }
        call = &spt.first;
        for (auto && sv : spt.first.delayedEvents) {
            writeEventCallback(os, sv);
        }
        os << "    if (state";
        writeRuleName(os, elem->rule->name, true);
        os << "(";
        if (hasSwitch)
            os << "--";
        unsigned id = st.next.empty() ? 0 : st.next[256];
        os << "ptr))\n"
           << "        goto STATE_" << id << ";\n";
    }

    unsigned id = root && !st.next.empty() ? st.next[0] : 0;
    if (id == 1) {
        os << "    ptr -= 1;\n";
    }
    os << "    goto STATE_" << id << ";\n";
}

//===========================================================================
static void writeGeneratedBy(ostream & os, const string & fname) {
    os << 1 + R"(
// )"   << fname
        << R"(
// Generated by pargen v)"
        << toString(appVersion()) << R"(, DO NOT EDIT.
// clang-format off
)";
}

//===========================================================================
static void writeCppfileStart(
    ostream & os,
    const Grammar & rules,
    const Grammar & options
) {
    writeGeneratedBy(os, options[kOptionApiOutputCpp]);

    auto parserClass = options[kOptionApiOutputClass];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
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
static void writeMainFuncIntro(ostream & os) {
    os << R"(parse (const char src[]) {
    const char * ptr = src;
    unsigned char ch;
    goto STATE_2;

STATE_0:
    // )" << kFailedStateName
           << R"(
    m_errpos = ptr - src - 1;
    return false;
)";
}

//===========================================================================
static void writeStateFuncIntro(ostream & os, const Element * root) {
    os << "state";
    writeRuleName(os, root->name, true);
    os << R"( (const char *& ptr) {
    const char * last = nullptr;
    unsigned char ch;
)";
    if (root->flags.any(Element::fOnStart))
        writeEventCallback(os, root->name, Element::fOnStart);
    if (root->flags.any(Element::fOnStartW))
        writeEventCallback(os, root->name, Element::fOnStartW, "ptr");
    os << R"(    goto STATE_2;

STATE_0:
    // )" << kFailedStateName
           << R"(
    if (last) {
        ptr = last;
        goto STATE_1;
    }
    return false;
)";
}

//===========================================================================
static void writeFunction(
    ostream & os,
    const Element * root,
    const unordered_set<State> & stateSet,
    const Grammar & options,
    bool inclStatePositions
) {
    auto parserClass = options[kOptionApiOutputClass];
    os << 1 + R"(

//===========================================================================
// Parser function covering:
//  - )"
       << stateSet.size() + 1 << R"( states
[[gsl::suppress(bounds)]]
bool )" << parserClass
       << "::";
    if (!root) {
        writeMainFuncIntro(os);
    } else {
        writeStateFuncIntro(os, root);
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
    writeGeneratedBy(os, options[kOptionApiOutputHeader]);

    auto parserClass = options[kOptionApiOutputClass];
    auto baseClass = options[kOptionApiBaseClass];
    auto baseHeader = options[kOptionApiBaseHeader];
    auto ns = options[kOptionApiNamespace];
    os << 1 + R"(
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
        if (elem.flags.any(Element::fFunction)) {
            hasFunctionRules = true;
            os << "    bool state";
            writeRuleName(os, elem.name, true);
            os << " (const char *& src);\n";
        }
    }
    if (hasFunctionRules)
        os << '\n';

    map<string, EnumFlags<Element::Flags>> events;
    ostringstream ostr;
    for (auto && elem : rules.rules()) {
        if (elem.flags.any(Element::fEvents)) {
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
        os << "    // Events\n";
        struct {
            Element::Flags flag;
            const char * text;
        } const sigs[] = {
            {Element::fOnStart, "Start ()"},
            {Element::fOnStartW, "Start (const char * ptr)"},
            {Element::fOnChar, "Char ()"},
            {Element::fOnCharW, "Char (char ch)"},
            {Element::fOnEnd, "End ()"},
            {Element::fOnEndW, "End (const char * eptr)"},
        };
        for (auto && ev : events) {
            for (auto && sig : sigs) {
                if (ev.second.any(sig.flag)) {
                    os << "    bool on" << ev.first << sig.text << ";\n";
                }
            }
        }
        os << '\n';
    }

    os << 1 + R"(
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
            const_cast<Element &>(elem).flags.reset(Element::fEvents);
        }
    }

    normalize(rules);
    if (opts.mergeRules)
        merge(rules);
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
            if (elem.flags.any(Element::fFunction)) {
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
    auto pos = (size_t) (os.tellp() - base);
    ostringstream raw;
    writeElement(raw, rule, false);
    auto vpre = string(prefix) + "    ";
    writeWordwrap(os, pos, raw.str(), maxWidth, vpre);
    os << '\n';
}
