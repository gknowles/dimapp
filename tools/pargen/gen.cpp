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

namespace {

struct StateElement {
    const char * rule;
    const Element * elem;
    unsigned rep;

    int compare (const StateElement & right) const {
        if (int rc = (int) (elem->id - right.elem->id))
            return rc;
        if (rule) {
            if (right.rule) {
                if (int rc = strcmp(rule, right.rule))
                    return rc;
            } else {
                return -1;
            }
        } else {
            if (right.rule)
                return 1;
        }
        int rc = rep - right.rep;
        return rc;
    }
    bool operator< (const StateElement & right) const {
        return compare(right) < 0;
    }
    bool operator== (const StateElement & right) const {
        return compare(right) == 0;
    }
};

struct State {
    unsigned id;
    vector<StateElement> positions;
    vector<unsigned> next;

    void clear () {
        id = 0;
        positions.clear();
        next.clear();
    }

    int compare (const State & right) const {
        if (int rc = (int) (positions.size() - right.positions.size()))
            return rc;
        auto ptr = right.positions.data();
        for (auto&& se : positions) {
            if (int rc = se.compare(*ptr))
                return rc;
            ++ptr;
        }
        return 0;
    }
    bool operator== (const State & right) const {
        return compare(right) == 0;
    }
};

} // namespace


namespace std {
template<> struct hash<StateElement> {
    size_t operator() (const StateElement & val) const {
        size_t out = val.elem->id;
        out = (out << 1) ^ val.rep;
        return out;
    }
};

template<> struct hash<State> {
    size_t operator() (const State & val) const {
        size_t out = 0;
        for (auto&& elem : val.positions) {
            out = (out << 1) ^ hash<StateElement>{}(elem);
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
    const Element & rule, 
    const char name[]
) {
    StateElement se;
    switch (rule.type) {
    case Element::kChoice:
        // all
        for (auto&& elem : rule.elements) {
            addPositions(st, elem, name);
        }
        break;
    case Element::kRule:
        addPositions(st, *rule.rule, rule.name.c_str());
        break;
    case Element::kSequence:
        // up to first with a minimum
        for (auto&& elem : rule.elements) {
            addPositions(st, elem, name);
            if (elem.m)
                return;
        }
        break;
    case Element::kTerminal:
        se.rule = name;
        se.elem = &rule;
        se.rep = 0;
        st->positions.push_back(se);
        break;
    }
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
    addPositions(st, rule, rule.name.c_str());
    sort(st->positions.begin(), st->positions.end());
}

//===========================================================================
static void buildStateTree (State * st) {
    st->next.assign(256, 0);
    State next;
    for (unsigned i = 0; i < 256; ++i) {
        next.clear();
        for (auto&& se : st->positions) {
            assert(se.elem->type == Element::kTerminal);
            if ((unsigned char) se.elem->value.front() != i) 
                continue;
            StateElement se2;
            se2.rep = se.rep + 1;
            if (se.elem->n < se2.rep) {
                se2.rule = se.rule;
                se2.elem = se.elem;
                next.positions.push_back(se2);
                continue;
            }
            if (!se.elem->parent)
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
