// Copyright Glen Knowles 2016 - 2018.
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
inline bool AbnfParser::onActionAsEnd() {
    m_elem.eventName = std::move(m_string);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionCharEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnChar;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionCharwEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnCharW;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionEndEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnEnd;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionEndwEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnEndW;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionFuncEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fFunction;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionMinEnd() {
    m_mustMatch = true;
    m_minRules = true;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionNoMinEnd() {
    m_mustMatch = true;
    m_minRules = false;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionStartEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnStart;
    return true;
}

//===========================================================================
inline bool AbnfParser::onActionStartwEnd() {
    using namespace Dim;
    m_elem.flags |= Element::fOnStartW;
    return true;
}

//===========================================================================
inline bool AbnfParser::onAlternationStart() {
    Element * elem = m_rules.addChoice(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onAlternationEnd() {
    assert(m_elems.back()->type == Element::kChoice);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValAltFirstEnd() {
    assert(m_number <= UCHAR_MAX);
    m_first = (unsigned char)m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValAltSecondEnd() {
    assert(m_number <= UCHAR_MAX);
    Element * elem = m_rules.addChoice(m_elems.back(), m_min, m_max);
    m_rules.addRange(elem, m_first, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatEachEnd() {
    assert(m_number <= UCHAR_MAX);
    auto elem = m_rules.addChoice(m_elems.back(), 1, 1);
    m_rules.addTerminal(elem, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatenationStart() {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onBinValConcatenationEnd() {
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
inline bool AbnfParser::onBinValSimpleEnd() {
    assert(m_number <= UCHAR_MAX);
    auto elem = m_rules.addChoice(m_elems.back(), m_min, m_max);
    m_rules.addTerminal(elem, (unsigned char)m_number);
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValInsensitiveEnd() {
    m_rules.addText(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSensitiveEnd() {
    m_rules.addLiteral(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSequenceStart() {
    assert(m_string.empty());
    return true;
}

//===========================================================================
inline bool AbnfParser::onCharValSequenceChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
inline bool AbnfParser::onConcatenationStart() {
    Element * elem = m_rules.addSequence(m_elems.back(), 1, 1);
    m_elems.push_back(elem);
    return true;
}

//===========================================================================
inline bool AbnfParser::onConcatenationEnd() {
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onDecValAltFirstEnd() {
    return onBinValAltFirstEnd();
}

//===========================================================================
inline bool AbnfParser::onDecValAltSecondEnd() {
    return onBinValAltSecondEnd();
}

//===========================================================================
inline bool AbnfParser::onDecValConcatEachEnd() {
    return onBinValConcatEachEnd();
}

//===========================================================================
inline bool AbnfParser::onDecValConcatenationStart() {
    return onBinValConcatenationStart();
}

//===========================================================================
inline bool AbnfParser::onDecValConcatenationEnd() {
    return onBinValConcatenationEnd();
}

//===========================================================================
inline bool AbnfParser::onDecValSequenceChar(char ch) {
    m_number = 10 * m_number + (ch - '0');
    return true;
}

//===========================================================================
inline bool AbnfParser::onDecValSimpleEnd() {
    return onBinValSimpleEnd();
}

//===========================================================================
inline bool AbnfParser::onDefinedAsIncrementalEnd() {
    m_incremental = true;
    return startRule();
}

//===========================================================================
inline bool AbnfParser::onDefinedAsSetEnd() {
    return startRule();
}

//===========================================================================
inline bool AbnfParser::onGroupStart(char const * ptr) {
    Element * elem = m_rules.addSequence(m_elems.back(), m_min, m_max);
    m_elems.push_back(elem);
    if (*ptr == '[') {
        elem = m_rules.addSequence(elem, 0, 1);
        m_elems.push_back(elem);
    }
    return true;
}

//===========================================================================
inline bool AbnfParser::onGroupEnd(char const * eptr) {
    if (eptr[-1] == ']') {
        assert(m_elems.back()->type == Element::kSequence);
        m_elems.pop_back();
    }
    assert(m_elems.back()->type == Element::kSequence);
    m_elems.pop_back();
    return true;
}

//===========================================================================
inline bool AbnfParser::onHexValAltFirstEnd() {
    return onBinValAltFirstEnd();
}

//===========================================================================
inline bool AbnfParser::onHexValAltSecondEnd() {
    return onBinValAltSecondEnd();
}

//===========================================================================
inline bool AbnfParser::onHexValConcatEachEnd() {
    return onBinValConcatEachEnd();
}

//===========================================================================
inline bool AbnfParser::onHexValConcatenationStart() {
    return onBinValConcatenationStart();
}

//===========================================================================
inline bool AbnfParser::onHexValConcatenationEnd() {
    return onBinValConcatenationEnd();
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
inline bool AbnfParser::onHexValSimpleEnd() {
    return onBinValSimpleEnd();
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
inline bool AbnfParser::onOptiondefEnd() {
    return onRulerefEnd();
}

//===========================================================================
inline bool AbnfParser::onOptionlistStart() {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
inline bool AbnfParser::onOptionlistEnd() {
    return onRuleEnd();
}

//===========================================================================
inline bool AbnfParser::onOptionnameChar(char ch) {
    if (m_string.empty()) {
        m_string.push_back('%');
    }
    return onRulenameChar(ch);
}

//===========================================================================
inline bool AbnfParser::onRepeatMaxEnd() {
    m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepeatMinmaxEnd() {
    m_min = m_max = m_number;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepeatRangeStart() {
    m_min = m_number;
    m_max = kUnlimited;
    m_number = 0;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRepetitionStart() {
    m_min = 1;
    m_max = 1;
    return true;
}

//===========================================================================
inline bool AbnfParser::onRuleEnd() {
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
inline bool AbnfParser::onRulenameStart() {
    assert(m_string.empty());
    return true;
}

//===========================================================================
inline bool AbnfParser::onRulenameChar(char ch) {
    m_string.push_back(ch);
    return true;
}

//===========================================================================
inline bool AbnfParser::onRulerefEnd() {
    m_rules.addRule(m_elems.back(), m_string, m_min, m_max);
    m_string.clear();
    return true;
}
