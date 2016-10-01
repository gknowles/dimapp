// parse.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

class ParserNotify : public IAbnfParserNotify {
public:
    ParserNotify(Grammar & rules);
    virtual ~ParserNotify() {}

private:
    // IAbnfParserNotify
    bool onStart() final { return true; }
    bool onEnd() final { return true; }
    bool onActionAsEnd(const char * eptr) final;
    bool onActionCharEnd(const char * eptr) final;
    bool onActionEndEnd(const char * eptr) final;
    bool onActionFuncEnd(const char * eptr) final;
    bool onActionStartEnd(const char * eptr) final;
    bool onAlternationStart(const char * ptr) final;
    bool onAlternationEnd(const char * eptr) final;
    bool onBinValAltFirstEnd(const char * eptr) final;
    bool onBinValAltSecondEnd(const char * eptr) final;
    bool onBinValConcatEachEnd(const char * eptr) final;
    bool onBinValConcatenationStart(const char * ptr) final;
    bool onBinValConcatenationEnd(const char * eptr) final;
    bool onBinValSequenceChar(char ch) final;
    bool onBinValSimpleEnd(const char * eptr) final;
    bool onCharValEnd(const char * eptr) final;
    bool onCharValSequenceStart(const char * ptr) final;
    bool onCharValSequenceChar(char ch) final;
    bool onConcatenationStart(const char * ptr) final;
    bool onConcatenationEnd(const char * eptr) final;
    bool onDecValAltFirstEnd(const char * eptr) final;
    bool onDecValAltSecondEnd(const char * eptr) final;
    bool onDecValConcatEachEnd(const char * eptr) final;
    bool onDecValConcatenationStart(const char * ptr) final;
    bool onDecValConcatenationEnd(const char * eptr) final;
    bool onDecValSequenceChar(char ch) final;
    bool onDecValSimpleEnd(const char * eptr) final;
    bool onDefinedAsIncrementalEnd(const char * eptr) final;
    bool onDefinedAsSetEnd(const char * eptr) final;
    bool onGroupStart(const char * ptr) final;
    bool onGroupEnd(const char * eptr) final;
    bool onHexValAltFirstEnd(const char * eptr) final;
    bool onHexValAltSecondEnd(const char * eptr) final;
    bool onHexValConcatEachEnd(const char * eptr) final;
    bool onHexValConcatenationStart(const char * ptr) final;
    bool onHexValConcatenationEnd(const char * eptr) final;
    bool onHexValSequenceChar(char ch) final;
    bool onHexValSimpleEnd(const char * eptr) final;
    bool onOptionQuotedChar(char ch) final;
    bool onOptionUnquotedChar(char ch) final;
    bool onOptiondefEnd(const char * eptr) final;
    bool onOptionlistStart(const char * ptr) final;
    bool onOptionlistEnd(const char * eptr) final;
    bool onOptionnameChar(char ch) final;
    bool onRepeatMaxEnd(const char * eptr) final;
    bool onRepeatMinmaxEnd(const char * eptr) final;
    bool onRepeatRangeStart(const char * ptr) final;
    bool onRepetitionStart(const char * ptr) final;
    bool onRuleEnd(const char * eptr) final;
    bool onRulenameStart(const char * ptr) final;
    bool onRulenameChar(char ch) final;
    bool onRulerefEnd(const char * eptr) final;

    Grammar & m_rules;
    vector<Element *> m_elems;
    string m_string;
    unsigned m_number{0};
    unsigned m_min{0};
    unsigned m_max{0};
    unsigned char m_first{0};
};


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   Helpers
*
***/


/****************************************************************************
*
*   ParserNotify
*
***/

//===========================================================================
ParserNotify::ParserNotify(Grammar & rules)
    : m_rules(rules) {}

//===========================================================================
// IAbnfParserNotify
//===========================================================================
bool ParserNotify::onActionAsEnd(const char * eptr) {
    m_elems.back()->eventName = move(m_string);
    m_string.clear();
    return true;
}

//===========================================================================
bool ParserNotify::onActionCharEnd(const char * eptr) {
    m_elems.back()->flags |= Element::kOnChar;
    return true;
}

//===========================================================================
bool ParserNotify::onActionEndEnd(const char * eptr) {
    m_elems.back()->flags |= Element::kOnEnd;
    return true;
}

//===========================================================================
bool ParserNotify::onActionFuncEnd(const char * eptr) {
    m_elems.back()->recurse = true;
    return true;
}

//===========================================================================
bool ParserNotify::onActionStartEnd(const char * eptr) {
    m_elems.back()->flags |= Element::kOnStart;
    return true;
}

//===========================================================================
bool ParserNotify::onAlternationStart(const char * ptr) {
    Element * elem = m_rules.addChoice(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
bool ParserNotify::onAlternationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kChoice);
    m_elems.pop_back();
    return true;
}

