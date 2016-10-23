// xmlbaseparse.h
// Generated by pargen at 2016-10-23T13:02:54-0700
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
    size_t errWhere () const { return m_errWhere; }

    IXmlBaseParserNotify * notify () const { return m_notify; }

    // sets notify and returns its previous value
    IXmlBaseParserNotify * notify (IXmlBaseParserNotify * notify);

private:
    bool stateContent (const char *& src);
    bool stateCp (const char *& src);

    IXmlBaseParserNotify * m_notify{nullptr};
    size_t m_errWhere{0};
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

    virtual bool onCDataWithEndChar (char ch) { return true; }
    virtual bool onCDataWithEndEnd (const char * eptr) { return true; }
    virtual bool onAttrContentEnd (const char * eptr) { return true; }
    virtual bool onAttrCopyChar (char ch) { return true; }
    virtual bool onAttrInPlaceStart (const char * ptr) { return true; }
    virtual bool onAttrInPlaceEnd (const char * eptr) { return true; }
    virtual bool onAttrNameStart (const char * ptr) { return true; }
    virtual bool onAttrNameEnd (const char * eptr) { return true; }
    virtual bool onCharDataCopyChar (char ch) { return true; }
    virtual bool onCharDataInPlaceEnd (const char * eptr) { return true; }
    virtual bool onContentStart (const char * ptr) { return true; }
    virtual bool onElemNameStart (const char * ptr) { return true; }
    virtual bool onElemNameEnd (const char * eptr) { return true; }
    virtual bool onElementEnd (const char * eptr) { return true; }
    virtual bool onEntityAmpEnd (const char * eptr) { return true; }
    virtual bool onEntityAposEnd (const char * eptr) { return true; }
    virtual bool onEntityGtEnd (const char * eptr) { return true; }
    virtual bool onEntityLtEnd (const char * eptr) { return true; }
    virtual bool onEntityOtherEnd (const char * eptr) { return true; }
    virtual bool onEntityQuotEnd (const char * eptr) { return true; }
};

} // namespace
