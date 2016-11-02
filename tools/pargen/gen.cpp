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
*   Helpers
*
***/

// forward declarations
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

//===========================================================================
bool operator<(const vector<StateElement> & a, const vector<StateElement> & b) {
    auto a1 = a.data();
    auto a2 = a1 + a.size();
    auto b1 = b.data();
    auto b2 = b1 + b.size();
    for (; a1 != a2 && b1 != b2; ++a1, ++b1) {
        if (int rc = a1->compare(*b1))
            return rc < 0;
    }
    return a1 == a2 && b1 != b2;
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
    return tie(recurse, elems, events, delayedEvents) <
           tie(right.recurse, right.elems, right.events, right.delayedEvents);
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
*   build state tree
*
***/

namespace {
struct StateTreeInfo {
    string m_path;
    unsigned m_nextStateId;
    unsigned m_transitions;
};
} // namespace

static bool s_simpleRecursion = false;

//===========================================================================
static size_t findRecursionSimple(StatePosition * sp) {
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        StateElement * first = sp->elems.data();
        StateElement * m = first + depth / 2;
        StateElement * z = first + depth;
        StateElement * y = z - 2;
        for (; y->elem->type != Element::kRule; --y) {
            if (y == m)
                return 0;
        }
        y += 1;
        size_t yz = z - y;
        for (StateElement * x = y - yz - 1; x > first; --x) {
            if (equal(x, x + yz, y, z)) {
                size_t num = x + yz - first;
                return num;
            }
        }
    }
    return 0;
}

//===========================================================================
static size_t findRecursionFull(StatePosition * sp) {
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        StateElement * m = sp->elems.data() + depth / 2;
        StateElement * z = &sp->elems.back();
        for (StateElement * y = z - 1; y >= m; --y) {
            if (y->elem == z->elem) {
                StateElement * x = y - (z - y);
                if (equal(x + 1, y, y + 1, z)) {
                    size_t num = y - sp->elems.data() + 1;
                    return num;
                }
            }
        }
    }
    return 0;
}

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
    size_t num = s_simpleRecursion 
        ? findRecursionSimple(sp) 
        : findRecursionFull(sp);
    if (num) {
        if (!init) {
            vector<StateElement> tmp{sp->elems};
            sp->elems.resize(num);
            sp->elems.back().rep = 0;
            addPositions(skippable, st, sp, false, *elem.rule, 0);
            sp->elems = move(tmp);
        }
        return;
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
        st->positions.insert(st->positions.end(), *sp);
        // cout << *sp << endl;
        break;
    }
done:
    if (rule.m == 0)
        *skippable = true;
    sp->elems.pop_back();
}

//===========================================================================
static void initPositions(State * st, const Element & root) {
    st->positions.clear();
    bool skippable;
    StatePosition sp;
    addPositions(&skippable, st, &sp, true, root, 0);
}

//===========================================================================
static void addEvent(
    vector<StateEvent> & events,
    const StateElement & se,
    Element::Flags flags) {
    StateEvent sv;
    sv.elem = se.elem->rule;
    if (!sv.elem->eventName.empty())
        sv.elem = sv.elem->eventRule;
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
    vector<StateEvent> & matched, const vector<StateEvent> & events) {
    if (matched.empty())
        return;
    int numEvents = (int)events.size();
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
                if ((~events[evi].flags & Element::kOnStart) ||
                    ++evi == numEvents) {
                    goto unmatched;
                }
                if (*mi == events[evi])
                    goto matched;
            }
        } else if (mi->flags & Element::kOnStart) {
            // start events can shift around char events.
            for (;;) {
                if ((~events[evi].flags & Element::kOnChar) ||
                    ++evi == numEvents) {
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
    const StateTreeInfo & sti,
    bool logError) {
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
            if (logError) {
                logMsgError() << "Conflicting parse events, " << sv.elem->name
                              << " at " << sti.m_path;
            }
            break;
        }
    }
    return success;
}

