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
static void normalize (Element & rule) {
    if (rule.elements.size() == 1) {
        Element & elem = rule.elements.front();
        rule.m *= elem.m;
        rule.n = max({rule.n, elem.n, rule.n * elem.n});
        rule.type = elem.type;
        rule.value = elem.value;
        vector<Element> tmp{move(elem.elements)};
        rule.elements = move(tmp);
        return normalize(rule);
    }
    
    for (auto&& elem : rule.elements)
        normalize(elem);

    switch (rule.type) {
    case Element::kChoice:
        normalizeChoice(rule);
        break;
    case Element::kSequence:
        normalizeSequence(rule);
        break;
    }
}

//===========================================================================
static void normalize (set<Element> & rules) {
    for (auto&& rule : rules) {
        normalize(const_cast<Element &>(rule));
    }
}

//===========================================================================
//static void compileRules (
//    set<Element> & rules,
//    const set<Element> & src,
//    const string & root
//) {
//    copyRules(rules, src, root, true);
//    normalize(rules);
//}

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
}
