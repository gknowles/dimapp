// gen.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

static void hashCombine(size_t &seed, size_t v);

const char kRootElement[] = "<ROOT>";
const char kDoneStateName[] = "<DONE>";
const char kFailedStateName[] = "<FAILED>";

namespace {

struct ElementDone : Element {
    ElementDone();
};

struct StateElement {
    const Element *elem;
    unsigned rep;

    int compare(const StateElement &right) const;
    bool operator<(const StateElement &right) const;
    bool operator==(const StateElement &right) const;
};

struct StatePosition {
    vector<StateElement> elems;
    bool recurse{false};

    bool operator<(const StatePosition &right) const;
    bool operator==(const StatePosition &right) const;
};
} // namespace

namespace std {
template <> struct hash<StateElement> {
    size_t operator()(const StateElement &val) const {
        size_t out = val.elem->id;
        hashCombine(out, val.rep);
        return out;
    }
};

template <> struct hash<StatePosition> {
    size_t operator()(const StatePosition &val) const {
        size_t out = 0;
        for (auto &&se : val.elems) {
            hashCombine(out, hash<StateElement>{}(se));
        }
        return out;
    }
};
}

namespace {
struct State {
    unsigned id;
    string name;
    set<StatePosition> positions;
    vector<unsigned> next;

    void clear() {
        id = 0;
        positions.clear();
        next.clear();
    }

    bool operator==(const State &right) const {
        return equal(
            positions.begin(),
            positions.end(),
            right.positions.begin(),
            right.positions.end());
    }
};

} // namespace

namespace std {
template <> struct hash<State> {
    size_t operator()(const State &val) const {
        size_t out = 0;
        for (auto &&sp : val.positions) {
            hashCombine(out, hash<StatePosition>{}(sp));
        }
        return out;
    }
};
} // namespace std


/****************************************************************************
*
*   Variables
*
***/

static unsigned s_nextStateId = 1;
static unsigned s_transitions;
static ElementDone s_done;


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static void copyRules(
    set<Element> &rules,
    const set<Element> &src,
    const string &root,
    bool failIfExists);
static void normalizeSequence(Element &rule);

static void addPositions(
    State *st, StatePosition *sp, bool init, const Element &elem, unsigned rep);

