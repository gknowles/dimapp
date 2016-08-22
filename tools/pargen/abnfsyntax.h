// abnfsyntax.h - pargen
// clang-format off
#pragma once


/****************************************************************************
*
*   AbnfSyntax
*
***/

class AbnfSyntax {
public:
    AbnfSyntax () {}
    ~AbnfSyntax () {}

    bool checkSyntax (const char src[]);

private:
    bool stateAlternation (const char *& src);
};