//===========================================================================
static bool resolveEventConflicts(
    set<StatePosition> & positions, const StateTreeInfo & sti) {
    if (positions.size() == 1)
        return true;

    // allow Char to match by moving forward in list
    vector<StateEvent> matched;
    auto b = positions.begin();
    auto e = positions.end();
    matched = b->events;
    size_t most = matched.size();
    for (++b; b != e; ++b) {
        most = max({b->events.size(), most});
        removeConflicts(matched, b->events);
    }
    if (most == matched.size())
        return true;

    bool success = true;
    set<StatePosition> next;
    for (auto && sp : positions) {
        if (sp.events.size() == matched.size()) {
            next.insert(next.end(), move(sp));
            continue;
        }
        StatePosition nsp;
        nsp.elems = move(sp.elems);
        nsp.recurse = sp.recurse;
        nsp.events = move(sp.events);
        if (!delayConflicts(nsp, matched, sti, success))
            success = false;
        next.insert(next.end(), move(nsp));
    }
    positions = move(next);
    return success;
}

//===========================================================================
static void
buildStateTree(State * st, unordered_set<State> & states, StateTreeInfo & sti) {
    st->next.assign(257, 0);
    State next;
    bool errors{false};
    for (unsigned i = 0; i < 257; ++i) {
        size_t pathLen = sti.m_path.size();
        if (i <= ' ' || i == '^' || i == 256) {
            sti.m_path.push_back('^');
            switch (i) {
            case 0: sti.m_path.push_back('0'); break;
            case 9: sti.m_path.push_back('I'); break;
            case 10: sti.m_path.push_back('J'); break;
            case 13: sti.m_path.push_back('M'); break;
            case ' ': sti.m_path.push_back('_'); break;
            case '^': sti.m_path.push_back('^'); break;
            case 256: sti.m_path.push_back('*'); break;
            }
        } else {
            sti.m_path.push_back(unsigned char(i));
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
                    for (auto && nsp : next.positions) {
                        auto && nse = nsp.elems.back();
                        if (nse.elem->type == Element::kRule &&
                            (se.elem->rule != nse.elem->rule ||
                             sp.events != nsp.events)) {
                            errors = true;
                            logMsgError() << "Multiple recursive targets, "
                                          << sti.m_path;
                            break;
                        }
                    }
                    addNextPositions(&next, sp);
                }
            }
        }
        if (!next.positions.empty()) {
            errors = !resolveEventConflicts(next.positions, sti) || errors;
            removePositionsWithMoreEvents(next.positions);

            // add state and, if it's new, all of its child states
            auto ib = states.insert(move(next));
            State * st2 = const_cast<State *>(&*ib.first);

            ++sti.m_transitions;
            if (ib.second) {
                st2->id = ++sti.m_nextStateId;
                st2->name = sti.m_path;
                const char * show = sti.m_path.data();
                if (sti.m_path.size() > 40) {
                    show += sti.m_path.size() - 40;
                }
                logMsgInfo() << sti.m_nextStateId << " states, "
                             << sti.m_transitions << " trans, "
                             << sti.m_path.size() << " chars, "
                             << st2->positions.size() << " exits, ..." << show;
                // if (sti.m_path.size() > 10) errors = true;
                if (!errors)
                    buildStateTree(st2, states, sti);
            }
            st->next[i] = st2->id;
        }
        sti.m_path.resize(pathLen);
    }
    if (!sti.m_path.size()) {
        logMsgInfo() << sti.m_nextStateId << " states, " << sti.m_transitions
                     << " transitions";
    }
}