//===========================================================================
bool ParserNotify::onBinValAltFirstEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    m_first = (unsigned char)m_number;
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onBinValAltSecondEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    Element * elem = m_rules.addChoice(m_elems.back(), m_min, m_max);
    m_rules.addRange(elem, m_first, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onBinValConcatEachEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    m_rules.addTerminal(m_elems.back(), (unsigned char)m_number, 1, 1);
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onBinValConcatenationStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
bool ParserNotify::onBinValConcatenationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
bool ParserNotify::onBinValSequenceChar(char ch) {
    m_number = 2 * m_number + (ch - '0');
    return true;
}

//===========================================================================
bool ParserNotify::onBinValSimpleEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    m_rules.addTerminal(m_elems.back(), (unsigned char)m_number, m_min, m_max);
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onCharValEnd(const char * eptr) {
    m_rules.addLiteral(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}

//===========================================================================
bool ParserNotify::onCharValSequenceStart(const char * ptr) {
    assert(m_string.empty());
    return true;
}

//===========================================================================
bool ParserNotify::onCharValSequenceChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
bool ParserNotify::onConcatenationStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
bool ParserNotify::onConcatenationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
bool ParserNotify::onDecValAltFirstEnd(const char * eptr) {
    return onBinValAltFirstEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDecValAltSecondEnd(const char * eptr) {
    return onBinValAltSecondEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDecValConcatEachEnd(const char * eptr) {
    return onBinValConcatEachEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDecValConcatenationStart(const char * ptr) {
    return onBinValConcatenationStart(ptr);
}

//===========================================================================
bool ParserNotify::onDecValConcatenationEnd(const char * eptr) {
    return onBinValConcatenationEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDecValSequenceChar(char ch) {
    m_number = 10 * m_number + (ch - '0');
    return true;
}

//===========================================================================
bool ParserNotify::onDecValSimpleEnd(const char * eptr) {
    return onBinValSimpleEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDefinedAsIncrementalEnd(const char * eptr) {
    if (auto rule = m_rules.element(m_string)) {
        m_string.clear();
        assert(m_elems.empty());
        m_elems.push_back(rule);
        return true;
    }
    return onDefinedAsSetEnd(eptr);
}

//===========================================================================
bool ParserNotify::onDefinedAsSetEnd(const char * eptr) {
    Element * rule = m_rules.addChoiceRule(m_string, 1, 1);
    m_string.clear();
    assert(m_elems.empty());
    m_elems.push_back(rule);
    return true;
}

//===========================================================================
bool ParserNotify::onGroupStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    if (*ptr == '[') {
        elem = m_rules.addSequence(elem, 0, 1);
        m_elems.push_back(elem);
    }
    return true;
}

//===========================================================================
bool ParserNotify::onGroupEnd(const char * eptr) {
    if (eptr[-1] == ']') {
        assert(m_elems.back()->type == Element::kSequence);
        m_elems.pop_back();
    }
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
bool ParserNotify::onHexValAltFirstEnd(const char * eptr) {
    return onBinValAltFirstEnd(eptr);
}

//===========================================================================
bool ParserNotify::onHexValAltSecondEnd(const char * eptr) {
    return onBinValAltSecondEnd(eptr);
}

//===========================================================================
bool ParserNotify::onHexValConcatEachEnd(const char * eptr) {
    return onBinValConcatEachEnd(eptr);
}

//===========================================================================
bool ParserNotify::onHexValConcatenationStart(const char * ptr) {
    return onBinValConcatenationStart(ptr);
}

//===========================================================================
bool ParserNotify::onHexValConcatenationEnd(const char * eptr) {
    return onBinValConcatenationEnd(eptr);
}

//===========================================================================
bool ParserNotify::onHexValSequenceChar(char ch) {
    if (ch <= '9') {
        m_number = 16 * m_number + ch - '0';
    } else if (ch <= 'Z') {
        m_number = 16 * m_number + ch - 'A' + 10;
    } else {
        m_number = 16 * m_number + ch - 'a' + 10;
    }
    return true;
}

//===========================================================================
bool ParserNotify::onHexValSimpleEnd(const char * eptr) {
    return onBinValSimpleEnd(eptr);
}

//===========================================================================
bool ParserNotify::onOptionQuotedChar(char ch) {
    return onRulenameChar(ch);
}

//===========================================================================
bool ParserNotify::onOptionUnquotedChar(char ch) {
    return onRulenameChar(ch);
}

//===========================================================================
bool ParserNotify::onOptiondefEnd(const char * eptr) {
    return onRulerefEnd(eptr);
}

//===========================================================================
bool ParserNotify::onOptionlistStart(const char * ptr) {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
bool ParserNotify::onOptionlistEnd(const char * eptr) {
    return onRuleEnd(eptr);
}

//===========================================================================
bool ParserNotify::onOptionnameChar(char ch) {
    if (m_string.empty()) {
        m_string.push_back('%');
    }
    return onRulenameChar(ch);
}

//===========================================================================
bool ParserNotify::onRepeatMaxEnd(const char * eptr) {
    m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onRepeatMinmaxEnd(const char * eptr) {
    m_min = m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onRepeatRangeStart(const char * ptr) {
    m_min = m_number;
    m_max = kUnlimited;
    m_number = 0;
    return true;
}

//===========================================================================
bool ParserNotify::onRepetitionStart(const char * ptr) {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
bool ParserNotify::onRuleEnd(const char * eptr) {
    m_elems.pop_back();
    return true;
}

//===========================================================================
bool ParserNotify::onRulenameStart(const char * ptr) {
    assert(m_string.empty());
    return true;
}

//===========================================================================
bool ParserNotify::onRulenameChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
bool ParserNotify::onRulerefEnd(const char * eptr) {
    m_rules.addRule(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
bool parseAbnf(Grammar & rules, const std::string & src) {
    ParserNotify notify(rules);
    AbnfParser parser(&notify);
    const char * ptr = src.c_str();
    if (!parser.parse(ptr))
        return false;
    normalize(rules);
    return true;
}
