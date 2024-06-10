// xmlbaseparse.g.h
// Generated by pargen v2.2.1, DO NOT EDIT.
// clang-format off
#pragma once

#include "xmlbaseparsebase.h"

namespace Dim::Detail {


/****************************************************************************
*
*   XmlBaseParser
*
***/

class XmlBaseParser : public XmlBaseParserBase {
public:
    using XmlBaseParserBase::XmlBaseParserBase;

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

private:
    bool stateCp (const char *& src);
    bool stateElementTail (const char *& src);

    // Events
    bool onAttrCopyChar (char ch);
    bool onAttrInPlaceEnd (const char * eptr);
    bool onAttrNameStart (const char * ptr);
    bool onAttrNameEnd (const char * eptr);
    bool onAttrValueStart (const char * ptr);
    bool onAttrValueEnd ();
    bool onCDataWithEndStart (const char * ptr);
    bool onCDataWithEndEnd (const char * eptr);
    bool onCharDataChar (char ch);
    bool onCharRefStart ();
    bool onCharRefEnd ();
    bool onCharRefDigitChar (char ch);
    bool onCharRefHexdigChar (char ch);
    bool onElemNameStart (const char * ptr);
    bool onElemNameEnd (const char * eptr);
    bool onElemTextStart (const char * ptr);
    bool onElemTextEnd ();
    bool onElementEnd ();
    bool onEntityAmpEnd ();
    bool onEntityAposEnd ();
    bool onEntityGtEnd ();
    bool onEntityLtEnd ();
    bool onEntityOtherEnd (const char * eptr);
    bool onEntityQuotEnd ();
    bool onEntityValueStart (const char * ptr);
    bool onEntityValueEnd (const char * eptr);
    bool onNormalizableWsChar ();

    // Data members
    size_t m_errpos{0};
};

} // namespace
