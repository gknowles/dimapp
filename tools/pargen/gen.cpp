// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// gen.cpp - pargen
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
static void addPositions(
    bool * skippable,
    State * st,
    StatePosition * sp,
    bool init,
    const Element & elem,
    unsigned rep
);


/****************************************************************************
*
*   StateElement
*
***/

//===========================================================================
size_t std::hash<StateElement>::operator()(const StateElement & val) const {
    size_t out = val.elem->id;
    hashCombine(out, val.rep);
    hashCombine(out, val.started);
    hashCombine(out, val.recurse);
    return out;
}

//===========================================================================
int StateElement::compare(const StateElement & right) const {
    if (int rc = (int)(elem->id - right.elem->id))
        return rc;
    if (int rc = rep - right.rep)
        return rc;
    if (int rc = started - right.started)
        return rc;
    if (int rc = recurse - right.recurse)
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
template<typename T>
int compare(
    const vector<T> & a,
    const vector<T> & b
) {
    auto a1 = a.data();
    auto a2 = a1 + a.size();
    auto b1 = b.data();
    auto b2 = b1 + b.size();
    for (;; ++a1, ++b1) {
        if (a1 == a2)
            return b1 != b2 ? -1 : 0;
        if (b1 == b2)
            return 1;
        if (int rc = a1->compare(*b1))
            return rc;
    }
}


/****************************************************************************
*
*   StatePosition
*
***/

//===========================================================================
size_t std::hash<StatePosition>::operator()(const StatePosition & val) const {
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

//===========================================================================
int StatePosition::compare(const StatePosition & right) const {
    if (int rc = recurseSe - right.recurseSe)
        return rc;
    if (int rc = recursePos - right.recursePos)
        return rc;
    if (int rc = ::compare(elems, right.elems))
        return rc;
    if (int rc = ::compare(events, right.events))
        return rc;
    if (int rc = ::compare(delayedEvents, right.delayedEvents))
        return rc;
    return 0;
}

//===========================================================================
bool StatePosition::operator<(const StatePosition & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool StatePosition::operator==(const StatePosition & right) const {
    return compare(right) == 0;
}


/****************************************************************************
*
*   StateEvent
*
***/

//===========================================================================
size_t std::hash<StateEvent>::operator()(const StateEvent & val) const {
    size_t out = val.elem->id;
    hashCombine(out, val.flags);
    hashCombine(out, val.distance);
    return out;
}

//===========================================================================
int StateEvent::compare(const StateEvent & right) const {
    if (int rc = distance - right.distance)
        return rc;
    if (int rc = (int)(elem->id - right.elem->id))
        return rc;
    if (int rc = flags - right.flags)
        return rc;
    return 0;
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

//===========================================================================
size_t std::hash<State>::operator()(const State & val) const {
    size_t out = 0;
    for (auto && spt : val.positions) {
        hashCombine(out, hash<StatePosition>{}(spt.first));
    }
    return out;
}

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
    unsigned m_depth;
    unsigned m_depthLimit;
    string m_path;
    unsigned m_nextStateId;
    unsigned m_transitions;
};
} // namespace

static bool s_simpleRecursion = false;

static void addChildStates(
    State * st,
    unordered_set<State> & states,
    StateTreeInfo & sti
);

//===========================================================================
static size_t findRecursionSimple(const StatePosition * sp) {
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        auto first = sp->elems.data();
        auto m = first + depth / 2;
        auto z = first + depth;
        auto y = z - 2;
        for (; y->elem->type != Element::kRule; --y) {
            if (y == m)
                return 0;
        }
        y += 1;
        size_t yz = z - y;
        for (auto x = y - yz - 1; x > first; --x) {
            if (equal(x, x + yz, y, z)) {
                size_t num = x + yz - first;
                return num;
            }
        }
    }
    return 0;
}

//===========================================================================
static size_t findRecursionFull(const StatePosition * sp) {
    size_t depth = sp->elems.size();
    if (depth >= 3) {
        auto m = sp->elems.data() + depth / 2;
        auto z = &sp->elems.back();
        for (auto y = z - 1; y >= m; --y) {
            if (y->elem == z->elem) {
                auto x = y - (z - y);
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
static void addRulePositions(
    bool * skippable,
    State * st,
    StatePosition * sp,
    bool init
) {
    auto & se = sp->elems.back();
    const Element & elem = *se.elem;
    *skippable = false;
    if (elem.rule->flags & Element::fFunction) {
        se.recurse = true;
        // Don't generate states for right recursion when it can be broken with
        // a call. This could also be done for left recursion when the grammar
        // allows it, but that's more difficult to determine.
        if constexpr (true || !init) {
            auto & terms [[maybe_unused]] = st->positions[*sp];
            assert(terms.none());
            return;
        }
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
            // sp->elems.back().recurse = true;
            addPositions(skippable, st, sp, true, *elem.rule, 0);
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
    unsigned rep
) {
    *skippable = false;
    if (rule.type == Element::kTerminal) {
        assert(rule.m == 1 && rule.n == 1);
        auto & terms = st->positions[*sp];
        terms[(unsigned char)rule.value.front()] = true;
        return;
    }

    StateElement se;
    se.elem = &rule;
    se.rep = rep;
    sp->elems.push_back(se);
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto && elem : rule.elements) {
            bool weak;
            addPositions(&weak, st, sp, init, elem, 0);
            if (weak)
                *skippable = true;
        }
        break;
    case Element::kRule:
        addRulePositions(skippable, st, sp, init);
        break;
    case Element::kSequence:
        // up to first with a minimum
        *skippable = true;
        for (auto && elem : rule.elements) {
            bool weak;
            addPositions(&weak, st, sp, init, elem, 0);
            if (!weak) {
                *skippable = false;
                break;
            }
        }
        break;
    case Element::kTerminal:
        // already handled in if-test above
        assert(0);
    }

    if (rule.m == 0)
        *skippable = true;
    sp->elems.pop_back();
    if (sp->recurseSe && sp->recurseSe == sp->elems.size()) {
        sp->recurseSe = 0;
        sp->recursePos = -1;
    }
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
    Element::Flags flags
) {
    StateEvent sv;
    sv.elem = se.elem->rule;
    if (sv.elem->eventName.size())
        sv.elem = sv.elem->eventRule;
    sv.flags = flags;
    events.push_back(sv);
}

//===========================================================================
static void setPositionPrefix(
    StatePosition * sp,
    vector<StateElement>::const_iterator begin,
    vector<StateElement>::const_iterator end,
    int recursePos, // position within sequence
    bool recurseSeIncluded, // sequence state element included? else 1 past
                            //   the end, ignored if pos == -1
    const vector<StateEvent> & events,
    bool started
) {
    sp->elems.assign(begin, end);
    if (recursePos != -1) {
        sp->recursePos = recursePos;
        if (recurseSeIncluded) {
            sp->recurseSe = int(sp->elems.size() - 1);
            assert(sp->elems[sp->recurseSe].elem->type == Element::kSequence);
        } else {
            sp->recurseSe = int(sp->elems.size());
        }
    }
    sp->events = events;
    if (started) {
        for (auto && se : sp->elems) {
            if (se.elem->type == Element::kRule)
                se.started = true;
        }
    }
}

//===========================================================================
static void addDelayedEvents(
    vector<StateEvent> & events,
    const StatePosition & sp
) {
    for (auto && sv : sp.delayedEvents) {
        events.push_back(sv);
        if (sv.flags
            & (Element::fOnStartW | Element::fOnEndW | Element::fOnCharW)
        ) {
            events.back().distance += 1;
        }
    }
}

//===========================================================================
static void addNextPositions(
    State * st,
    const StatePosition & sp,
    const bitset<256> & chars
) {
    bool terminal = chars.any();

    if constexpr (false && sp.recurseSe) {
        assert(terminal);
        assert(sp.recurseSe < sp.elems.size());
        StatePosition nsp;
        setPositionPrefix(
            &nsp,
            sp.elems.begin(),
            sp.elems.begin() + sp.recurseSe,
            -1,
            false,
            sp.delayedEvents,
            false
        );
        bool weak;
        auto & e = *sp.elems[sp.recurseSe].elem;
        assert(e.type == Element::kSequence);
        assert(sp.recursePos < e.elements.size());
        addPositions(&weak, st, &nsp, false, e.elements[sp.recursePos], 0);
    }

    auto terminalStarted = sp.elems.end();
    auto done = sp.elems.front().elem->value == kDoneRuleName;
    vector<StateEvent> events;
    auto it = sp.elems.rbegin();
    auto eit = sp.elems.rend();

    if (terminal || done && sp.elems.size() == 1) {
        addDelayedEvents(events, sp);
    }
    if (terminal && !done) {
        // add OnStart events top to bottom
        for (auto && se2 : sp.elems) {
            if (se2.elem->type == Element::kRule && !se2.started) {
                auto flags = se2.elem->rule->flags & Element::fStartEvents;
                if (flags)
                    addEvent(events, se2, flags);
            }
        }
        // bubble up OnChar events bottom to top
        for (auto it2 = it; it2 != eit; ++it2) {
            if (it2->elem->type == Element::kRule) {
                auto flags = it2->elem->rule->flags & Element::fCharEvents;
                if (flags)
                    addEvent(events, *it2, flags);
            }
        }
    }

    for (; it != eit; ++it) {
        const StateElement & se = *it;
        int recursePos{-1};

        // advance to next in sequence
        if (se.elem->type == Element::kSequence) {
            auto & sePrev = it[-1];
            const Element * cur = sePrev.elem;
            const Element * last = &se.elem->elements.back();

            if (cur->type == Element::kRule && sePrev.recurse)
                recursePos = int(cur - se.elem->elements.data());

            if (cur != last) {
                StatePosition nsp;
                setPositionPrefix(
                    &nsp,
                    sp.elems.begin(),
                    it.base(),
                    recursePos,
                    true,
                    events,
                    terminal
                );
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
            if ((se.started || !done)
                && (~se.elem->rule->flags & Element::fFunction)
            ) {
                auto flags = se.elem->rule->flags & Element::fEndEvents;
                if (flags)
                    addEvent(events, se, flags);
            }
            // when exiting the parser (via a done sentinel) go directly out,
            // no not pass go, do not honor repetitions
            if (done)
                continue;
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
                &nsp,
                sp.elems.begin(),
                se_end,
                recursePos,
                false,
                events,
                terminal
            );
            bool weak;
            addPositions(&weak, st, &nsp, false, *se.elem, rep);
        }
        // don't advance unless rep >= m
        if (rep < m)
            return;

        // advance parent
    }

    //-----------------------------------------------------------------------
    // no more states

    StatePosition nsp;
    nsp.events = move(events);
    StateElement nse;
    nse.elem = done ? &ElementDone::s_abort : &ElementDone::s_consume;
    nse.rep = 0;
    nsp.elems.push_back(nse);
    if (done) {
        // add entry for completed done
        nsp.events.clear();
        addDelayedEvents(nsp.events, sp);
        st->positions[nsp];
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

    bool weak;
    addPositions(&weak, st, &nsp, false, ElementNull::s_elem, 0);
}

//===========================================================================
// when positions only vary by the set of events, remove all but the
// first - which should also be the one with the simplest events.
static void removePositionsWithMoreEvents(State & st) {
    if (st.positions.size() < 2)
        return;
    auto x = st.positions.begin();
    auto y = next(x);
    auto last = st.positions.end();
    while (y != last) {
        auto n = next(y);
        if (x->first.recurseSe == y->first.recurseSe
            && x->first.recursePos == y->first.recursePos
            && x->first.elems == y->first.elems
            && x->second == y->second
        ) {
            st.positions.erase(y);
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
        if (mi->flags & Element::fCharEvents) {
            // char can shift around start events
            for (;;) {
                if ((events[evi].flags & Element::fStartEvents) == 0
                    || ++evi == numEvents
                ) {
                    goto unmatched;
                }
                if (*mi == events[evi])
                    goto matched;
            }
        } else if (mi->flags & Element::fStartEvents) {
            // start events can shift around char events.
            for (;;) {
                if ((events[evi].flags & Element::fCharEvents) == 0
                    || ++evi == numEvents
                ) {
                    goto unmatched;
                }
                if (*mi == events[evi])
                    goto matched;
            }
        }
        assert(mi->flags & Element::fEndEvents);
        // [[fallthrough]];

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
    bool logError
) {
    for (auto && sv : matched) {
        auto svi = nsp.events.begin();
        auto e = nsp.events.end();
        for (; svi != e; ++svi) {
            if (sv == *svi) {
                nsp.events.erase(svi);
                break;
            }
        }
    }
    nsp.delayedEvents = move(nsp.events);
    nsp.events = matched;
    return true;
}

//===========================================================================
static bool resolveEventConflicts(State & st, const StateTreeInfo & sti) {
    if (st.positions.size() == 1)
        return true;

    // allow Char to match by moving forward in list
    vector<StateEvent> matched;
    auto b = st.positions.begin();
    auto e = st.positions.end();
    matched = b->first.events;
    size_t most = matched.size();
    for (++b; b != e; ++b) {
        auto & evts = b->first.events;
        most = max({evts.size(), most});
        removeConflicts(matched, evts);
    }
    if (most == matched.size())
        return true;

    bool success = true;
    map<StatePosition, bitset<256>> next;
    for (auto && spt : st.positions) {
        if (spt.first.events.size() == matched.size()) {
            next.insert(next.end(), move(spt));
            continue;
        }
        StatePosition nsp = move(spt.first);
        if (!delayConflicts(nsp, matched, sti, success))
            success = false;
        next.insert(next.end(), make_pair(move(nsp), move(spt.second)));
    }
    st.positions = move(next);
    return success;
}

//===========================================================================
static void appendHexByte(string & out, unsigned byte) {
    out += hexFromNibble(byte / 16);
    out += hexFromNibble(byte % 16);
}

//===========================================================================
static void appendPathChar(string & out, unsigned i) {
    if (i < ' ' || i == '^' || i >= 127) {
        out.push_back('^');
        if (i < ' ') {
            out.push_back((char)i + '@');
        } else if (i == 256) {
            out.push_back('*');
        } else {
            out.push_back('x');
            appendHexByte(out, i);
        }
    } else {
        out.push_back(unsigned char(i));
    }
}

//===========================================================================
static unsigned addState(
    State * st,
    unordered_set<State> & states,
    StateTreeInfo & sti,
    unsigned i
) {
    if (st->positions.empty())
        return 0;

    size_t pathLen = sti.m_path.size();
    appendPathChar(sti.m_path, i);
    sti.m_depth += 1;

    bool errors = !resolveEventConflicts(*st, sti);
    removePositionsWithMoreEvents(*st);

    // add state and, if it's new, its child states
    auto ib = states.insert(move(*st));
    State * st2 = const_cast<State *>(&*ib.first);

    ++sti.m_transitions;
    if (ib.second) {
        // it's a new state
        st2->id = ++sti.m_nextStateId;
        st2->name = sti.m_path;
        const char * show = sti.m_path.data();
        if (sti.m_path.size() > 40) {
            show += sti.m_path.size() - 40;
        }
        logMsgDebug() << sti.m_nextStateId << " states, " << sti.m_transitions
                      << " trans, " << sti.m_depth << " depth, "
                      << st2->positions.size() << " exits, " << kLeftQ << show
                      << kRightQ;
        if (!errors && (!sti.m_depthLimit || sti.m_depth < sti.m_depthLimit))
            addChildStates(st2, states, sti);
    }

    sti.m_depth -= 1;
    sti.m_path.resize(pathLen);
    return st2->id;
}

//===========================================================================
static void addChildStates(
    State * st,
    unordered_set<State> & states,
    StateTreeInfo & sti
) {
    st->next.assign(257, 0);
    State next;
    bool errors{false};

    // process all terminal combinations in order of lowest character value
    bitset<256> avail;
    for (auto && spt : st->positions)
        avail |= spt.second;

    for (unsigned i = 0; i < 256; ++i) {
        if (!avail.test(i))
            continue;

        bitset<256> include;
        bitset<256> exclude = ~avail;
        bool found{false};
        for (auto && spt : st->positions) {
            if (!spt.second.test(i)) {
                exclude |= spt.second;
            } else if (!found) {
                include = spt.second;
                found = true;
            } else {
                include &= spt.second;
            }
        }
        if (!include.test(i))
            continue;
        include &= ~exclude;

        next.clear();
        for (auto && spt : st->positions) {
            if ((spt.second & include).any())
                addNextPositions(&next, spt.first, include);
        }
        unsigned id = addState(&next, states, sti, i);
        for (unsigned i = 0; i < include.size(); ++i) {
            if (include[i])
                st->next[i] = id;
        }
        avail &= ~include;
    }

    next.clear();
    for (auto && spt : st->positions) {
        if (spt.second.any())
            continue;
        auto elem = spt.first.elems.back().elem;
        assert(elem->type == Element::kRule);

        if (elem->value == kDoneRuleName) {
            st->next[0] = 1;
            continue;
        }

        assert(elem->rule->flags & Element::fFunction);
        for (auto && nspt : next.positions) {
            if (nspt.second.none()) {
                auto && nse = nspt.first.elems.back();
                assert(nse.elem->type == Element::kRule);
                if ((elem->rule != nse.elem->rule
                     || spt.first.events != nspt.first.events)) {
                    errors = true;
                    logMsgError() << "Multiple recursive targets, " << kLeftQ
                                  << sti.m_path << kRightQ;
                    break;
                }
            }
        }
        bitset<256> chars;
        addNextPositions(&next, spt.first, chars);
    }
    if (!errors)
        st->next[256] = addState(&next, states, sti, 256);

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
    bool dedupStates,
    unsigned depthLimit
) {
    logMsgInfo() << "rule: " << root.name;

    states->clear();
    State state;
    StateTreeInfo sti;
    sti.m_depthLimit = depthLimit;
    sti.m_depth = 0;
    sti.m_nextStateId = 0;
    sti.m_transitions = 0;
    state.id = ++sti.m_nextStateId;
    state.name = kDoneStateName;
    StatePosition nsp;
    StateElement nse;
    nse.elem = &ElementDone::s_consume;
    nse.rep = 0;
    nsp.elems.push_back(nse);
    state.positions[nsp];
    states->insert(move(state));

    state.clear();
    if (inclDeps) {
        initPositions(&state, root);
        state.id = ++sti.m_nextStateId;
        auto ib = states->insert(move(state));
        addChildStates(const_cast<State *>(&*ib.first), *states, sti);
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
    bool next[257];

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
    vector<StateInfo> info;
    unordered_map<StateKey, vector<unsigned>> idByKey;

    unsigned lastMapId{0};
    vector<unsigned> pmapped;
    vector<unsigned> pmap;
    vector<unsigned> qmapped;
    vector<unsigned> qmap;
};

} // namespace


//===========================================================================
// StateKey
//===========================================================================

//===========================================================================
size_t std::hash<StateKey>::operator()(const StateKey & val) const {
    size_t out = 0;
    for (auto && sv : val.events) {
        hashCombine(out, hash<StateEvent>{}(sv));
    }
    hashCombine(out, hashBytes(val.next, sizeof(val.next)));
    return out;
}

//===========================================================================
bool StateKey::operator==(const StateKey & right) const {
    return events == right.events
        && memcmp(next, right.next, sizeof(next)) == 0;
}

//===========================================================================
bool StateKey::operator!=(const StateKey & right) const {
    return !operator==(right);
}

//===========================================================================
static void copy(StateKey & out, const State & st) {
    out.events = st.positions.begin()->first.events;
    if (st.next.empty()) {
        memset(out.next, 0, sizeof(out.next));
        return;
    }
    assert(st.next.size() == 257);
    for (unsigned i = 0; i < 257; ++i)
        out.next[i] = st.next[i];
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

    logMsgDebug() << di.states->size() << " states, merging state " << srcId
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
    di.info[srcId] = {};
}

//===========================================================================
static bool equalize(unsigned a, unsigned b, DedupInfo & di) {
    StateInfo & x = di.info[a];
    StateInfo & y = di.info[b];
    if (x.key.events != y.key.events)
        return false;
    size_t n = x.state->next.size();
    if (n != y.state->next.size())
        return false;
    auto xn = x.state->next.data();
    auto yn = y.state->next.data();
    for (unsigned i = 0; i < n; ++i) {
        unsigned p = xn[i];
        unsigned q = yn[i];
        if (p == q)
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
        di.pmapped.push_back(p);
        di.qmap[q] = di.lastMapId;
        di.qmapped.push_back(q);
        if (!equalize(p, q, di))
            return false;
    }
    return true;
}

//===========================================================================
// Makes one pass through all the equal keys and returns true if any of the
// associated states were equivalent (i.e. got it's references merge with
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
                for (auto && id : di.pmapped)
                    di.pmap[id] = 0;
                di.pmapped.clear();
                for (auto && id : di.qmapped)
                    di.qmap[id] = 0;
                di.qmapped.clear();
                di.pmap[x] = ++di.lastMapId;
                di.pmapped.push_back(x);
                di.qmap[y] = di.lastMapId;
                di.qmapped.push_back(y);
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
    // to hash functions and makes sure that we never remove a state that
    // could be the target of a merge from a subsequent state in the same
    // pass.
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
    auto numIds = max_element(
        states.begin(),
        states.end(),
        [](auto & a, auto & b){ return a.id < b.id; }
    )->id + 1;

    di.info.resize(numIds);
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
    di.pmap.assign(numIds, 0);
    di.qmap.assign(numIds, 0);

    // keep making dedup passes through all the keys until no more dups are
    // found.
    for (unsigned i = 1;; ++i) {
        logMsgInfo() << "dedup pass #" << i;
        if (!dedupStateTreePass(di))
            break;
    }
    logMsgInfo() << states.size() << " unique states";
}
