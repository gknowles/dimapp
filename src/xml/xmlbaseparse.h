// xmlbaseparse.h
// Generated by pargen at 2016-12-17T07:54:34-0800
// clang-format off
#pragma once

namespace Dim::Detail {

// forward declarations
class IXmlBaseParserNotify;


/****************************************************************************
*
*   XmlBaseParser
*
***/

class XmlBaseParser {
public:
    XmlBaseParser (IXmlBaseParserNotify * notify) : m_notify(notify) {}
    ~XmlBaseParser () {}

    bool parse (const char src[]);
    size_t errpos () const { return m_errpos; }

    IXmlBaseParserNotify * notify () const { return m_notify; }

    // sets notify and returns its previous value
    IXmlBaseParserNotify * notify (IXmlBaseParserNotify * notify);

private:
    bool stateCp (const char *& src);
    bool stateElementTail (const char *& src);

    IXmlBaseParserNotify * m_notify{nullptr};
    size_t m_errpos{0};
};


/****************************************************************************
*
*   Parser event notifications
*   Clients inherit from this class to process parse events
*
***/

class IXmlBaseParserNotify {
public:
    virtual ~IXmlBaseParserNotify () {}

    virtual bool onStart () { return true; }
    virtual bool onEnd () { return true; }

    virtual bool onAttrCopyChar (char ch) { return true; }
    virtual bool onAttrInPlaceEnd (const char * eptr) { return true; }
    virtual bool onAttrNameStart (const char * ptr) { return true; }
    virtual bool onAttrNameEnd (const char * eptr) { return true; }
    virtual bool onAttrValueStart (const char * ptr) { return true; }
    virtual bool onAttrValueEnd (const char * eptr) { return true; }
    virtual bool onCDataWithEndStart (const char * ptr) { return true; }
    virtual bool onCDataWithEndEnd (const char * eptr) { return true; }
    virtual bool onCharDataChar (char ch) { return true; }
    virtual bool onCharRefStart (const char * ptr) { return true; }
    virtual bool onCharRefEnd (const char * eptr) { return true; }
    virtual bool onCharRefDigitChar (char ch) { return true; }
    virtual bool onCharRefHexdigChar (char ch) { return true; }
    virtual bool onElemNameStart (const char * ptr) { return true; }
    virtual bool onElemNameEnd (const char * eptr) { return true; }
    virtual bool onElemTextStart (const char * ptr) { return true; }
    virtual bool onElemTextEnd (const char * eptr) { return true; }
    virtual bool onElementEnd (const char * eptr) { return true; }
    virtual bool onEntityAmpEnd (const char * eptr) { return true; }
    virtual bool onEntityAposEnd (const char * eptr) { return true; }
    virtual bool onEntityGtEnd (const char * eptr) { return true; }
    virtual bool onEntityLtEnd (const char * eptr) { return true; }
    virtual bool onEntityOtherEnd (const char * eptr) { return true; }
    virtual bool onEntityQuotEnd (const char * eptr) { return true; }
    virtual bool onEntityValueStart (const char * ptr) { return true; }
    virtual bool onEntityValueEnd (const char * eptr) { return true; }
    virtual bool onNormalizableWsChar (char ch) { return true; }
};

} // namespace
