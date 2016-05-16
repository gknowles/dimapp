// pargen.cpp - pargen
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Declarations
*
***/

static unsigned s_nextElemId;

//===========================================================================
static void getLineRules (set<Element> & rules) {
    // line = OWS *1(param-list) OWS
    auto * rule = addSequenceRule(rules, "line", 1, 1);
    addRule(rule, "ows", 1, 1);
    addRule(rule, "param-list", 0, 1);
    addRule(rule, "ows", 1, 1);

    // param-list = param *(ws param)
    rule = addSequenceRule(rules, "param-list", 1, 1);
    addRule(rule, "param", 1, 1);
    auto * elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "ws", 1, 1);
    addRule(elem, "param", 1, 1);

    // param = *fully-quoted [half-quoted] *BSLASH
    // fully-quoted = *1unquoted active-quote quoted active-quote
    // half-quoted = *1unquoted active-quote quoted
    // unquoted = 1*( ptext  / escaped-quote ) 
    // quoted = 1*( ptext / dquote-pair / escaped-quote / ws )
    // ptext = *(pchar / BSLASH) pchar
    // escaped-quote = *bslash-pair BSLASH DQUOTE
    // active-quote = *bslash-pair DQUOTE
    // dquote-pair = DQUOTE DQUOTE
    // bslash-pair = BSLASH BSLASH
    // pchar = %x21 / %x23-%x5b / %x5d-%x7e
    // RWS = 1*(SP / HTAB)
    // OWS = *(SP / HTAB)
    // SP = %x20
    // HTAB = %x9
    // DQUOTE = %x22
    // BSLASH = %x5c
}

//===========================================================================
static void getCoreRules (set<Element> & rules) {
    Element * rule;
//    Element * elem;

    // ALPHA          =  %x41-5A / %x61-7A   ; A-Z / a-z
    rule = addChoiceRule(rules, "ALPHA", 1, 1);
    addRange(rule, 0x41, 0x5a);
    addRange(rule, 0x61, 0x7a);

    // BIT            =  "0" / "1"
    rule = addChoiceRule(rules, "BIT", 1, 1);
    addLiteral(rule, "0", 1, 1);
    addLiteral(rule, "1", 1, 1);

    // CHAR           =  %x01-7F

    // CR             =  %x0D
    rule = addChoiceRule(rules, "CR", 1, 1);
    addRange(rule, 0xd, 0xd);
    
    // CRLF           =  CR LF
    rule = addSequenceRule(rules, "CRLF", 1, 1);
    addRule(rule, "CR", 1, 1);
    addRule(rule, "LF", 1, 1);

    // CTL            =  %x00-1F / %x7F

    // DIGIT          =  %x30-39
    rule = addChoiceRule(rules, "DIGIT", 1, 1);
    addRange(rule, 0x30, 0x39);

    // DQUOTE         =  %x22
    rule = addChoiceRule(rules, "DQUOTE", 1, 1);
    addRange(rule, 0x22, 0x22);

    // HEXDIG         =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
    rule = addChoiceRule(rules, "HEXDIG", 1, 1);
    addRule(rule, "DIGIT", 1, 1);
    addLiteral(rule, "A", 1, 1);
    addLiteral(rule, "B", 1, 1);
    addLiteral(rule, "C", 1, 1);
    addLiteral(rule, "D", 1, 1);
    addLiteral(rule, "E", 1, 1);
    addLiteral(rule, "F", 1, 1);

    // HTAB           =  %x09
    rule = addChoiceRule(rules, "HTAB", 1, 1);
    addRange(rule, 0x9, 0x9);
    
    // LF             =  %x0A
    rule = addChoiceRule(rules, "LF", 1, 1);
    addRange(rule, 0xa, 0xa);

    // LWSP           =  *(WSP / CRLF WSP)
    // OCTET          =  %x00-FF

    // SP             =  %x20
    rule = addChoiceRule(rules, "SP", 1, 1);
    addRange(rule, 0x20, 0x20);

    // VCHAR          =  %x21-7E
    rule = addChoiceRule(rules, "VCHAR", 1, 1);
    addRange(rule, 0x21, 0x7e);

    // WSP            =  SP / HTAB
    rule = addChoiceRule(rules, "WSP", 1, 1);
    addRule(rule, "SP", 1, 1);
    addRule(rule, "HTAB", 1, 1);
}

