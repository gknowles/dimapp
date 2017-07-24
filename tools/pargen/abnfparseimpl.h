// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// abnfparseimpl.h - pargen

#include "core/core.h"

#include "abnfparse.h"


/****************************************************************************
*
*   AbnfParser
*
***/

//===========================================================================
inline bool AbnfParser::onActionAsEnd(const char * eptr) {
    m_elem.eventName = std::move(m_string);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionCharEnd(const char * eptr) {
    using namespace Dim;
    m_elem.flags |= Element::fOnChar;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionEndEnd(const char * eptr) {
    using namespace Dim;
    m_elem.flags |= Element::fOnEnd;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionFuncEnd(const char * eptr) {
    using namespace Dim;
    m_elem.flags |= Element::fFunction;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionMinEnd(const char * eptr) {
    m_mustMatch = true;
    m_minRules = true;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionNoMinEnd(const char * eptr) {
    m_mustMatch = true;
    m_minRules = false;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionStartEnd(const char * eptr) {
    using namespace Dim;
    m_elem.flags |= Element::fOnStart;
    return true;
}

//===========================================================================
inline bool AbnfParser::onAlternationStart(const char * ptr) {
    Element * elem = m_rules.addChoice(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onAlternationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kChoice);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValAltFirstEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    m_first = (unsigned char)m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValAltSecondEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    Element * elem = m_rules.addChoice(m_elems.back(), m_min, m_max);
    m_rules.addRange(elem, m_first, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatEachEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    auto elem = m_rules.addChoice(m_elems.back(), 1, 1);
    m_rules.addTerminal(elem, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatenationStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatenationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValSequenceChar(char ch) {
    m_number = 2 * m_number + (ch - '0');
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValSimpleEnd(const char * eptr) {
    assert(m_number <= UCHAR_MAX);
    auto elem = m_rules.addChoice(m_elems.back(), m_min, m_max);
    m_rules.addTerminal(elem, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValInsensitiveEnd(const char * eptr) {
    m_rules.addText(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSensitiveEnd(const char * eptr) {
    m_rules.addLiteral(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSequenceStart(const char * ptr) {
    assert(m_string.empty());
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSequenceChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
inline bool AbnfParser::onConcatenationStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onConcatenationEnd(const char * eptr) {
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onDecValAltFirstEnd(const char * eptr) {
    return onBinValAltFirstEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onDecValAltSecondEnd(const char * eptr) {
    return onBinValAltSecondEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onDecValConcatEachEnd(const char * eptr) {
    return onBinValConcatEachEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onDecValConcatenationStart(const char * ptr) {
    return onBinValConcatenationStart(ptr);
}

//===========================================================================
inline bool AbnfParser::onDecValConcatenationEnd(const char * eptr) {
    return onBinValConcatenationEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onDecValSequenceChar(char ch) {
    m_number = 10 * m_number + (ch - '0');
    return true;
}

//===========================================================================
inline bool AbnfParser::onDecValSimpleEnd(const char * eptr) {
    return onBinValSimpleEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onDefinedAsIncrementalEnd(const char * eptr) {
    m_incremental = true;
    return startRule();
}

//===========================================================================
inline bool AbnfParser::onDefinedAsSetEnd(const char * eptr) {
    return startRule();
}

//===========================================================================
inline bool AbnfParser::onGroupStart(const char * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    if (*ptr == '[') {
        elem = m_rules.addSequence(elem, 0, 1);
        m_elems.push_back(elem);
    }
    return true;
}

//===========================================================================
inline bool AbnfParser::onGroupEnd(const char * eptr) {
    if (eptr[-1] == ']') {
        assert(m_elems.back()->type == Element::kSequence);
        m_elems.pop_back();
    }
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onHexValAltFirstEnd(const char * eptr) {
    return onBinValAltFirstEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onHexValAltSecondEnd(const char * eptr) {
    return onBinValAltSecondEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onHexValConcatEachEnd(const char * eptr) {
    return onBinValConcatEachEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onHexValConcatenationStart(const char * ptr) {
    return onBinValConcatenationStart(ptr);
}

//===========================================================================
inline bool AbnfParser::onHexValConcatenationEnd(const char * eptr) {
    return onBinValConcatenationEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onHexValSequenceChar(char ch) {
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
inline bool AbnfParser::onHexValSimpleEnd(const char * eptr) {
    return onBinValSimpleEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onOptionQuotedChar(char ch) {
    return onRulenameChar(ch);
}

//===========================================================================
inline bool AbnfParser::onOptionUnquotedChar(char ch) {
    return onRulenameChar(ch);
}

//===========================================================================
inline bool AbnfParser::onOptiondefEnd(const char * eptr) {
    return onRulerefEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onOptionlistStart(const char * ptr) {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
inline bool AbnfParser::onOptionlistEnd(const char * eptr) {
    return onRuleEnd(eptr);
}

//===========================================================================
inline bool AbnfParser::onOptionnameChar(char ch) {
    if (m_string.empty()) {
        m_string.push_back('%');
    }
    return onRulenameChar(ch);
}

//===========================================================================
inline bool AbnfParser::onRepeatMaxEnd(const char * eptr) {
    m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepeatMinmaxEnd(const char * eptr) {
    m_min = m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepeatRangeStart(const char * ptr) {
    m_min = m_number;
    m_max = kUnlimited;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepetitionStart(const char * ptr) {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRuleEnd(const char * eptr) {
    using namespace Dim;
    if (m_elems.size() == 1) {
        if (!m_mustMatch || m_minRules == m_minRulesEnabled) {
            Element * elem = m_rules.element(m_elem.name);
            if (!elem) {
                elem = m_rules.addChoiceRule(m_elem.name, 1, 1);
            } else {
                if (m_incremental) {
                    m_incremental = false;
                } else {
                    return false;
                }
            }
            elem->flags |= m_elem.flags;
            if (m_elem.eventName.size())
                elem->eventName = m_elem.eventName;
            elem->elements.insert(
                elem->elements.end(),
                m_elem.elements.begin(),
                m_elem.elements.end());
        }
    }
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onRulenameStart(const char * ptr) {
    assert(m_string.empty());
    return true;
}

//===========================================================================
inline bool AbnfParser::onRulenameChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
inline bool AbnfParser::onRulerefEnd(const char * eptr) {
    m_rules.addRule(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}
