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
    value = kDoneElement;
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
    // name.clear();
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
*   copy rules
*
***/

//===========================================================================
static void copyRequiredDeps(
    set<Element> & rules, const set<Element> & src, const Element & root) {
    switch (root.type) {
    case Element::kSequence:
    case Element::kChoice:
        for (auto && elem : root.elements)
            copyRequiredDeps(rules, src, elem);
        break;
    case Element::kRule: copyRules(rules, src, root.value, false); break;
    }
}

//===========================================================================
void copyRules(
    set<Element> & rules,
    const set<Element> & src,
    const string & root,
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
        if (elem.type != Element::kChoice)
            continue;
        for (auto && e : elem.elements) {
            tmp.push_back(move(e));
            Element & back = tmp.back();
            back.m *= elem.m;
            back.n = max({back.n, elem.n, back.n * elem.n});
        }
        elem = move(tmp.back());
        tmp.pop_back();
    }
    for (auto && e : tmp) {
        rule.elements.push_back(move(e));
    }
}

//===========================================================================
static void normalizeSequence(Element & rule) {
    // it's okay to elevate a sub-list if multiplying the ordinals
    // doesn't change them?
    assert(rule.elements.size() > 1);
    for (unsigned i = 0; i < rule.elements.size(); ++i) {
        Element & elem = rule.elements[i];
        if (elem.type != Element::kSequence)
            continue;
        bool skip = false;
        for (auto && e : elem.elements) {
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
static void normalizeRule(Element & rule, const set<Element> & rules) {
    Element elem;
    elem.name = rule.value;
    rule.rule = &*rules.find(elem);
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
    switch (rule.type) {
    case Element::kChoice:
    case Element::kSequence:
        for (auto && elem : rule.elements) {
            markRecursion(rules, elem, used);
        }
        break;
    case Element::kRule:
        if (used[rule.rule->id])
            const_cast<Element *>(rule.rule)->recurse = true;
        if (rule.rule->recurse)
            return;
        used[rule.rule->id] = true;
        markRecursion(rules, const_cast<Element &>(*rule.rule), used);
        used[rule.rule->id] = false;
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
static void addRulePositions(State * st, StatePosition * sp, bool init) {
    const Element & elem = *sp->elems.back().elem;

    // Don't generate states for right recursion when it can be broken with
    // a call. This could also be done for left recursion when the grammar
    // allows it, but that's more difficult to determine.
    if (elem.rule->recurse && !init) {
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
    State * st,
    StatePosition * sp,
    bool init,
    const Element & rule,
    unsigned rep) {
    StateElement se;
    se.elem = &rule;
    se.rep = rep;
    sp->elems.push_back(se);
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto && elem : rule.elements) {
            addPositions(st, sp, true, elem, 0);
        }
        break;
    case Element::kRule: addRulePositions(st, sp, init); break;
    case Element::kSequence:
        // up to first with a minimum
        for (auto && elem : rule.elements) {
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
initPositions(State * st, const set<Element> & rules, const string & root) {
    Element key;
    key.name = root;
    const Element & rule = *rules.find(key);
    st->positions.clear();
    StatePosition sp;
    addPositions(st, &sp, true, rule, 0);
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
    vector<StateEvent> events = sp.delayedEvents;
    auto it = sp.elems.rbegin();
    auto eit = sp.elems.rend();
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
                    addPositions(st, &nsp, false, *cur, 0);
                    if (cur->m)
                        return;
                } while (cur != last);
            }
        }

        if (se.elem->type == Element::kRule) {
            if ((se.elem->rule->flags & Element::kOnEnd) &&
                (se.started || !done)) {
                addEvent(events, se, Element::kOnEnd);
            }
            // when exiting the parser (via a done sentinel) go directly out,
            // no not pass go, do not honor repititions
            if (done)
                continue;
        }

        if (se.elem->type == Element::kTerminal) {
            terminal = true;
            for (auto it2 = it; it2 != eit; ++it2) {
                if (it2->elem->type == Element::kRule && !done) {
                    unsigned flags = it2->elem->rule->flags;
                    if ((flags & Element::kOnStart) && !it2->started) {
                        addEvent(events, *it2, Element::kOnStart);
                    }
                    if (flags & Element::kOnChar) {
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
            addPositions(st, &nsp, false, *se.elem, rep);
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
    assert(terminal);
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

    addPositions(st, &nsp, false, ElementNull::s_elem, 0);
}

//===========================================================================
static void
buildStateTree(State * st, string * path, unordered_set<State> & states) {
    st->next.assign(257, 0);
    State next;
    for (unsigned i = 0; i < 257; ++i) {
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
                        logMsgError() << "Multiple recursive targets, "
                                      << *path;
                    }
                    addNextPositions(&next, sp);
                }
            }
        }
        if (size_t numpos = next.positions.size()) {
            unordered_map<StateEvent, int> counts;
            for (auto && sp : next.positions) {
                for (auto && sv : sp.events) {
                    counts[sv] += 1;
                }
            }
            unordered_set<StateEvent> conflicts;
            for (auto && cnt : counts) {
                if (cnt.second != numpos) {
                    conflicts.insert(cnt.first);
                    if (cnt.first.flags & Element::kOnChar) {
                        logMsgError() << "Conflicting parse events, " << *path;
                    }
                }
            }
            if (!conflicts.empty()) {
                set<StatePosition> positions;
                for (auto && sp : next.positions) {
                    StatePosition nsp;
                    nsp.elems = sp.elems;
                    nsp.recurse = sp.recurse;
                    for (auto && sv : sp.events) {
                        if (conflicts.count(sv)) {
                            nsp.delayedEvents.push_back(sv);
                        } else {
                            nsp.events.push_back(sv);
                        }
                    }
                    positions.insert(nsp);
                }
                next.positions = move(positions);
            }

            auto ib = states.insert(move(next));
            State * st2 = const_cast<State *>(&*ib.first);

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
                             << "(" << path->size() << " chars) ..." << show;
                buildStateTree(st2, path, states);
            }
            path->resize(pathLen);

            st->next[i] = st2->id;
        }
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
    state.name = kDoneElement;
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