//===========================================================================
static void getAbnfRules (set<Element> & rules) {
    Element * rule;
    Element * elem;
    Element * elem2;

    // rulelist       =  1*( rule / (*c-wsp c-nl) )
    rule = addChoiceRule(rules, "rulelist", 1, kUnlimited);
    addRule(rule, "rule", 1, 1);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "c-wsp", 0, kUnlimited);
    addRule(elem, "c-nl", 1, 1);

    // rule           =  rulename defined-as elements c-nl
    rule = addSequenceRule(rules, "rule", 1, 1);
    addRule(rule, "rulename", 1, 1);
    addRule(rule, "defined-as", 1, 1);
    addRule(rule, "elements", 1, 1);
    addRule(rule, "c-nl", 1, 1);

    // rulename       =  ALPHA *(ALPHA / DIGIT / "-")
    rule = addSequenceRule(rules, "rulename", 1, 1);
    addRule(rule, "ALPHA", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRule(elem, "ALPHA", 1, 1);
    addRule(elem, "DIGIT", 1, 1);
    addLiteral(elem, "-", 1, 1);

    // defined-as     =  *c-wsp ("=" / "=/") *c-wsp
    rule = addSequenceRule(rules, "defined-as", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    elem = addChoice(rule, 1, 1);
    addLiteral(elem, "=", 1, 1);
    elem2 = addSequence(elem, 1, 1);
    addLiteral(elem2, "/", 1, 1);
    addLiteral(elem2, "=", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);

    // elements       =  alternation *c-wsp
    rule = addSequenceRule(rules, "elements", 1, 1);
    addRule(rule, "alternation", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);

    // c-wsp          =  WSP / (c-nl WSP)
    rule = addChoiceRule(rules, "c-wsp", 1, 1);
    addRule(rule, "WSP", 1, 1);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "c-nl", 1, 1);
    addRule(elem, "WSP", 1, 1);

    // c-nl           =  comment / CRLF
    rule = addChoiceRule(rules, "c-nl", 1, 1);
    addRule(rule, "comment", 1, 1);
    addRule(rule, "CRLF", 1, 1);

    // comment        =  ";" *(WSP / VCHAR) CRLF
    rule = addSequenceRule(rules, "comment", 1, 1);
    addLiteral(rule, ";", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRule(elem, "WSP", 1, 1);
    addRule(elem, "VCHAR", 1, 1);
    addRule(rule, "CRLF", 1, 1);

    // alternation    =  concatenation *(*c-wsp "/" *c-wsp concatenation)
    rule = addSequenceRule(rules, "alternation", 1, 1);
    addRule(rule, "concatenation", 1, 1);
    elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "c-wsp", 0, kUnlimited);
    addLiteral(elem, "/", 1, 1);
    addRule(elem, "c-wsp", 0, kUnlimited);
    addRule(elem, "concatenation", 1, 1);

    // concatenation  =  repetition *(1*c-wsp repetition)
    rule = addSequenceRule(rules, "concatenation", 1, 1);
    addRule(rule, "repetition", 1, 1);
    elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "c-wsp", 1, kUnlimited);
    addRule(elem, "repetition", 1, 1);

    // repetition     =  [repeat] element
    rule = addSequenceRule(rules, "repetition", 1, 1);
    addRule(rule, "repeat", 0, 1);
    addRule(rule, "element", 1, 1);

    // repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
    rule = addChoiceRule(rules, "repeat", 1, 1);
    addRule(rule, "DIGIT", 1, kUnlimited);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "DIGIT", 0, kUnlimited);
    addLiteral(elem, "*", 1, 1);
    addRule(elem, "DIGIT", 0, kUnlimited);

    // element        =  rulename / group / option / char-val / num-val 
    rule = addChoiceRule(rules, "element", 1, 1);
    addRule(rule, "rulename", 1, 1);
    addRule(rule, "group", 1, 1);
    addRule(rule, "option", 1, 1);
    addRule(rule, "char-val", 1, 1);
    addRule(rule, "num-val", 1, 1);

    // group          =  "(" *c-wsp alternation *c-wsp ")"
    rule = addSequenceRule(rules, "group", 1, 1);
    addLiteral(rule, "(", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addRule(rule, "alternation", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addLiteral(rule, ")", 1, 1);

    // option         =  "[" *c-wsp alternation *c-wsp "]"
    rule = addSequenceRule(rules, "option", 1, 1);
    addLiteral(rule, "[", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addRule(rule, "alternation", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addLiteral(rule, "]", 1, 1);

    // char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
    rule = addSequenceRule(rules, "char-val", 1, 1);
    addRule(rule, "DQUOTE", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRange(elem, 0x20, 0x21);
    addRange(elem, 0x23, 0x7e);
    addRule(rule, "DQUOTE", 1, 1);

    // num-val        =  "%" (bin-val / dec-val / hex-val)
    rule = addSequenceRule(rules, "num-val", 1, 1);
    addLiteral(rule, "%", 1, 1);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "bin-val", 1, 1);
    addRule(elem, "dec-val", 1, 1);
    addRule(elem, "hex-val", 1, 1);

    // bin-val        =  "b" 1*BIT
    //                 [ 1*("." 1*BIT) / ("-" 1*BIT) ]
    rule = addSequenceRule(rules, "bin-val", 1, 1);
    addLiteral(rule, "b", 1, 1);
    addRule(rule, "BIT", 1, kUnlimited);
    elem = addChoice(rule, 0, 1);
    elem2 = addSequence(elem, 1, kUnlimited);
    addLiteral(elem2, ".", 1, 1);
    addRule(elem2, "BIT", 1, kUnlimited);
    elem2 = addSequence(elem, 1, 1);
    addLiteral(elem2, "-", 1, 1);
    addRule(elem2, "BIT", 1, kUnlimited);

    // dec-val        =  "d" 1*DIGIT
    //                 [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
    rule = addSequenceRule(rules, "dec-val", 1, 1);
    addLiteral(rule, "d", 1, 1);
    addRule(rule, "DIGIT", 1, kUnlimited);
    elem = addChoice(rule, 0, 1);
    elem2 = addSequence(elem, 1, kUnlimited);
    addLiteral(elem2, ".", 1, 1);
    addRule(elem2, "DIGIT", 1, kUnlimited);
    elem2 = addSequence(elem, 1, 1);
    addLiteral(elem2, "-", 1, 1);
    addRule(elem2, "DIGIT", 1, kUnlimited);

    // hex-val        =  "x" 1*HEXDIG
    //                 [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
    rule = addSequenceRule(rules, "hex-val", 1, 1);
    addLiteral(rule, "x", 1, 1);
    addRule(rule, "HEXDIG", 1, kUnlimited);
    elem = addChoice(rule, 0, 1);
    elem2 = addSequence(elem, 1, kUnlimited);
    addLiteral(elem2, ".", 1, 1);
    addRule(elem2, "HEXDIG", 1, kUnlimited);
    elem2 = addSequence(elem, 1, 1);
    addLiteral(elem2, "-", 1, 1);
    addRule(elem2, "HEXDIG", 1, kUnlimited);
}

//===========================================================================
static Element * addElement (Element * rule, unsigned m, unsigned n) {
    rule->elements.resize(rule->elements.size() + 1);
    // rule->elements.emplace_back();
    Element * e = &rule->elements.back();
    e->id = ++s_nextElemId;
    e->parent = rule;
    e->m = m;
    e->n = n;
    return e;
}


/****************************************************************************
*     
*   Application
*     
***/

namespace {
class Application : public ITaskNotify {
    int m_argc;
    char ** m_argv;
public:
    Application (int argc, char * argv[]);
    void onTask () override;
};
} // namespace

//===========================================================================
Application::Application (int argc, char * argv[]) 
    : m_argc(argc)
    , m_argv(argv)
{}

//===========================================================================
void Application::onTask () {
    //if (m_argc < 2) {
    //    cout << "pargen v0.1.0 (" __DATE__ ") - simplistic parser generator\n"
    //        << "usage: pargen\n";
    //    return appSignalShutdown(kExitBadArgs);
    //}

    set<Element> rules;
    getLineRules(rules);
    rules.clear();
    getCoreRules(rules);
    getAbnfRules(rules);

    // ostringstream os;
    writeParser(cout, rules, "num-val");

    appSignalShutdown(kExitSuccess);
}


/****************************************************************************
*
*   Externals
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _set_error_mode(_OUT_TO_MSGBOX);

    Application app(argc, argv);
    return appRun(app);
}

//===========================================================================
Element * addSequenceRule (
    set<Element> & rules, 
    const string & name,
    unsigned m,
    unsigned n
) {
    auto e = addChoiceRule(rules, name, m, n);
    e = addSequence(e, 1, 1);
    return e;
}

//===========================================================================
Element * addChoiceRule (
    set<Element> & rules, 
    const string & name,
    unsigned m,
    unsigned n
) {
    Element e;
    e.id = ++s_nextElemId;
    e.name = name;
    e.m = m;
    e.n = n;
    e.type = Element::kChoice;
    auto ib = rules.insert(e);
    assert(ib.second);
    return const_cast<Element *>(&*ib.first);
}

//===========================================================================
Element * addSequence (Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kSequence;
    return e;
}

//===========================================================================
Element * addChoice (Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kChoice;
    return e;
}

//===========================================================================
void addRule (
    Element * rule, 
    const string & name, 
    unsigned m, 
    unsigned n
) {
    auto e = addElement(rule, m, n);
    e->type = Element::kRule;
    e->value = name;
}

//===========================================================================
void addTerminal (
    Element * rule,
    unsigned char ch,
    unsigned m,
    unsigned n
) {
    auto e = addElement(rule, m, n);
    e->type = Element::kTerminal;
    e->value = ch;
}

//===========================================================================
void addLiteral (
    Element * rule, 
    const string & value,
    unsigned m,
    unsigned n
) {
    auto c = addChoice(rule, m, n);
    for (unsigned char ch : value) {
        addTerminal(c, ch, 1, 1);
        if (islower(ch)) {
            addTerminal(c, (unsigned char) toupper(ch), 1, 1);
        } else if (isupper(ch)) {
            addTerminal(c, (unsigned char) tolower(ch), 1, 1);
        }
    }
}

//===========================================================================
void addRange (
    Element * rule, 
    unsigned char a,
    unsigned char b
) {
    assert(a <= b);
    for (; a <= b; ++a) {
        addTerminal(rule, a, 1, 1);
    }
}
