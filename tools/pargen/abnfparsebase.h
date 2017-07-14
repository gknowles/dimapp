// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// abnfparsebase.h - pargen

#include "intern.h"


/****************************************************************************
*
*   AbnfParserBase
*
***/

struct AbnfParserBase {
    AbnfParserBase(Grammar & rules, bool minRules);

    bool startRule();

    Grammar & m_rules;
    bool m_minRulesEnabled;
    std::vector<Element *> m_elems;
    std::string m_string;
    unsigned m_number{0};
    unsigned m_min{0};
    unsigned m_max{0};
    unsigned char m_first{0};

    // uncommitted info
    bool m_incremental{false};
    bool m_mustMatch{false};
    bool m_minRules{false};
    Element m_elem;
};

//===========================================================================
inline AbnfParserBase::AbnfParserBase(Grammar & rules, bool minRules)
    : m_rules{rules}
    , m_minRulesEnabled{minRules} 
{
    m_elem = {};
}

//===========================================================================
inline bool AbnfParserBase::startRule() {
    m_elem.name = std::move(m_string);
    m_elem.flags = {};
    m_elem.eventName.clear();
    m_elem.elements.clear();

    m_string.clear();
    assert(m_elems.empty());
    m_elems.push_back(&m_elem);

    m_mustMatch = false;
    return true;
}