//===========================================================================
void buildStateTree(
    unordered_set<State> * states,
    const Element & root,
    bool inclDeps,
    bool dedupStates) {
    logMsgInfo() << "rule: " << root.name;

    states->clear();
    State state;
    StateTreeInfo sti;
    sti.m_nextStateId = 0;
    sti.m_transitions = 0;
    state.id = ++sti.m_nextStateId;
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
        initPositions(&state, root);
        state.id = ++sti.m_nextStateId;
        auto ib = states->insert(move(state));
        buildStateTree(const_cast<State *>(&*ib.first), *states, sti);
        if (dedupStates)
            dedupStateTree(*states);
    }
}


/****************************************************************************
*
*   dedup state tree
*
***/

namespace {
struct StateKey {
    vector<StateEvent> events;
    unsigned next[257];

    bool operator==(const StateKey & right) const;
    bool operator!=(const StateKey & right) const;
};
struct StateInfo {
    State * state{nullptr};
    StateKey key;
    map<unsigned, vector<unsigned>> usedBy;
};
} // namespace
namespace std {
template <> struct hash<StateKey> {
    size_t operator()(const StateKey & val) const;
};
} // namespace std
namespace {
struct DedupInfo {
    unordered_set<State> * states{nullptr};
    unordered_map<unsigned, StateInfo> info;
    unordered_map<StateKey, vector<unsigned>> idByKey;

    unsigned lastMapId{0};
    unordered_map<unsigned, unsigned> pmap;
    unordered_map<unsigned, unsigned> qmap;
};
} // namespace


//===========================================================================
// StateKey
//===========================================================================
namespace std {
//===========================================================================
size_t hash<StateKey>::operator()(const StateKey & val) const {
    size_t out = 0;
    for (auto && sv : val.events) {
        hashCombine(out, hash<StateEvent>{}(sv));
    }
    for (auto && i : val.next) {
        hashCombine(out, i);
    }
    return out;
}
} // namespace std

//===========================================================================
bool StateKey::operator==(const StateKey & right) const {
    return events == right.events &&
           memcmp(next, right.next, sizeof(next)) == 0;
}

//===========================================================================
bool StateKey::operator!=(const StateKey & right) const {
    return !operator==(right);
}

//===========================================================================
static void copy(StateKey & out, const State & st) {
    out.events = st.positions.begin()->events;
    if (st.next.empty()) {
        memset(out.next, 0, sizeof(out.next));
        return;
    }
    assert(st.next.size() == 257);
    unordered_map<unsigned, unsigned> idmap;
    unsigned nextId = 0;
    for (unsigned i = 0; i < 257; ++i) {
        if (unsigned id = st.next[i]) {
            unsigned & mapped = idmap[id];
            if (!mapped)
                mapped = ++nextId;
            out.next[i] = mapped;
        } else {
            out.next[i] = 0;
        }
    }
}


//===========================================================================
// Helpers
//===========================================================================

//===========================================================================
static void insertKeyRef(DedupInfo & di, const StateInfo & val) {
    auto & ids = di.idByKey[val.key];
    auto ii = equal_range(ids.begin(), ids.end(), val.state->id);
    assert(ii.first == ii.second);
    ids.insert(ii.first, val.state->id);
}

//===========================================================================
static void eraseKeyRef(DedupInfo & di, const StateInfo & val) {
    auto & ids = di.idByKey[val.key];
    auto ii = equal_range(ids.begin(), ids.end(), val.state->id);
    assert(ii.first != ii.second);
    if (ids.size() == 1) {
        di.idByKey.erase(val.key);
    } else {
        ids.erase(ii.first);
    }
}

