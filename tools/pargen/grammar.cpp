// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// grammar.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


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
*   Grammar
*
***/

//===========================================================================
void Grammar::clear() {
    m_nextElemId = 0;
    m_rules.clear();
    m_eventNames.clear();
    m_errpos = 0;
}

//===========================================================================
void Grammar::clearEvents() {
    m_eventNames.clear();
}

//===========================================================================
void Grammar::addOption(string_view name, string_view value) {
    auto e = addChoiceRule(name, 1, 1);
    e->type = Element::kRule;
    e->value = value;
}

//===========================================================================
void Grammar::setOption(string_view name, string_view value) {
    // replace value if option already exists
    if (auto elem = element(name)) {
        while (!elem->elements.empty())
            elem = &elem->elements.front();
        assert(elem->type == Element::kRule);
        elem->value = value;
        return;
    }

    // option doesn't exist, add it
    addOption(name, value);
}

//===========================================================================
const char *
Grammar::optionString(string_view name, const char * def) const {
    if (auto elem = element(name)) {
        while (!elem->elements.empty())
            elem = &elem->elements.front();
        assert(elem->type == Element::kRule);
        return elem->value.c_str();
    }
    return def;
}

//===========================================================================
unsigned Grammar::optionUnsigned(string_view name, unsigned def) const {
    if (auto value = optionString(name, nullptr)) {
        return (unsigned)strtoul(value, nullptr, 10);
    }
    return def;
}

//===========================================================================
const char * Grammar::operator[](string_view name) const {
    return optionString(name, "");
}

//===========================================================================
Element * Grammar::addSequenceRule(
    string_view name,
    unsigned m,
    unsigned n,
    unsigned flags // Element::kFlag*
    ) {
    auto e = addChoiceRule(name, m, n, flags);
    e = addSequence(e, 1, 1);
    return e;
}

//===========================================================================
Element * Grammar::addChoiceRule(
    string_view name,
    unsigned m,
    unsigned n,
    unsigned flags // Element::kFlag*
    ) {
    Element e;
    e.id = ++m_nextElemId;
    e.name = name;
    e.m = m;
    e.n = n;
    e.type = Element::kChoice;
    e.flags = flags;
    auto ib = m_rules.insert(e);
    assert(ib.second);
    return const_cast<Element *>(&*ib.first);
}

//===========================================================================
Element * Grammar::addElement(Element * rule, unsigned m, unsigned n) {
    rule->elements.resize(rule->elements.size() + 1);
    Element * e = &rule->elements.back();
    e->id = ++m_nextElemId;
    e->m = m;
    e->n = n;
    return e;
}

//===========================================================================
Element * Grammar::addSequence(Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kSequence;
    return e;
}

//===========================================================================
Element * Grammar::addChoice(Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kChoice;
    return e;
}

//===========================================================================
void Grammar::addRule(
    Element * rule,
    string_view name,
    unsigned m,
    unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kRule;
    e->value = name;
}

//===========================================================================
void Grammar::addTerminal(Element * rule, unsigned char ch) {
    assert(rule->type == Element::kChoice);
    auto e = addElement(rule, 1, 1);
    e->type = Element::kTerminal;
    e->value = ch;
}

//===========================================================================
void Grammar::addText(
    Element * rule,
    string_view value,
    unsigned m,
    unsigned n) {
    auto s = addSequence(rule, m, n);
    for (unsigned char ch : value) {
        auto c = addChoice(s, 1, 1);
        addTerminal(c, ch);
        if (islower(ch)) {
            addTerminal(c, (unsigned char)toupper(ch));
        } else if (isupper(ch)) {
            addTerminal(c, (unsigned char)tolower(ch));
        }
    }
}

//===========================================================================
void Grammar::addLiteral(
    Element * rule,
    string_view value,
    unsigned m,
    unsigned n) {
    auto s = addSequence(rule, m, n);
    for (unsigned char ch : value) {
        auto c = addChoice(s, 1, 1);
        addTerminal(c, ch);
    }
}

//===========================================================================
void Grammar::addRange(Element * rule, unsigned char a, unsigned char b) {
    assert(a <= b);
    for (unsigned i = a; i <= b; ++i) {
        addTerminal(rule, (unsigned char)i);
    }
}

//===========================================================================
const Element * Grammar::eventAlways(std::string_view name) {
    Element key;
    key.name = name;
    auto it = m_rules.find(key);
    if (it == m_rules.end())
        it = m_eventNames.insert(key).first;
    return &*it;
}

//===========================================================================
Element * Grammar::element(string_view name) {
    Element key;
    key.name = name;
    auto it = m_rules.find(key);
    if (it == m_rules.end())
        return nullptr;
    return const_cast<Element *>(&*it);
}

//===========================================================================
const Element * Grammar::element(string_view name) const {
    Element key;
    key.name = name;
    auto it = m_rules.find(key);
    if (it == m_rules.end())
        return nullptr;
    return &*it;
}


/****************************************************************************
*
*   process options
*
***/

//===========================================================================
static void
ensureOption(Grammar & rules, const char name[], string_view value) {
    if (!*rules.optionString(name))
        rules.addOption(name, value);
}

