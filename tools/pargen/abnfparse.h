// abnfparse.h
// Generated by pargen at 2016-12-19T03:00:29-0800
// clang-format off
#pragma once

// forward declarations
class IAbnfParserNotify;


/****************************************************************************
*
*   AbnfParser
*
***/

class AbnfParser {
public:
    AbnfParser (IAbnfParserNotify * notify) : m_notify(notify) {}
    ~AbnfParser () {}

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

    IAbnfParserNotify * notify () const { return m_notify; }

    // sets notify and returns its previous value
    IAbnfParserNotify * notify (IAbnfParserNotify * notify);

private:
    bool stateGroupTail (const char *& src);

    IAbnfParserNotify * m_notify{nullptr};
    size_t m_errpos{0};
};


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to process parse events
*
***/

class IAbnfParserNotify {
public:
    virtual ~IAbnfParserNotify () {}

    virtual bool onStart () { return true; }
    virtual bool onEnd () { return true; }

    virtual bool onActionAsEnd (const char * eptr) { return true; }
    virtual bool onActionCharEnd (const char * eptr) { return true; }
    virtual bool onActionEndEnd (const char * eptr) { return true; }
    virtual bool onActionFuncEnd (const char * eptr) { return true; }
    virtual bool onActionMinEnd (const char * eptr) { return true; }
    virtual bool onActionNoMinEnd (const char * eptr) { return true; }
    virtual bool onActionStartEnd (const char * eptr) { return true; }
    virtual bool onAlternationStart (const char * ptr) { return true; }
    virtual bool onAlternationEnd (const char * eptr) { return true; }
    virtual bool onBinValAltFirstEnd (const char * eptr) { return true; }
    virtual bool onBinValAltSecondEnd (const char * eptr) { return true; }
    virtual bool onBinValConcatEachEnd (const char * eptr) { return true; }
    virtual bool onBinValConcatenationStart (const char * ptr) { return true; }
    virtual bool onBinValConcatenationEnd (const char * eptr) { return true; }
    virtual bool onBinValSequenceChar (char ch) { return true; }
    virtual bool onBinValSimpleEnd (const char * eptr) { return true; }
    virtual bool onCharValInsensitiveEnd (const char * eptr) { return true; }
    virtual bool onCharValSensitiveEnd (const char * eptr) { return true; }
    virtual bool onCharValSequenceStart (const char * ptr) { return true; }
    virtual bool onCharValSequenceChar (char ch) { return true; }
    virtual bool onConcatenationStart (const char * ptr) { return true; }
    virtual bool onConcatenationEnd (const char * eptr) { return true; }
    virtual bool onDecValAltFirstEnd (const char * eptr) { return true; }
    virtual bool onDecValAltSecondEnd (const char * eptr) { return true; }
    virtual bool onDecValConcatEachEnd (const char * eptr) { return true; }
    virtual bool onDecValConcatenationStart (const char * ptr) { return true; }
    virtual bool onDecValConcatenationEnd (const char * eptr) { return true; }
    virtual bool onDecValSequenceChar (char ch) { return true; }
    virtual bool onDecValSimpleEnd (const char * eptr) { return true; }
    virtual bool onDefinedAsIncrementalEnd (const char * eptr) { return true; }
    virtual bool onDefinedAsSetEnd (const char * eptr) { return true; }
    virtual bool onGroupStart (const char * ptr) { return true; }
    virtual bool onGroupEnd (const char * eptr) { return true; }
    virtual bool onHexValAltFirstEnd (const char * eptr) { return true; }
    virtual bool onHexValAltSecondEnd (const char * eptr) { return true; }
    virtual bool onHexValConcatEachEnd (const char * eptr) { return true; }
    virtual bool onHexValConcatenationStart (const char * ptr) { return true; }
    virtual bool onHexValConcatenationEnd (const char * eptr) { return true; }
    virtual bool onHexValSequenceChar (char ch) { return true; }
    virtual bool onHexValSimpleEnd (const char * eptr) { return true; }
    virtual bool onOptionQuotedChar (char ch) { return true; }
    virtual bool onOptionUnquotedChar (char ch) { return true; }
    virtual bool onOptiondefEnd (const char * eptr) { return true; }
    virtual bool onOptionlistStart (const char * ptr) { return true; }
    virtual bool onOptionlistEnd (const char * eptr) { return true; }
    virtual bool onOptionnameChar (char ch) { return true; }
    virtual bool onRepeatMaxEnd (const char * eptr) { return true; }
    virtual bool onRepeatMinmaxEnd (const char * eptr) { return true; }
    virtual bool onRepeatRangeStart (const char * ptr) { return true; }
    virtual bool onRepetitionStart (const char * ptr) { return true; }
    virtual bool onRuleEnd (const char * eptr) { return true; }
    virtual bool onRulenameStart (const char * ptr) { return true; }
    virtual bool onRulenameChar (char ch) { return true; }
    virtual bool onRulerefEnd (const char * eptr) { return true; }
};