//===========================================================================
static void hashCombine(size_t &seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

//===========================================================================
static void copyRequiredDeps(
    set<Element> &rules, const set<Element> &src, const Element &root) {
    switch (root.type) {
    case Element::kSequence:
    case Element::kChoice:
        for (auto &&elem : root.elements)
            copyRequiredDeps(rules, src, elem);
        break;
    case Element::kRule: copyRules(rules, src, root.value, false); break;
    }
}

//===========================================================================
static void copyRules(
    set<Element> &rules,
    const set<Element> &src,
    const string &root,
    bool failIfExists) {
    Element key;
    key.name = root;
    auto it = src.find(key);
    if (it == src.end()) {
        logMsgError() << "Rule not found, " << root;
        return;
    }
    auto ib = rules.insert(*it);
    if (!ib.second) {
        if (failIfExists) {
            logMsgError() << "Rule already exists, " << root;
        }
        return;
    }
    copyRequiredDeps(rules, src, *ib.first);
}

//===========================================================================
static void normalizeChoice(Element &rule) {
    assert(rule.elements.size() > 1);
    vector<Element> tmp;
    for (auto &&elem : rule.elements) {
        if (elem.type != Element::kChoice)
            continue;
        for (auto &&e : elem.elements) {
            tmp.push_back(move(e));
            Element &back = tmp.back();
            back.m *= elem.m;
            back.n = max({back.n, elem.n, back.n * elem.n});
        }
        elem = move(tmp.back());
        tmp.pop_back();
    }
    for (auto &&e : tmp) {
        rule.elements.push_back(move(e));
    }
}

//===========================================================================
static void normalizeSequence(Element &rule) {
    // it's okay to elevate a sub-list if multiplying the ordinals
    // doesn't change them?
    assert(rule.elements.size() > 1);
    for (unsigned i = 0; i < rule.elements.size(); ++i) {
        Element &elem = rule.elements[i];
        if (elem.type != Element::kSequence)
            continue;
        bool skip = false;
        for (auto &&e : elem.elements) {
            if (e.m != elem.m * e.m ||
                e.n != max({elem.n, e.n, elem.n * e.n})) {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;
        if (elem.elements.size() > 1) {
            rule.elements.insert(
                rule.elements.begin() + i + 1,
                elem.elements.begin() + 1,
                elem.elements.end());
        }
        Element tmp = move(elem.elements.front());
        elem = move(tmp);
        i -= 1;
    }
}

//===========================================================================
static void normalizeRule(Element &rule, const set<Element> &rules) {
    Element elem;
    elem.name = rule.value;
    rule.rule = &*rules.find(elem);
}

//===========================================================================
static void normalize(Element &rule, const set<Element> &rules) {
    if (rule.elements.size() == 1) {
        Element &elem = rule.elements.front();
        rule.m *= elem.m;
        rule.n = max({rule.n, elem.n, rule.n * elem.n});
        rule.type = elem.type;
        rule.value = elem.value;
        vector<Element> tmp{move(elem.elements)};
        rule.elements = move(tmp);
        return normalize(rule, rules);
    }

    for (auto &&elem : rule.elements)
        normalize(elem, rules);

    switch (rule.type) {
    case Element::kChoice: normalizeChoice(rule); break;
    case Element::kSequence: normalizeSequence(rule); break;
    case Element::kRule: normalizeRule(rule, rules); break;
    }
}

//===========================================================================
static void normalize(set<Element> &rules) {
    for (auto &&rule : rules) {
        normalize(const_cast<Element &>(rule), rules);
    }
}

//===========================================================================
static void
findRecursion(set<Element> &rules, Element &rule, vector<bool> &used) {
    switch (rule.type) {
    case Element::kChoice:
    case Element::kSequence:
        for (auto &&elem : rule.elements) {
            findRecursion(rules, elem, used);
        }
        break;
    case Element::kRule:
        if (used[rule.rule->id])
            const_cast<Element *>(rule.rule)->recurse = true;
        if (rule.rule->recurse)
            return;
        used[rule.rule->id] = true;
        findRecursion(rules, const_cast<Element &>(*rule.rule), used);
        used[rule.rule->id] = false;
    }
}

//===========================================================================
static void findRecursion(set<Element> &rules, Element &rule) {
    size_t maxId = 0;
    for (auto &&elem : rules) {
        const_cast<Element &>(elem).recurse = false;
        maxId = max((size_t)elem.id, maxId);
    }
    vector<bool> used(maxId, false);
    findRecursion(rules, rule, used);
}

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
static void addRulePositions(State *st, StatePosition *sp, bool init) {
    const Element &elem = *sp->elems.back().elem;

    if (elem.rule->recurse) {
        st->positions.insert(*sp);
        return;
    }

    // check for rule recursion
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        StateElement *m = sp->elems.data() + depth / 2;
        StateElement *z = &sp->elems.back();
        for (StateElement *y = z - 1; y >= m; --y) {
            if (y->elem == z->elem) {
                StateElement *x = y - (z - y);
                if (equal(x + 1, y, y + 1, z)) {
                    if (!init) {
                        vector<StateElement> tmp{sp->elems};
                        size_t num = y - sp->elems.data() + 1;
                        sp->elems.resize(num);
                        sp->elems.back().rep = 0;
                        addPositions(st, sp, false, *elem.rule, 0);
                        sp->elems = move(tmp);
                    }
                    return;
                }
            }
        }
    }

    addPositions(st, sp, init, *elem.rule, 0);
}

//===========================================================================
static void addPositions(
    State *st,
    StatePosition *sp,
    bool init,
    const Element &rule,
    unsigned rep) {
    StateElement se;
    se.elem = &rule;
    se.rep = rep;
    sp->elems.push_back(se);
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto &&elem : rule.elements) {
            addPositions(st, sp, true, elem, 0);
        }
        break;
    case Element::kRule: addRulePositions(st, sp, init); break;
    case Element::kSequence:
        // up to first with a minimum
        for (auto &&elem : rule.elements) {
            addPositions(st, sp, true, elem, 0);
            if (elem.m)
                goto done;
        }
        break;
    case Element::kTerminal:
        st->positions.insert(*sp);
        // cout << *sp << endl;
        break;
    }
done:
    sp->elems.pop_back();
}

//===========================================================================
static void
initPositions(State *st, const set<Element> &rules, const string &root) {
    Element key;
    key.name = root;
    const Element &rule = *rules.find(key);
    st->positions.clear();
    StatePosition sp;
    addPositions(st, &sp, true, rule, 0);
}

