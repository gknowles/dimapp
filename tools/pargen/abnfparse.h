// abnfparse.h
// Generated by pargen 2.0.0 at 2017-08-06T10:06:22-0700
// clang-format off
#pragma once

#include "abnfparsebase.h"


/****************************************************************************
*
*   AbnfParser
*
***/

class AbnfParser : public AbnfParserBase {
public:
    using AbnfParserBase::AbnfParserBase;

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

private:
    bool stateGroupTail (const char *& src);

    // Events
    bool onActionAsEnd ();
    bool onActionCharEnd ();
    bool onActionCharwEnd ();
    bool onActionEndEnd ();
    bool onActionEndwEnd ();
    bool onActionFuncEnd ();
    bool onActionMinEnd ();
    bool onActionNoMinEnd ();
    bool onActionStartEnd ();
    bool onActionStartwEnd ();
    bool onAlternationStart ();
    bool onAlternationEnd ();
    bool onBinValAltFirstEnd ();
    bool onBinValAltSecondEnd ();
    bool onBinValConcatEachEnd ();
    bool onBinValConcatenationStart ();
    bool onBinValConcatenationEnd ();
    bool onBinValSequenceChar (char ch);
    bool onBinValSimpleEnd ();
    bool onCharValInsensitiveEnd ();
    bool onCharValSensitiveEnd ();
    bool onCharValSequenceStart ();
    bool onCharValSequenceChar (char ch);
    bool onConcatenationStart ();
    bool onConcatenationEnd ();
    bool onDecValAltFirstEnd ();
    bool onDecValAltSecondEnd ();
    bool onDecValConcatEachEnd ();
    bool onDecValConcatenationStart ();
    bool onDecValConcatenationEnd ();
    bool onDecValSequenceChar (char ch);
    bool onDecValSimpleEnd ();
    bool onDefinedAsIncrementalEnd ();
    bool onDefinedAsSetEnd ();
    bool onGroupStart (const char * ptr);
    bool onGroupEnd (const char * eptr);
    bool onHexValAltFirstEnd ();
    bool onHexValAltSecondEnd ();
    bool onHexValConcatEachEnd ();
    bool onHexValConcatenationStart ();
    bool onHexValConcatenationEnd ();
    bool onHexValSequenceChar (char ch);
    bool onHexValSimpleEnd ();
    bool onOptionQuotedChar (char ch);
    bool onOptionUnquotedChar (char ch);
    bool onOptiondefEnd ();
    bool onOptionlistStart ();
    bool onOptionlistEnd ();
    bool onOptionnameChar (char ch);
    bool onRepeatMaxEnd ();
    bool onRepeatMinmaxEnd ();
    bool onRepeatRangeStart ();
    bool onRepetitionStart ();
    bool onRuleEnd ();
    bool onRulenameStart ();
    bool onRulenameChar (char ch);
    bool onRulerefEnd ();

    // Data members
    size_t m_errpos{0};
};