//===========================================================================
static void mergeState(unsigned dstId, unsigned srcId, DedupInfo & di) {
    StateInfo & dst = di.info[dstId];
    StateInfo & src = di.info[srcId];

    logMsgInfo() << di.states->size() << " states, merging state " << srcId
                 << " into " << dstId;

    // move down references from src's parents to point to dst
    for (auto && by : src.usedBy) {
        auto & user = di.info[by.first];
        auto & dstUsed = dst.usedBy[by.first];
        for (auto && i : by.second) {
            user.state->next[i] = dstId;
            dstUsed.push_back(i);
        }
        // update key (and idByKey references) of changed parent
        eraseKeyRef(di, user);
        copy(user.key, *user.state);
        insertKeyRef(di, user);
    }

    // remove source from idByKey (after the parents are updated so any
    // recursive references can be handled first).
    eraseKeyRef(di, src);

    // remove back references to source from it's children
    if (!src.state->next.empty()) {
        for (unsigned i = 0; i < 257; ++i) {
            if (unsigned id = src.state->next[i])
                di.info[id].usedBy.erase(srcId);
        }
    }

    // update dst name with merge history
    dst.state->aliases.push_back(to_string(srcId) + ": " + src.state->name);
    dst.state->aliases.insert(
        dst.state->aliases.end(),
        src.state->aliases.begin(),
        src.state->aliases.end());

    // delete source state and it's dedup info
    di.states->erase(*src.state);
    di.info.erase(srcId);
}

//===========================================================================
static bool equalize(unsigned a, unsigned b, DedupInfo & di) {
    StateInfo & x = di.info[a];
    StateInfo & y = di.info[b];
    if (x.key != y.key)
        return false;
    size_t n = x.state->next.size();
    if (n != y.state->next.size())
        return false;
    if (x.state->next == y.state->next)
        return true;
    for (unsigned i = 0; i < n; ++i) {
        unsigned p = x.state->next[i];
        unsigned q = y.state->next[i];
        if (!p && !q)
            continue;
        if (!p || !q)
            return false;
        unsigned ap = di.pmap[p];
        unsigned aq = di.qmap[q];
        if (ap != aq)
            return false;
        if (ap)
            continue;
        di.pmap[p] = ++di.lastMapId;
        di.qmap[q] = di.lastMapId;
        if (p != q && !equalize(p, q, di))
            return false;
    }
    return true;
}

//===========================================================================
// Makes one pass through all the equal keys and returns true if any of the
// assoicated states were equivalent (i.e. got it's references merge with
// another state and removed).
static bool dedupStateTreePass(DedupInfo & di) {
    vector<pair<unsigned, unsigned>> matched;

    for (auto && kv : di.idByKey) {
        for (size_t b = kv.second.size(); b-- > 1;) {
            unsigned y = kv.second[b];
            for (auto && x : kv.second) {
                if (x == y)
                    break;
                di.lastMapId = 0;
                di.pmap.clear();
                di.qmap.clear();
                di.pmap[x] = ++di.lastMapId;
                di.qmap[y] = di.lastMapId;
                if (equalize(x, y, di)) {
                    matched.push_back({x, y});
                    break;
                }
            }
        }
    }
    if (matched.empty())
        return false;

    // Sort descending, by source. Normalizes the processing with respect
    // to hash functions and makes sure that we never remove a future
    // target.
    sort(matched.begin(), matched.end(), [](auto & a, auto & b) {
        return a.second > b.second;
    });
    for (auto && xy : matched) {
        mergeState(xy.first, xy.second, di);
    }
    return true;
}

//===========================================================================
void dedupStateTree(unordered_set<State> & states) {
    DedupInfo di;
    di.states = &states;
    for (auto && st : states) {
        auto & si = di.info[st.id];
        si.state = const_cast<State *>(&st);
        if (!st.next.empty()) {
            for (unsigned i = 0; i < 257; ++i) {
                if (unsigned id = st.next[i]) {
                    di.info[id].usedBy[st.id].push_back(i);
                }
            }
        }
        copy(si.key, st);
        insertKeyRef(di, si);
    }

    // keep making dedup passes through all the keys until no more dups are
    // found.
    for (unsigned i = 1;; ++i) {
        logMsgDebug() << "dedup pass #" << i;
        if (!dedupStateTreePass(di))
            break;
    }
    logMsgDebug() << states.size() << " unique states";
}