//===========================================================================
static void addNextPositions(State *st, const StatePosition &sp) {
    if (sp.recurse)
        st->positions.insert(sp);

    auto it = sp.elems.rbegin();
    auto eit = sp.elems.rend();
    for (; it != eit; ++it) {
        const StateElement &se = *it;
        bool fromRecurse = false;

        // advance to next in sequence
        if (se.elem->type == Element::kSequence) {
            const Element *cur = (it - 1)->elem;
            const Element *last = &se.elem->elements.back();

            if (cur->type == Element::kRule && cur->rule->recurse)
                fromRecurse = true;

            if (cur != last) {
                StatePosition nsp;
                nsp.recurse = fromRecurse;
                nsp.elems.assign(sp.elems.begin(), it.base());
                do {
                    cur += 1;
                    addPositions(st, &nsp, false, *cur, 0);
                    if (cur->m)
                        return;
                } while (cur != last);
            }
        }

        // repeat and/or advance to next
        unsigned rep = se.rep + 1;
        unsigned m = se.elem->m;
        unsigned n = se.elem->n;
        // repeat if rep < n
        if (rep < n) {
            if (rep >= m && n == kUnlimited) {
                // when the count is unlimited collapse all counts >= m
                // to the same state
                rep = m;
            }
            StatePosition nsp;
            nsp.recurse = fromRecurse;
            nsp.elems.assign(sp.elems.begin(), --it.base());
            addPositions(st, &nsp, false, *se.elem, rep);
        }
        // don't advance unless rep >= m
        if (rep < m)
            return;

        // advance parent
    }

    // completed parsing
    StatePosition nsp;
    StateElement nse;
    nse.elem = &s_done;
    nse.rep = 0;
    nsp.elems.push_back(nse);
    st->positions.insert(nsp);
}

//===========================================================================
static void
buildStateTree(State *st, string *path, unordered_set<State> &states) {
    st->next.assign(256, 0);
    State next;
    for (unsigned i = 0; i < 256; ++i) {
        next.clear();
        for (auto &&sp : st->positions) {
            auto &se = sp.elems.back();
            if (se.elem->type == Element::kTerminal) {
                if ((unsigned char)se.elem->value.front() == i)
                    addNextPositions(&next, sp);
            } else {
                assert(se.elem->type == Element::kRule);
                if (i == 0 && se.elem == &s_done)
                    st->next[i] = 1;
            }
        }
        if (next.positions.size()) {
            auto ib = states.insert(move(next));
            State *st2 = const_cast<State *>(&*ib.first);

            if (i <= ' ') {
                path->push_back('\\');
                switch (i) {
                case 9: path->push_back('t'); break;
                case 10: path->push_back('n'); break;
                case 13: path->push_back('r'); break;
                case 32: path->push_back('s'); break;
                }
            } else {
                path->push_back(unsigned char(i));
            }

            ++s_transitions;
            if (ib.second) {
                st2->id = ++s_nextStateId;
                st2->name = *path;
                const char *show = path->data();
                if (path->size() > 40) {
                    show += path->size() - 40;
                }
                logMsgInfo() << s_nextStateId << " states, " << s_transitions
                             << " transitions, "
                             << "(" << path->size() << " chars) ..." << show;
                buildStateTree(st2, path, states);
            }
            path->pop_back();
            if (i <= ' ')
                path->pop_back();

            st->next[i] = st2->id;
        }
    }
    if (!path->size()) {
        logMsgInfo() << s_nextStateId << " states, " << s_transitions
                     << " transitions";
    }
}

//===========================================================================
static void buildStateTree(
    unordered_set<State> *states,
    const set<Element> &rules,
    const string &root,
    bool inclDeps) {
    logMsgInfo() << "rule: " << root;

    states->clear();
    State state;
    s_nextStateId = 0;
    s_transitions = 0;
    state.id = ++s_nextStateId;
    state.name = kDoneStateName;
    StatePosition nsp;
    StateElement nse;
    nse.elem = &s_done;
    nse.rep = 0;
    nsp.elems.push_back(nse);
    state.positions.insert(nsp);
    states->insert(move(state));

    state.clear();
    if (inclDeps) {
        initPositions(&state, rules, root);
        state.id = ++s_nextStateId;
        auto ib = states->insert(move(state));

        string path;
        buildStateTree(const_cast<State *>(&*ib.first), &path, *states);
    }
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
        if (elem->type != Element::kTerminal && elem != &s_done) {
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

state0: // <FAILED>
    return false;
)";
    vector<const State *> states;
    states.resize(s_nextStateId);
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
*   ElementDone
*
***/

//===========================================================================
ElementDone::ElementDone() {
    type = Element::kRule;
    value = kDoneStateName;
}


/****************************************************************************
 *
 *   StateElement and StatePosition
 *
 ***/

//===========================================================================
int StateElement::compare(const StateElement &right) const {
    if (int rc = (int)(elem->id - right.elem->id))
        return rc;
    int rc = rep - right.rep;
    return rc;
}

//===========================================================================
bool StateElement::operator<(const StateElement &right) const {
    return compare(right) < 0;
}

//===========================================================================
bool StateElement::operator==(const StateElement &right) const {
    return compare(right) == 0;
}

//===========================================================================
bool StatePosition::operator<(const StatePosition &right) const {
    return recurse < right.recurse ||
           recurse == right.recurse && elems < right.elems;
}

//===========================================================================
bool StatePosition::operator==(const StatePosition &right) const {
    return elems == right.elems && recurse == right.recurse;
}


/****************************************************************************
*
*   Internal API
*
***/

static bool s_markRecursion = true;
static bool s_buildStateTree = true;
static bool s_writeStatePositions = false;

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