//===========================================================================
bool processOptions(Grammar & rules) {
    if (!*rules.optionString(kOptionRoot)) {
        logMsgError() << "Option not found, " << kOptionRoot;
        return false;
    }
    string prefix = rules.optionString(kOptionApiPrefix);
    if (prefix.empty()) {
        logMsgError() << "Option not found, " << kOptionApiPrefix;
        return false;
    }
    ensureOption(rules, kOptionApiParserClass, prefix + "Parser");
    ensureOption(rules, kOptionApiNotifyClass, "I" + prefix + "ParserNotify");
    auto & f = use_facet<ctype<char>>(locale());
    f.tolower(prefix.data(), prefix.data() + prefix.size());
    ensureOption(rules, kOptionApiHeaderFile, prefix + "parse.h");
    ensureOption(rules, kOptionApiCppFile, prefix + "parse.cpp");
    return true;
}


/****************************************************************************
*
*   copy rules
*
***/

//===========================================================================
static bool
copyRequiredDeps(Grammar & out, const Grammar & src, const Element & root) {
    switch (root.type) {
    case Element::kSequence:
    case Element::kChoice:
        for (auto && elem : root.elements)
            if (!copyRequiredDeps(out, src, elem))
                return false;
        break;
    case Element::kRule:
        if (!copyRules(out, src, root.value, false))
            return false;
    }
    return true;
}

//===========================================================================
bool copyRules(
    Grammar & out,
    const Grammar & src,
    string_view rootname,
    bool failIfExists) {
    auto root = src.element(rootname);
    if (!root) {
        logMsgError() << "Rule not found, " << rootname;
        return false;
    }
    auto & rules = const_cast<set<Element> &>(out.rules());
    auto ib = rules.insert(*root);
    if (!ib.second) {
        if (failIfExists) {
            logMsgError() << "Rule already exists, " << rootname;
            return false;
        }
        return true;
    }
    auto & elem = *ib.first;
    if ((elem.flags & Element::kFunction) && (elem.flags & Element::kOnChar)) {
        logMsgError() << "Rule with both function and onChar, " << elem.value;
        return false;
    }
    return copyRequiredDeps(out, src, elem);
}


/****************************************************************************
*
*   normalize
*
***/

//===========================================================================
static void normalizeChoice(Element & rule) {
    assert(
        rule.elements.size() > 1
        || rule.elements.size() == 1
            && rule.elements[0].type == Element::kTerminal);
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
    sort(rule.elements.begin(), rule.elements.end(), [](auto & a, auto & b) {
        return tie(a.name, a.type, a.value) < tie(b.name, b.type, b.value);
    });
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
            if (e.m != elem->m * e.m
                || e.n != max({elem->n, e.n, elem->n * e.n})) {
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
static void normalizeRule(Element & rule, Grammar & rules) {
    rule.rule = rules.element(rule.value);
}

//===========================================================================
static void
normalize(Element & rule, const Element * parent, Grammar & rules) {
    if (rule.elements.size() == 1) {
        Element & elem = rule.elements.front();
        if (elem.type != Element::kTerminal
            || parent && parent->type == Element::kChoice && rule.m == 1
                && rule.n == 1) {
            rule.m *= elem.m;
            rule.n = max({rule.n, elem.n, rule.n * elem.n});
            rule.type = elem.type;
            rule.value = elem.value;
            vector<Element> tmp{move(elem.elements)};
            rule.elements = move(tmp);
            return normalize(rule, parent, rules);
        }
    }

    if (rule.eventName.size())
        rule.eventRule = rules.eventAlways(rule.eventName);

    for (auto && elem : rule.elements)
        normalize(elem, &rule, rules);

    switch (rule.type) {
    case Element::kChoice: normalizeChoice(rule); break;
    case Element::kSequence: normalizeSequence(rule); break;
    case Element::kRule: normalizeRule(rule, rules); break;
    }
}

//===========================================================================
void normalize(Grammar & rules) {
    rules.clearEvents();
    for (auto && rule : rules.rules()) {
        normalize(const_cast<Element &>(rule), nullptr, rules);
    }
}


/****************************************************************************
*
*   mark recursion
*
***/

//===========================================================================
static void
addFunctionTags(Grammar & rules, Element & rule, vector<bool> & used) {
    bool wasUsed;
    switch (rule.type) {
    case Element::kChoice:
    case Element::kSequence:
        for (auto && elem : rule.elements) {
            addFunctionTags(rules, elem, used);
        }
        break;
    case Element::kRule:
        wasUsed = used[rule.rule->id];
        if (wasUsed && (~rule.rule->flags & Element::kOnChar))
            const_cast<Element *>(rule.rule)->flags |= Element::kFunction;
        if (rule.rule->flags & Element::kFunction)
            return;
        used[rule.rule->id] = true;
        addFunctionTags(rules, const_cast<Element &>(*rule.rule), used);
        used[rule.rule->id] = wasUsed;
    }
}

//===========================================================================
void functionTags(Grammar & rules, Element & rule, bool reset, bool mark) {
    if (reset || mark) {
        size_t maxId = 0;
        for (auto && elem : rules.rules()) {
            if (reset)
                const_cast<Element &>(elem).flags &= ~Element::kFunction;
            maxId = max((size_t)elem.id, maxId);
        }
        if (mark) {
            vector<bool> used(maxId, false);
            addFunctionTags(rules, rule, used);
        }
    }
}
