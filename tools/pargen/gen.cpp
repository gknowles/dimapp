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

static void hashCombine(size_t & seed, size_t v);


/****************************************************************************
*
*   Variables
*
***/

static unsigned s_nextStateId = 1;
static unsigned s_transitions;


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static void normalizeSequence(Element & rule);
static void addPositions(
    bool * skippable,
    State * st,
    StatePosition * sp,
    bool init,
    const Element & elem,
    unsigned rep);

//===========================================================================
static void hashCombine(size_t & seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


/****************************************************************************
*
*   ElementDone
*
***/

ElementDone ElementDone::s_elem;

//===========================================================================
ElementDone::ElementDone() {
    type = Element::kRule;
    value = kDoneRuleName;
    rule = this;
}


/****************************************************************************
*
*   ElementNull
*
***/

ElementNull ElementNull::s_elem;

//===========================================================================
ElementNull::ElementNull() {
    type = Element::kTerminal;
    value = '\0';
}


/****************************************************************************
*
*   StateElement
*
***/

namespace std {
//===========================================================================
size_t hash<StateElement>::operator()(const StateElement & val) const {
    size_t out = val.elem->id;
    hashCombine(out, val.rep);
    hashCombine(out, val.started);
    return out;
}
} // namespace std

//===========================================================================
int StateElement::compare(const StateElement & right) const {
    if (int rc = (int)(elem->id - right.elem->id))
        return rc;
    if (int rc = rep - right.rep)
        return rc;
    if (int rc = started - right.started)
        return rc;
    return 0;
}

//===========================================================================
bool StateElement::operator<(const StateElement & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool StateElement::operator==(const StateElement & right) const {
    return compare(right) == 0;
}


/****************************************************************************
*
*   StatePosition
*
***/

namespace std {
//===========================================================================
size_t hash<StatePosition>::operator()(const StatePosition & val) const {
    size_t out = 0;
    for (auto && se : val.elems) {
        hashCombine(out, hash<StateElement>{}(se));
    }
    for (auto && sv : val.events) {
        hashCombine(out, hash<StateEvent>{}(sv));
    }
    for (auto && sv : val.delayedEvents) {
        hashCombine(out, hash<StateEvent>{}(sv));
    }
    return out;
}
} // namespace std

//===========================================================================
bool StatePosition::operator<(const StatePosition & right) const {
    return make_tuple(recurse, elems, events, delayedEvents) <
           make_tuple(
               right.recurse, right.elems, right.events, right.delayedEvents);
}

//===========================================================================
bool StatePosition::operator==(const StatePosition & right) const {
    return recurse == right.recurse && elems == right.elems &&
           events == right.events && delayedEvents == right.delayedEvents;
}


/****************************************************************************
*
*   StateEvent
*
***/

namespace std {
//===========================================================================
size_t hash<StateEvent>::operator()(const StateEvent & val) const {
    size_t out = val.elem->id;
    hashCombine(out, val.flags);
    return out;
}
} // namespace std

//===========================================================================
int StateEvent::compare(const StateEvent & right) const {
    if (int rc = (int)(elem->id - right.elem->id))
        return rc;
    int rc = flags - right.flags;
    return rc;
}

//===========================================================================
bool StateEvent::operator<(const StateEvent & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool StateEvent::operator==(const StateEvent & right) const {
    return compare(right) == 0;
}


/****************************************************************************
*
*   State
*
***/

namespace std {
//===========================================================================
size_t hash<State>::operator()(const State & val) const {
    size_t out = 0;
    for (auto && sp : val.positions) {
        hashCombine(out, hash<StatePosition>{}(sp));
    }
    return out;
}
} // namespace std

//===========================================================================
void State::clear() {
    name.clear();
    id = 0;
    positions.clear();
    next.clear();
}

//===========================================================================
bool State::operator==(const State & right) const {
    return positions == right.positions;
}


/****************************************************************************
*
*   process options
*
***/

//===========================================================================
bool processOptions(std::set<Element> & rules) {
    if (!*getOptionString(rules, kOptionRoot)) {
        logMsgError() << "Option not found, " << kOptionRoot;
        return false;
    }
    string prefix = getOptionString(rules, kOptionApiPrefix);
    if (prefix.empty()) {
        logMsgError() << "Option not found, " << kOptionApiPrefix;
        return false;
    }
    transform(prefix.begin(), prefix.end(), prefix.begin(), tolower);
    if (!*getOptionString(rules, kOptionApiHeaderFile)) {
        addOption(rules, kOptionApiHeaderFile, prefix + "parse.h");
    }
    if (!*getOptionString(rules, kOptionApiCppFile)) {
        addOption(rules, kOptionApiCppFile, prefix + "parse.cpp");
    }
    return true;
}


/****************************************************************************
*
*   copy rules
*
***/

//===========================================================================
static bool copyRequiredDeps(
    set<Element> & rules, const set<Element> & src, const Element & root) {
    switch (root.type) {
    case Element::kSequence:
    case Element::kChoice:
        for (auto && elem : root.elements)
            if (!copyRequiredDeps(rules, src, elem))
                return false;
        break;
    case Element::kRule:
        if (!copyRules(rules, src, root.value, false))
            return false;
    }
    return true;
}

//===========================================================================
bool copyRules(
    set<Element> & rules,
    const set<Element> & src,
    const string & root,
    bool failIfExists) {
    Element key;
    key.name = root;
    key.type = Element::kRule;
    auto it = src.find(key);
    if (it == src.end()) {
        logMsgError() << "Rule not found, " << root;
        return false;
    }
    auto ib = rules.insert(*it);
    if (!ib.second) {
        if (failIfExists) {
            logMsgError() << "Rule already exists, " << root;
            return false;
        }
        return true;
    }
    auto & elem = *ib.first;
    if (elem.recurse && (elem.flags & Element::kOnChar)) {
        logMsgError() << "Rule with both recurse and onChar, " << elem.value;
        return false;
    }
    return copyRequiredDeps(rules, src, elem);
}


/****************************************************************************
*
*   normalize
*
***/

//===========================================================================
static void normalizeChoice(Element & rule) {
    assert(rule.elements.size() > 1);
    vector<Element> tmp;
    for (auto && elem : rule.elements) {
        if (elem.type != Element::kChoice) {
            tmp.push_back(move(elem));
            continue;
        }
        for (auto && e : elem.elements) {
            tmp.push_back(move(e));
            Element & back = tmp.back();
            back.m *= elem.m;
            back.n = max({back.n, elem.n, back.n * elem.n});
        }
    }
    rule.elements = tmp;
    for (auto && elem : rule.elements)
        elem.pos = 0;
}

//===========================================================================
static void normalizeSequence(Element & rule) {
    // it's okay to elevate a sub-list if multiplying the ordinals
    // doesn't change them?
    assert(rule.elements.size() > 1);
    for (unsigned i = 0; i < rule.elements.size(); ++i) {
        Element * elem = rule.elements.data() + i;
        if (elem->type != Element::kSequence)
            continue;
        bool skip = false;
        for (auto && e : elem->elements) {
            if (e.m != elem->m * e.m ||
                e.n != max({elem->n, e.n, elem->n * e.n})) {
                skip = true;
                break;
            }
        }
        if (skip)
            continue;
        if (elem->elements.size() > 1) {
            rule.elements.insert(
                rule.elements.begin() + i + 1,
                elem->elements.begin() + 1,
                elem->elements.end());
            elem = rule.elements.data() + i;
        }
        Element tmp = move(elem->elements.front());
        *elem = move(tmp);
        i -= 1;
    }
    int pos = 0;
    for (auto && elem : rule.elements) 
        elem.pos = ++pos;
}

//===========================================================================
static void normalizeRule(Element & rule, const set<Element> & rules) {
    Element elem;
    elem.name = rule.value;
    auto bi = rules.find(elem);
    if (bi == rules.end()) {
        rule.rule = nullptr;
    } else {
        rule.rule = &*bi;
    }
}

//===========================================================================
static void normalize(Element & rule, const set<Element> & rules) {
    if (rule.elements.size() == 1) {
        Element & elem = rule.elements.front();
        rule.m *= elem.m;
        rule.n = max({rule.n, elem.n, rule.n * elem.n});
        rule.type = elem.type;
        rule.value = elem.value;
        vector<Element> tmp{move(elem.elements)};
        rule.elements = move(tmp);
        return normalize(rule, rules);
    }

    for (auto && elem : rule.elements)
        normalize(elem, rules);

    switch (rule.type) {
    case Element::kChoice: normalizeChoice(rule); break;
    case Element::kSequence: normalizeSequence(rule); break;
    case Element::kRule: normalizeRule(rule, rules); break;
    }
}

//===========================================================================
void normalize(set<Element> & rules) {
    for (auto && rule : rules) {
        normalize(const_cast<Element &>(rule), rules);
    }
}


/****************************************************************************
*
*   mark recursion
*
***/

//===========================================================================
static void
markRecursion(set<Element> & rules, Element & rule, vector<bool> & used) {
    bool wasUsed;
    switch (rule.type) {
    case Element::kChoice:
    case Element::kSequence:
        for (auto && elem : rule.elements) {
            markRecursion(rules, elem, used);
        }
        break;
    case Element::kRule:
        wasUsed = used[rule.rule->id];
        if (wasUsed && (~rule.rule->flags & Element::kOnChar))
            const_cast<Element *>(rule.rule)->recurse = true;
        if (rule.rule->recurse)
            return;
        used[rule.rule->id] = true;
        markRecursion(rules, const_cast<Element &>(*rule.rule), used);
        used[rule.rule->id] = wasUsed;
    }
}

//===========================================================================
void markRecursion(set<Element> & rules, Element & rule, bool reset) {
    size_t maxId = 0;
    for (auto && elem : rules) {
        if (reset)
            const_cast<Element &>(elem).recurse = false;
        maxId = max((size_t)elem.id, maxId);
    }
    vector<bool> used(maxId, false);
    markRecursion(rules, rule, used);
}


/****************************************************************************
*
*   build state tree
*
***/

//===========================================================================
static void
addRulePositions(bool * skippable, State * st, StatePosition * sp, bool init) {
    const Element & elem = *sp->elems.back().elem;
    *skippable = false;
    // Don't generate states for right recursion when it can be broken with
    // a call. This could also be done for left recursion when the grammar
    // allows it, but that's more difficult to determine.
    if (elem.rule->recurse /* && !init */) {
        st->positions.insert(*sp);
        return;
    }

    // check for rule recursion
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        StateElement * m = sp->elems.data() + depth / 2;
        StateElement * z = &sp->elems.back();
        for (StateElement * y = z - 1; y >= m; --y) {
            if (y->elem == z->elem) {
                StateElement * x = y - (z - y);
                if (equal(x + 1, y, y + 1, z)) {
                    if (!init) {
                        vector<StateElement> tmp{sp->elems};
                        size_t num = y - sp->elems.data() + 1;
                        sp->elems.resize(num);
                        sp->elems.back().rep = 0;
                        addPositions(skippable, st, sp, false, *elem.rule, 0);
                        sp->elems = move(tmp);
                    }
                    return;
                }
            }
        }
    }

    addPositions(skippable, st, sp, init, *elem.rule, 0);
}

//===========================================================================
static void addPositions(
    bool * skippable,
    State * st,
    StatePosition * sp,
    bool init,
    const Element & rule,
    unsigned rep) {
    *skippable = false;
    StateElement se;
    se.elem = &rule;
    se.rep = rep;
    sp->elems.push_back(se);
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto && elem : rule.elements) {
            bool weak;
            addPositions(&weak, st, sp, true, elem, 0);
            if (weak)
                *skippable = true;
        }
        break;
    case Element::kRule: addRulePositions(skippable, st, sp, init); break;
    case Element::kSequence:
        // up to first with a minimum
        for (auto && elem : rule.elements) {
            bool weak;
            addPositions(&weak, st, sp, true, elem, 0);
            if (!weak)
                goto done;
        }
        *skippable = true;
        break;
    case Element::kTerminal:
        st->positions.insert(*sp);
        // cout << *sp << endl;
        break;
    }
done:
    if (rule.m == 0)
        *skippable = true;
    sp->elems.pop_back();
}

