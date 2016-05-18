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

static void hashCombine (size_t & seed, size_t v);

namespace {

struct StateElement {
    const Element * elem;
    unsigned rep;

    int compare (const StateElement & right) const;
    bool operator< (const StateElement & right) const;
    bool operator== (const StateElement & right) const;
};

struct StatePosition {
    vector<StateElement> elems;

    bool operator< (const StatePosition & right) const;
    bool operator== (const StatePosition & right) const;
};

struct State {
    unsigned id;
    vector<StatePosition> positions;
    vector<unsigned> next;

    void clear () {
        id = 0;
        positions.clear();
        next.clear();
    }

    bool operator== (const State & right) const {
        return equal(
            positions.begin(), 
            positions.end(), 
            right.positions.begin(), 
            right.positions.end()
        );
    }
};

} // namespace

namespace std {
template<> struct hash<StateElement> {
    size_t operator() (const StateElement & val) const {
        size_t out = val.elem->id;
        hashCombine(out, val.rep);
        return out;
    }
};

template<> struct hash<StatePosition> {
    size_t operator() (const StatePosition & val) const {
        size_t out = 0;
        for (auto&& se : val.elems) {
            hashCombine(out, hash<StateElement>{}(se));
        }
        return out;
    }
};

template<> struct hash<State> {
    size_t operator() (const State & val) const {
        size_t out = 0;
        for (auto&& sp : val.positions) {
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

static unordered_set<State> s_states;
static unsigned s_nextStateId;


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static void copyRules (
    set<Element> & rules,
    const set<Element> & src,
    const string & root,
    bool failIfExists
);
static void normalizeSequence (Element & rule);

//===========================================================================
static void hashCombine (size_t & seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

//===========================================================================
static void copyRequiredDeps (
    set<Element> & rules,
    const set<Element> & src,
    const Element & root
) {
    switch (root.type) {
    case Element::kSequence:
    case Element::kChoice:
        for (auto&& elem : root.elements)
            copyRequiredDeps(rules, src, elem);
        break;
    case Element::kRule:
        copyRules(rules, src, root.value, false);
        break;
    }
}

//===========================================================================
static void copyRules (
    set<Element> & rules,
    const set<Element> & src,
    const string & root,
    bool failIfExists
) {
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
static void normalizeChoice (Element & rule) {
    vector<Element> tmp;
    for (auto&& elem : rule.elements) {
        if (elem.type != Element::kChoice)
            continue;
        for (auto&& e : elem.elements) {
            tmp.push_back(move(e));
            Element & back = tmp.back();
            back.m *= elem.m;
            back.n = max({back.n, elem.n, back.n * elem.n});
        }
        elem = move(tmp.back());
        tmp.pop_back();
    }
    for (auto&& e : tmp) {
        rule.elements.push_back(move(e));
    }
}

//===========================================================================
static void normalizeSequence (Element & rule) {
    // it's okay to elevate a sub-list if multiplying the ordinals 
    // doesn't change them?
    for (unsigned i = 0; i < rule.elements.size(); ++i) {
        Element & elem = rule.elements[i];
        if (elem.type != Element::kSequence)
            continue;
        bool skip = false;
        for (auto&& e : elem.elements) {
            if (e.m != elem.m * e.m
                || e.n != max({elem.n, e.n, elem.n * e.n})
            ) {
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
                elem.elements.end()
            );
        }
        Element tmp = move(elem.elements.front());
        elem = move(tmp);
        i -= 1;
    }
}

//===========================================================================
static void normalizeRule (Element & rule, const set<Element> & rules) {
    Element elem;
    elem.name = rule.value;
    rule.rule = &*rules.find(elem);
}

//===========================================================================
static void normalize (Element & rule, const set<Element> & rules) {
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
    
    for (auto&& elem : rule.elements)
        normalize(elem, rules);

    switch (rule.type) {
    case Element::kChoice:
        normalizeChoice(rule);
        break;
    case Element::kSequence:
        normalizeSequence(rule);
        break;
    case Element::kRule:
        normalizeRule(rule, rules);
        break;
    }

    for (auto&& elem : rule.elements)
        elem.parent = &rule;
}

//===========================================================================
static void normalize (set<Element> & rules) {
    for (auto&& rule : rules) {
        normalize(const_cast<Element &>(rule), rules);
    }
}

//===========================================================================
ostream & operator<< (ostream & os, const Element & elem) {
    if (elem.m != 1 || elem.n != 1) {
        if (elem.m) os << elem.m;
        os << '*';
        if (elem.n != kUnlimited) os << elem.n;
    }
    bool first = true;
    switch (elem.type) {
    case Element::kChoice:
        os << "( ";
        for (auto&& e : elem.elements) {
            if (first) {
                first = false;
            } else {
                os << " / ";
            }
            os << e;
        }
        os << " )";
        break;
    case Element::kRule:
        os << elem.value;
        break;
    case Element::kSequence:
        os << "( ";
        for (auto&& e : elem.elements) {
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
        os << "%x" << hex << (unsigned) elem.value[0];
        break;
    }
    return os;
}

//===========================================================================
ostream & operator<< (ostream & os, const set<Element> & rules) {
    for (auto&& rule : rules) {
        os << rule.name << " = " << rule << '\n';
    }
    return os;
}

//===========================================================================
static void addPositions (
    State * st, 
    StatePosition * sp,
    const Element & rule
) {
    StateElement se;
    se.elem = &rule;
    se.rep = 0;
    sp->elems.push_back(se);
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto&& elem : rule.elements) {
            addPositions(st, sp, elem);
        }
        break;
    case Element::kRule:
        addPositions(st, sp, *rule.rule);
        break;
    case Element::kSequence:
        // up to first with a minimum
        for (auto&& elem : rule.elements) {
            addPositions(st, sp, elem);
            if (elem.m)
                goto done;
        }
        break;
    case Element::kTerminal:
        st->positions.push_back(*sp);
        break;
    }
done:
    sp->elems.pop_back();
}

//===========================================================================
static void initPositions (
    State * st, 
    const set<Element> & rules,
    const string & root
) {
    Element key;
    key.name = root;
    const Element & rule = *rules.find(key);
    st->positions.clear();
    StatePosition sp;
    addPositions(st, &sp, rule);
    sort(st->positions.begin(), st->positions.end());
}

//===========================================================================
static void buildStateTree (State * st) {
    st->next.assign(256, 0);
    State next;
    for (unsigned i = 0; i < 256; ++i) {
        next.clear();
        for (auto&& sp : st->positions) {
            auto & se = sp.elems.back();
            assert(se.elem->type == Element::kTerminal);
            if ((unsigned char) se.elem->value.front() != i) 
                continue;
            if (se.elem->n < se.rep + 1) {
                next.positions.push_back(sp);
                if (se.elem->n == kUnlimited) {
                    next.positions.back().elems.back().rep = se.elem->m + 1;
                } else {
                    next.positions.back().elems.back().rep += 1;
                }
                continue;
            }
            if (sp.elems.size() == 1)
                continue;

        }
        if (next.positions.size()) {
            auto ib = s_states.insert(move(next));
            State * st2 = const_cast<State *>(&*ib.first);
            if (!ib.second) {
                st2->id = ++s_nextStateId;
            }
            st->next[i] = st2->id;
            if (!ib.second)
                buildStateTree(st2);
        }
    }
}


/****************************************************************************
*
*   StateElement and StatePosition
*
***/

//===========================================================================
int StateElement::compare (const StateElement & right) const {
    if (int rc = (int) (elem->id - right.elem->id))
        return rc;
    int rc = rep - right.rep;
    return rc;
}

//===========================================================================
bool StateElement::operator< (const StateElement & right) const {
    return compare(right) < 0;
}

//===========================================================================
bool StateElement::operator== (const StateElement & right) const {
    return compare(right) == 0;
}

//===========================================================================
bool StatePosition::operator< (const StatePosition & right) const {
    return elems < right.elems;
}

//===========================================================================
bool StatePosition::operator== (const StatePosition & right) const {
    return elems == right.elems;
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void writeParser (
    ostream & os, 
    const set<Element> & src,
    const string & root
) {
    set<Element> rules;
    copyRules(rules, src, root, true);
    normalize(rules);
    os << rules;

    State state;
    initPositions(&state, rules, root);
    cout << "Positions: " << state.positions.size() << endl;
    
    s_states.clear();
    s_nextStateId = 0;
    state.id = ++s_nextStateId;
    s_states.insert(move(state));
    buildStateTree(const_cast<State *>(&*s_states.begin()));
}
