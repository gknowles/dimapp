// abnfparser.h - pargen
// clang-format off
#pragma once


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to make process parsed results
*
***/

class IAbnfParserNotify {
public:
    virtual ~IAbnfParserNotify () {}

    virtual bool startDoc () {}
    virtual bool endDoc () {}
};


/****************************************************************************
*
*   AbnfParser
*
***/

class AbnfParser {
public:
    AbnfParser (IAbnfParserNotify * notify) : m_notify(notify) {}
    ~AbnfParser () {}

    bool checkSyntax (const char src[]);

private:
    bool stateAlternation (const char *& src);

    IAbnfParserNotify * m_notify{nullptr};
};