//===========================================================================
static void
initPositions(State * st, const set<Element> & rules, const string & root) {
    Element key;
    key.name = root;
    const Element & rule = *rules.find(key);
    st->positions.clear();
    bool skippable;
    StatePosition sp;
    addPositions(&skippable, st, &sp, true, rule, 0);
}

//===========================================================================
static void addEvent(
    vector<StateEvent> & events,
    const StateElement & se,
    Element::Flags flags) {
    StateEvent sv;
    sv.elem = se.elem->rule;
    sv.flags = flags;
    events.push_back(sv);
}

//===========================================================================
template <typename Iter>
static void setPositionPrefix(
    StatePosition * sp,
    Iter begin,
    Iter end,
    bool recurse,
    const vector<StateEvent> & events,
    bool terminal) {
    sp->recurse = recurse;
    sp->elems.assign(begin, end);
    sp->events = events;
    if (terminal) {
        for (auto && se : sp->elems) {
            if (se.elem->type == Element::kRule)
                se.started = true;
        }
    }
}

//===========================================================================
static void addNextPositions(State * st, const StatePosition & sp) {
    if (sp.recurse)
        st->positions.insert(sp);

    bool terminal{false};
    auto terminalStarted = sp.elems.end();
    bool done = sp.elems.front().elem == &ElementDone::s_elem;
    vector<StateEvent> events;
    auto it = sp.elems.rbegin();
    auto eit = sp.elems.rend();
    if (it->elem->type == Element::kTerminal || done && sp.elems.size() == 1) {
        events = sp.delayedEvents;
    }
    for (; it != eit; ++it) {
        const StateElement & se = *it;
        bool fromRecurse = false;

        // advance to next in sequence
        if (se.elem->type == Element::kSequence) {
            const Element * cur = (it - 1)->elem;
            const Element * last = &se.elem->elements.back();

            if (cur->type == Element::kRule && cur->rule->recurse)
                fromRecurse = true;

            if (cur != last) {
                StatePosition nsp;
                setPositionPrefix(
                    &nsp,
                    sp.elems.begin(),
                    it.base(),
                    fromRecurse,
                    events,
                    terminal);
                do {
                    cur += 1;
                    bool weak;
                    addPositions(&weak, st, &nsp, false, *cur, 0);
                    if (!weak)
                        return;
                } while (cur != last);
            }
        }

        if (se.elem->type == Element::kRule) {
            if ((se.elem->rule->flags & Element::kOnEnd) &&
                (se.started || !done) && !se.elem->rule->recurse) {
                addEvent(events, se, Element::kOnEnd);
            }
            // when exiting the parser (via a done sentinel) go directly out,
            // no not pass go, do not honor repititions
            if (done)
                continue;
        }

        if (se.elem->type == Element::kTerminal) {
            assert(it == sp.elems.rbegin());
            terminal = true;
            if (!done) {
                // add OnStart events top to bottom
                for (auto && se : sp.elems) {
                    if (se.elem->type == Element::kRule &&
                        (se.elem->rule->flags & Element::kOnStart) &&
                        !se.started) {
                        addEvent(events, se, Element::kOnStart);
                    }
                }
                // bubble up OnChar events bottom to top
                for (auto it2 = it; it2 != eit; ++it2) {
                    if (it2->elem->type == Element::kRule &&
                        (it2->elem->rule->flags & Element::kOnChar)) {
                        addEvent(events, *it2, Element::kOnChar);
                    }
                }
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
            auto se_end = --it.base();
            if (terminalStarted == sp.elems.end())
                terminalStarted = se_end;
            StatePosition nsp;
            setPositionPrefix(
                &nsp, sp.elems.begin(), se_end, fromRecurse, events, terminal);
            bool weak;
            addPositions(&weak, st, &nsp, false, *se.elem, rep);
        }
        // don't advance unless rep >= m
        if (rep < m)
            return;

        // advance parent
    }

    //-----------------------------------------------------------------------
    // completed parsing

    StatePosition nsp;
    nsp.events = events;
    StateElement nse;
    nse.elem = &ElementDone::s_elem;
    nse.rep = 0;
    nsp.elems.push_back(nse);
    if (done) {
        // add entry for completed done
        st->positions.insert(nsp);
        return;
    }

    // add position state for null terminator
    auto elemBase = sp.elems.begin();
    for (auto it = elemBase; it != terminalStarted; ++it) {
        if (it->elem->type == Element::kRule) {
            nsp.elems.push_back(*it);
            nsp.elems.back().started = true;
        }
    }
    for (auto it = terminalStarted; it != sp.elems.end(); ++it) {
        if (it->elem->type == Element::kRule && it->started) {
            nsp.elems.push_back(*it);
        }
    }

    // remove end events generated when unwinding past all terminals, they'll
    // be executed by the completed done
    auto sv_end = nsp.events.end();
    auto it2 = remove_if(nsp.events.begin(), sv_end, [&](const auto & a) {
        if (a.flags & Element::kOnEnd) {
            for (auto it = elemBase; it != terminalStarted; ++it) {
                if (it->elem == a.elem)
                    return true;
            }
        }
        return false;
    });
    nsp.events.erase(it2, sv_end);

    bool weak;
    addPositions(&weak, st, &nsp, false, ElementNull::s_elem, 0);
}

//===========================================================================
// when positions only vary by the set of events, remove all but the
// first - which should also be the one with the simplest events.
static void removePositionsWithMoreEvents(set<StatePosition> & positions) {
    if (positions.size() < 2)
        return;
    auto x = positions.begin();
    auto y = next(x);
    auto e = positions.end();
    while (y != e) {
        auto n = next(y);
        if (x->recurse == y->recurse && x->elems == y->elems) {
            positions.erase(y);
        } else {
            x = y;
        }
        y = n;
    }
}

//===========================================================================
static void removeConflicts(
    vector<StateEvent> & matched,
    const vector<StateEvent> & events
) {
    if (matched.empty())
        return;
    int numEvents = (int) events.size();
    vector<bool> used(numEvents);
    auto mi = matched.begin();
    int evi = 0;
    while (mi != matched.end()) {
        for (evi = 0; evi < numEvents; ++evi) {
            if (!used[evi])
                goto found_unused;
        }
        matched.erase(mi, matched.end());
        return;

    found_unused:
        if (*mi == events[evi]) 
            goto matched;
        if (mi->flags & Element::kOnChar) {
            // char can shift around start events
            for (;;) {
                if ((~events[evi].flags & Element::kOnStart)
                    || ++evi == numEvents
                ) {
                    goto unmatched;
                }
                if (*mi == events[evi]) 
                    goto matched;
            }
        } else if (mi->flags & Element::kOnStart) {
            // start events can shift around char events.
            for (;;) {
                if ((~events[evi].flags & Element::kOnChar)
                    || ++evi == numEvents
                    ) {
                    goto unmatched;
                }
                if (*mi == events[evi]) 
                    goto matched;
            }
        }
        assert(mi->flags & Element::kOnEnd);

    unmatched:
        mi = matched.erase(mi);
        continue;

    matched:
        used[evi] = true;
        ++mi;
    }
}

//===========================================================================
// returns false if there are conflicts that can't be delayed
static bool delayConflicts(
    StatePosition & nsp,
    const vector<StateEvent> & matched,
    const string & path
) {
    for (auto && sv : matched) {
        auto evi = nsp.events.begin();
        auto e = nsp.events.end();
        for (; evi != e; ++evi) {
            if (sv == *evi) {
                nsp.events.erase(evi);
                break;
            }
        }
    }
    nsp.delayedEvents = move(nsp.events);
    nsp.events = matched;

    bool success = true;
    for (auto && sv : nsp.delayedEvents) {
        if (sv.flags & Element::kOnChar) {
            success = false;
            logMsgError() << "Conflicting parse events, "
                << sv.elem->name << " at "
                << path;
        }
    }
    return success;
}

//===========================================================================
static bool resolveEventConflicts(
    set<StatePosition> & positions, 
    const string & path
) {
    if (positions.size() == 1)
        return true;

    // allow Char to match by moving forward in list
    vector<StateEvent> matched;
    auto b = positions.begin();
    auto e = positions.end();
    matched = b->events;
    size_t most = matched.size();
    for (++b; b != e; ++b) {
        most = max({ b->events.size(), most });
        removeConflicts(matched, b->events);
    }
    if (most == matched.size())
        return true;

    bool success = true;
    set<StatePosition> next;
    for (auto && sp : positions) {
        if (sp.events.size() == matched.size()) {
            next.insert(move(sp));
            continue;
        }
        StatePosition nsp;
        nsp.elems = move(sp.elems);
        nsp.recurse = sp.recurse;
        nsp.events = move(sp.events);
        if (!delayConflicts(nsp, matched, path))
            success = false;
        next.insert(move(nsp));
    }
    positions = move(next);
    return success;
}

//===========================================================================
static void
buildStateTree(State * st, string * path, unordered_set<State> & states) {
    st->next.assign(257, 0);
    State next;
    bool errors{false};
    for (unsigned i = 0; i < 257; ++i) {
        size_t pathLen = path->size();
        if (i <= ' ' || i == 256) {
            path->push_back('\\');
            switch (i) {
            case 0: path->push_back('0'); break;
            case 9: path->push_back('t'); break;
            case 10: path->push_back('n'); break;
            case 13: path->push_back('r'); break;
            case 32: path->push_back('s'); break;
            case 256: path->push_back('*'); break;
            }
        } else {
            path->push_back(unsigned char(i));
        }

        next.clear();
        for (auto && sp : st->positions) {
            auto & se = sp.elems.back();
            if (se.elem->type == Element::kTerminal) {
                if ((unsigned char)se.elem->value.front() == i)
                    addNextPositions(&next, sp);
            } else {
                if (se.elem == &ElementDone::s_elem) {
                    if (i == 0)
                        st->next[i] = 1;
                } else if (i == 256) {
                    assert(se.elem->type == Element::kRule);
                    assert(se.elem->rule->recurse);
                    if (next.positions.size()) {
                        errors = true;
                        logMsgError() << "Multiple recursive targets, "
                                      << *path;
                    }
                    addNextPositions(&next, sp);
                }
            }
        }
        if (!next.positions.empty()) {

            errors = !resolveEventConflicts(next.positions, *path) || errors;
            removePositionsWithMoreEvents(next.positions);

            // add state and all of it's child states
            auto ib = states.insert(move(next));
            State * st2 = const_cast<State *>(&*ib.first);

            ++s_transitions;
            if (ib.second) {
                st2->id = ++s_nextStateId;
                st2->name = *path;
                const char * show = path->data();
                if (path->size() > 40) {
                    show += path->size() - 40;
                }
                logMsgInfo() << s_nextStateId << " states, " << s_transitions
                             << " transitions, "
                             << "(" << path->size() << " chars, "
                             << st2->positions.size() << " exits) ..." << show;
                //if (path->size() > 40) errors = true;
                if (!errors)
                    buildStateTree(st2, path, states);
            }
            st->next[i] = st2->id;
        }
        path->resize(pathLen);
    }
    if (!path->size()) {
        logMsgInfo() << s_nextStateId << " states, " << s_transitions
                     << " transitions";
    }
}

//===========================================================================
void buildStateTree(
    unordered_set<State> * states,
    const set<Element> & rules,
    const string & root,
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
    nse.elem = &ElementDone::s_elem;
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
