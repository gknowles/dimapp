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
static void getLineRules(set<Element> & rules) {
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
static void getTestRules(set<Element> & rules) {
    Element * rule;
    Element * elem;
    rule = addChoiceRule(rules, "left-recurse", 1, 1);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "left-recurse", 1, 1);
    addTerminal(elem, 's', 1, 1);
    addTerminal(rule, 'y', 1, 1);

    rule = addChoiceRule(rules, "right-recurse", 1, 1);
    elem = addSequence(rule, 1, 1);
    addTerminal(elem, 's', 1, 1);
    addRule(elem, "right-recurse", 1, 1);
    addTerminal(rule, 'y', 1, 1);

    rule = addChoiceRule(rules, "simple-recurse", 1, 1);
    addRule(rule, "simple-recurse", 1, 1);
    addTerminal(rule, 'y', 1, 1);
}

static bool s_allRules = true;

//===========================================================================
static void getCoreRules(set<Element> & rules) {
    Element * rule;

    // ALPHA          =  %x41-5A / %x61-7A   ; A-Z / a-z
    rule = addChoiceRule(rules, "ALPHA", 1, 1);
    if (s_allRules) {
        addRange(rule, 0x41, 0x5a);
        addRange(rule, 0x61, 0x7a);
    } else {
        addRange(rule, 0x5a, 0x5a);
    }

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
    if (s_allRules) {
        addRule(rule, "LF", 1, 1);
    }

    // CTL            =  %x00-1F / %x7F

    // DIGIT          =  %x30-39
    rule = addChoiceRule(rules, "DIGIT", 1, 1);
    if (s_allRules) {
        addRange(rule, 0x30, 0x39);
    } else {
        addRange(rule, 0x39, 0x39);
    }

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

    // NEWLINE        =  CR / LF / CRLF
    rule = addChoiceRule(rules, "NEWLINE", 1, 1);
    if (s_allRules) {
        addRule(rule, "LF", 1, 1);
        addRule(rule, "CRLF", 1, 1);
    } else {
        addRule(rule, "CR", 1, 1);
    }

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
    if (s_allRules) {
        addRule(rule, "HTAB", 1, 1);
    }
}

//===========================================================================
static void getAbnfRules(set<Element> & rules) {
    Element * rule;
    Element * elem;
    Element * elem2;

    // definitions taken from rfc5234

    // rulelist       =  1*( rule / (*c-wsp c-nl) )
    rule = addChoiceRule(rules, "rulelist", 1, kUnlimited);
    addRule(rule, "rule", 1, 1);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "c-wsp", 0, kUnlimited);
    // addRule(elem, "WSP", 0, kUnlimited);    // see errata 3076
    addRule(elem, "c-nl", 1, 1);

    // rule =  rulename [rule-recurse] defined-as elements [actions] c-nl
    rule = addSequenceRule(rules, "rule", 1, 1, Element::kOnEnd);
    addRule(rule, "rulename", 1, 1);
    addRule(rule, "defined-as", 1, 1);
    addRule(rule, "elements", 1, 1);
    addRule(rule, "actions", 0, 1);
    addRule(rule, "c-nl", 1, 1);

    // rulename       =  ALPHA *(ALPHA / DIGIT / "-")
    rule = addSequenceRule(
        rules, "rulename", 1, 1, Element::kOnStart | Element::kOnChar);
    addRule(rule, "ALPHA", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRule(elem, "ALPHA", 1, 1);
    addRule(elem, "DIGIT", 1, 1);
    addLiteral(elem, "-", 1, 1);

    // rule-recurse = "*"
    rule = addSequenceRule(rules, "rule-recurse", 1, 1, Element::kOnEnd);
    addLiteral(rule, "*", 1, 1);

    // defined-as =  *c-wsp (defined-as-set / defined-as-incremental) *c-wsp
    rule = addSequenceRule(rules, "defined-as", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "defined-as-set", 1, 1);
    addRule(elem, "defined-as-incremental", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    // defined-as-set = "="
    rule = addSequenceRule(rules, "defined-as-set", 1, 1, Element::kOnEnd);
    addLiteral(rule, "=", 1, 1);
    // defined-as-incremental = "=/"
    rule =
        addSequenceRule(rules, "defined-as-incremental", 1, 1, Element::kOnEnd);
    addLiteral(rule, "=/", 1, 1);

    // elements       =  alternation *c-wsp
    rule = addSequenceRule(rules, "elements", 1, 1);
    addRule(rule, "alternation", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    // addRule(rule, "WSP", 0, kUnlimited);        // see errata 2968

    // actions = "{" *c-wsp [action *(*c-wsp "," *c-wsp action)] *c-wsp "}"
    rule = addSequenceRule(rules, "actions", 1, 1);
    addLiteral(rule, "{", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    elem = addSequence(rule, 0, 1);
    addRule(elem, "action", 1, 1);
    elem2 = addSequence(elem, 0, kUnlimited);
    addRule(elem2, "c-wsp", 0, kUnlimited);
    addLiteral(elem2, ",", 1, 1);
    addRule(elem2, "c-wsp", 0, kUnlimited);
    addRule(elem2, "action", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addLiteral(rule, "}", 1, 1);
    // action = action-start / action-end / action-char
    rule = addChoiceRule(rules, "action", 1, 1);
    addRule(rule, "action-start", 1, 1);
    addRule(rule, "action-end", 1, 1);
    addRule(rule, "action-char", 1, 1);
    // action-start = "start" {end}
    rule = addSequenceRule(rules, "action-start", 1, 1, Element::kOnEnd);
    addLiteral(rule, "start", 1, 1);
    // action-end = "end" {end}
    rule = addSequenceRule(rules, "action-end", 1, 1, Element::kOnEnd);
    addLiteral(rule, "end", 1, 1);
    // action-char = "char" {end}
    rule = addSequenceRule(rules, "action-char", 1, 1, Element::kOnEnd);
    addLiteral(rule, "char", 1, 1);

    // c-wsp          =  WSP / (c-nl WSP)
    rule = addChoiceRule(rules, "c-wsp", 1, 1);
    addRule(rule, "WSP", 1, 1);
    if (s_allRules) {
        elem = addSequence(rule, 1, 1);
        addRule(elem, "c-nl", 1, 1);
        addRule(elem, "WSP", 1, 1);
    }

    // c-nl           =  comment / NEWLINE
    rule = addChoiceRule(rules, "c-nl", 1, 1);
    if (s_allRules) {
        addRule(rule, "comment", 1, 1);
    }
    addRule(rule, "NEWLINE", 1, 1);
    // comment        =  ";" *(WSP / VCHAR) NEWLINE
    rule = addSequenceRule(rules, "comment", 1, 1);
    addLiteral(rule, ";", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRule(elem, "WSP", 1, 1);
    addRule(elem, "VCHAR", 1, 1);
    addRule(rule, "NEWLINE", 1, 1);

    // alternation    =  concatenation *(*c-wsp "/" *c-wsp concatenation)
    rule = addSequenceRule(
        rules, "alternation", 1, 1, Element::kOnStart | Element::kOnEnd);
    addRule(rule, "concatenation", 1, 1);
    elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "c-wsp", 0, kUnlimited);
    addLiteral(elem, "/", 1, 1);
    addRule(elem, "c-wsp", 0, kUnlimited);
    addRule(elem, "concatenation", 1, 1);

    // concatenation  =  repetition *(1*c-wsp repetition)
    rule = addSequenceRule(
        rules, "concatenation", 1, 1, Element::kOnStart | Element::kOnEnd);
    addRule(rule, "repetition", 1, 1);
    elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "c-wsp", 1, kUnlimited);
    addRule(elem, "repetition", 1, 1);

    // repetition     =  [repeat] element
    rule = addSequenceRule(rules, "repetition", 1, 1, Element::kOnStart);
    addRule(rule, "repeat", 0, 1);
    addRule(rule, "element", 1, 1);

    // repeat         =  repeat-minmax / repeat-range
    rule = addChoiceRule(rules, "repeat", 1, 1);
    addRule(rule, "repeat-minmax", 1, 1);
    addRule(rule, "repeat-range", 1, 1);
    // repeat-minmax = dec-val-sequence
    rule = addChoiceRule(rules, "repeat-minmax", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);
    // repeat-range = [dec-val-sequence] * [repeat-max]
    rule = addSequenceRule(rules, "repeat-range", 1, 1, Element::kOnStart);
    addRule(rule, "dec-val-sequence", 0, 1);
    addLiteral(rule, "*", 1, 1);
    addRule(rule, "repeat-max", 0, 1);
    // repeat-max = dec-val-sequence
    rule = addChoiceRule(rules, "repeat-max", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);

    // element        =  ruleref / group / char-val / num-val
    rule = addChoiceRule(rules, "element", 1, 1);
    addRule(rule, "ruleref", 1, 1);
    addRule(rule, "group", 1, 1);
    addRule(rule, "char-val", 1, 1);
    addRule(rule, "num-val", 1, 1);
    // ruleref = rulename
    rule = addChoiceRule(rules, "ruleref", 1, 1, Element::kOnEnd);
    addRule(rule, "rulename", 1, 1);

    // group =  ( "(" / "[" ) group-tail
    rule = addSequenceRule(
        rules, "group", 1, 1, Element::kOnStart | Element::kOnEnd);
    elem = addChoice(rule, 1, 1);
    addLiteral(elem, "(", 1, 1);
    addLiteral(elem, "[", 1, 1);
    addRule(rule, "group-tail", 1, 1);
    // group-tail* =  *c-wsp alternation *c-wsp ( ")" / "]" )
    rule = addSequenceRule(rules, "group-tail", 1, 1, 0, true);
    addRule(rule, "c-wsp", 0, kUnlimited);
    addRule(rule, "alternation", 1, 1);
    addRule(rule, "c-wsp", 0, kUnlimited);
    elem = addChoice(rule, 1, 1);
    addLiteral(elem, ")", 1, 1);
    addLiteral(elem, "]", 1, 1);

    // char-val       =  DQUOTE char-val-sequence DQUOTE
    rule = addSequenceRule(rules, "char-val", 1, 1, Element::kOnEnd);
    addRule(rule, "DQUOTE", 1, 1);
    addRule(rule, "char-val-sequence", 1, 1);
    addRule(rule, "DQUOTE", 1, 1);
    // char-val-sequence = *(%x20-21 / %x23-7E)
    rule = addChoiceRule(
        rules,
        "char-val-sequence",
        0,
        kUnlimited,
        Element::kOnStart | Element::kOnChar);
    if (s_allRules) {
        addRange(rule, 0x23, 0x7e);
    } else {
        addRange(rule, 0x20, 0x21);
    }

    // num-val        =  "%" (bin-val / dec-val / hex-val)
    rule = addSequenceRule(rules, "num-val", 1, 1);
    addLiteral(rule, "%", 1, 1);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "bin-val", 1, 1);
    addRule(elem, "dec-val", 1, 1);
    addRule(elem, "hex-val", 1, 1);

    // bin-val-sequence = 1*BIT
    rule = addSequenceRule(rules, "bin-val-sequence", 1, 1, Element::kOnChar);
    addRule(rule, "BIT", 1, kUnlimited);
    // dec-val-sequence = 1*DIGIT
    rule = addSequenceRule(rules, "dec-val-sequence", 1, 1, Element::kOnChar);
    addRule(rule, "DIGIT", 1, kUnlimited);
    // hex-val-sequence = 1*HEXDIG
    rule = addSequenceRule(rules, "hex-val-sequence", 1, 1, Element::kOnChar);
    addRule(rule, "HEXDIG", 1, kUnlimited);

    // bin-val = "b" (bin-val-simple / bin-val-concatenation
    //      / bin-val-alterntation)
    rule = addSequenceRule(rules, "bin-val", 1, 1);
    addLiteral(rule, "b", 1, 1);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "bin-val-simple", 1, 1);
    addRule(elem, "bin-val-concatenation", 1, 1);
    addRule(elem, "bin-val-alternation", 1, 1);
    // bin-val-simple = bin-val-sequence
    rule = addSequenceRule(rules, "bin-val-simple", 1, 1, Element::kOnEnd);
    addRule(rule, "bin-val-sequence", 1, 1);
    // bin-val-concatenation = bin-val-concat-each 1*("." bin-val-concat-each)
    rule = addSequenceRule(
        rules,
        "bin-val-concatenation",
        1,
        1,
        Element::kOnStart | Element::kOnEnd);
    addRule(rule, "bin-val-concat-each", 1, 1);
    elem = addSequence(rule, 1, kUnlimited);
    addLiteral(elem, ".", 1, 1);
    addRule(elem, "bin-val-concat-each", 1, 1);
    // bin-val-concat-each = bin-val-sequence
    rule = addSequenceRule(rules, "bin-val-concat-each", 1, 1, Element::kOnEnd);
    addRule(rule, "bin-val-sequence", 1, 1);
    // bin-val-alternation = bin-val-alt-first "-" bin-val-alt-second
    rule = addSequenceRule(rules, "bin-val-alternation", 1, 1);
    addRule(rule, "bin-val-alt-first", 1, 1);
    addLiteral(rule, "-", 1, 1);
    addRule(rule, "bin-val-alt-second", 1, 1);
    // bin-val-alt-first = bin-val-sequence
    rule = addSequenceRule(rules, "bin-val-alt-first", 1, 1, Element::kOnEnd);
    addRule(rule, "bin-val-sequence", 1, 1);
    // bin-val-alt-second = bin-val-sequence
    rule = addSequenceRule(rules, "bin-val-alt-second", 1, 1, Element::kOnEnd);
    addRule(rule, "bin-val-sequence", 1, 1);

    // dec-val = "d" (dec-val-simple / dec-val-concatenation
    //      / dec-val-alterntation)
    rule = addSequenceRule(rules, "dec-val", 1, 1);
    addLiteral(rule, "d", 1, 1);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "dec-val-simple", 1, 1);
    addRule(elem, "dec-val-concatenation", 1, 1);
    addRule(elem, "dec-val-alternation", 1, 1);
    // dec-val-simple = dec-val-sequence
    rule = addSequenceRule(rules, "dec-val-simple", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);
    // dec-val-concatenation = dec-val-concat-each 1*("." dec-val-concat-each)
    rule = addSequenceRule(
        rules,
        "dec-val-concatenation",
        1,
        1,
        Element::kOnStart | Element::kOnEnd);
    addRule(rule, "dec-val-concat-each", 1, 1);
    elem = addSequence(rule, 1, kUnlimited);
    addLiteral(elem, ".", 1, 1);
    addRule(elem, "dec-val-concat-each", 1, 1);
    // dec-val-concat-each = dec-val-sequence
    rule = addSequenceRule(rules, "dec-val-concat-each", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);
    // dec-val-alternation = dec-val-alt-first "-" dec-val-alt-second
    rule = addSequenceRule(rules, "dec-val-alternation", 1, 1);
    addRule(rule, "dec-val-alt-first", 1, 1);
    addLiteral(rule, "-", 1, 1);
    addRule(rule, "dec-val-alt-second", 1, 1);
    // dec-val-alt-first = dec-val-sequence
    rule = addSequenceRule(rules, "dec-val-alt-first", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);
    // dec-val-alt-second = dec-val-sequence
    rule = addSequenceRule(rules, "dec-val-alt-second", 1, 1, Element::kOnEnd);
    addRule(rule, "dec-val-sequence", 1, 1);

    // hex-val = "x" (hex-val-simple / hex-val-concatenation
    //      / hex-val-alterntation)
    rule = addSequenceRule(rules, "hex-val", 1, 1);
    addLiteral(rule, "x", 1, 1);
    elem = addChoice(rule, 1, 1);
    addRule(elem, "hex-val-simple", 1, 1);
    addRule(elem, "hex-val-concatenation", 1, 1);
    addRule(elem, "hex-val-alternation", 1, 1);
    // hex-val-simple = hex-val-sequence
    rule = addSequenceRule(rules, "hex-val-simple", 1, 1, Element::kOnEnd);
    addRule(rule, "hex-val-sequence", 1, 1);
    // hex-val-concatenation = hex-val-concat-each 1*("." hex-val-concat-each)
    rule = addSequenceRule(
        rules,
        "hex-val-concatenation",
        1,
        1,
        Element::kOnStart | Element::kOnEnd);
    addRule(rule, "hex-val-concat-each", 1, 1);
    elem = addSequence(rule, 1, kUnlimited);
    addLiteral(elem, ".", 1, 1);
    addRule(elem, "hex-val-concat-each", 1, 1);
    // hex-val-concat-each = hex-val-sequence
    rule = addSequenceRule(rules, "hex-val-concat-each", 1, 1, Element::kOnEnd);
    addRule(rule, "hex-val-sequence", 1, 1);
    // hex-val-alternation = hex-val-alt-first "-" hex-val-alt-second
    rule = addSequenceRule(rules, "hex-val-alternation", 1, 1);
    addRule(rule, "hex-val-alt-first", 1, 1);
    addLiteral(rule, "-", 1, 1);
    addRule(rule, "hex-val-alt-second", 1, 1);
    // hex-val-alt-first = hex-val-sequence
    rule = addSequenceRule(rules, "hex-val-alt-first", 1, 1, Element::kOnEnd);
    addRule(rule, "hex-val-sequence", 1, 1);
    // hex-val-alt-second = hex-val-sequence
    rule = addSequenceRule(rules, "hex-val-alt-second", 1, 1, Element::kOnEnd);
    addRule(rule, "hex-val-sequence", 1, 1);
}

//===========================================================================
static Element * addElement(Element * rule, unsigned m, unsigned n) {
    rule->elements.resize(rule->elements.size() + 1);
    // rule->elements.emplace_back();
    Element * e = &rule->elements.back();
    e->id = ++s_nextElemId;
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
    Application(int argc, char * argv[]);
    void onTask() override;
};
} // namespace

//===========================================================================
Application::Application(int argc, char * argv[])
    : m_argc(argc)
    , m_argv(argv) {}

//===========================================================================
void Application::onTask() {
    // if (m_argc < 2) {
    //    cout << "pargen v0.1.0 (" __DATE__ ") - simplistic parser
    //    generator\n"
    //        << "usage: pargen\n";
    //    return appSignalShutdown(kExitBadArgs);
    //}

    set<Element> rules;
    getLineRules(rules);
    rules.clear();
    getCoreRules(rules);
    getAbnfRules(rules);
    getTestRules(rules);

    TimePoint start = Clock::now();
    ofstream oh("abnfparse.h");
    ofstream ocpp("abnfparse.cpp");
    if (s_allRules) {
        writeParser(oh, ocpp, rules, "rulelist");
    } else {
        writeParser(oh, ocpp, rules, "bin-val");
    }
    oh.close();
    ocpp.close();
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    cout << "Elapsed time: " << elapsed.count() << " seconds" << endl;

    ostringstream abnf;
    for (auto && rule : rules) {
        abnf << rule.name << " = " << rule << '\n';
    }
    rules.clear();
    bool valid = parseAbnf(&rules, abnf.str());
    cout << "Valid: " << valid << endl;

    ostringstream o1;
    for (auto && rule : rules) {
        o1 << rule.name << " = " << rule << '\n';
    }

    ostringstream o2;
    rules.clear();
    valid = parseAbnf(&rules, o1.str());
    for (auto && rule : rules) {
        o2 << rule.name << " = " << rule << '\n';
    }
    cout << "Round trip: " << (o1.str() == o2.str()) << endl;

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

    consoleEnableCtrlC(false);
    Application app(argc, argv);
    return appRun(app);
}

//===========================================================================
Element * addSequenceRule(
    set<Element> & rules,
    const string & name,
    unsigned m,
    unsigned n,
    unsigned flags, // Element::kFlag*
    bool recurse) {
    auto e = addChoiceRule(rules, name, m, n, flags, recurse);
    e = addSequence(e, 1, 1);
    return e;
}

//===========================================================================
Element * addChoiceRule(
    set<Element> & rules,
    const string & name,
    unsigned m,
    unsigned n,
    unsigned flags, // Element::kFlag*
    bool recurse) {
    Element e;
    e.id = ++s_nextElemId;
    e.name = name;
    e.m = m;
    e.n = n;
    e.type = Element::kChoice;
    e.flags = flags;
    e.recurse = recurse;
    auto ib = rules.insert(e);
    assert(ib.second);
    return const_cast<Element *>(&*ib.first);
}

//===========================================================================
Element * addSequence(Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kSequence;
    return e;
}

//===========================================================================
Element * addChoice(Element * rule, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kChoice;
    return e;
}

//===========================================================================
void addRule(Element * rule, const string & name, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kRule;
    e->value = name;
}

//===========================================================================
void addTerminal(Element * rule, unsigned char ch, unsigned m, unsigned n) {
    auto e = addElement(rule, m, n);
    e->type = Element::kTerminal;
    e->value = ch;
}

//===========================================================================
void addLiteral(Element * rule, const string & value, unsigned m, unsigned n) {
    auto s = addSequence(rule, m, n);
    for (unsigned char ch : value) {
        auto c = addChoice(s, 1, 1);
        addTerminal(c, ch, 1, 1);
        if (islower(ch)) {
            addTerminal(c, (unsigned char)toupper(ch), 1, 1);
        } else if (isupper(ch)) {
            addTerminal(c, (unsigned char)tolower(ch), 1, 1);
        }
    }
}

//===========================================================================
void addRange(Element * rule, unsigned char a, unsigned char b) {
    assert(a <= b);
    for (; a <= b; ++a) {
        addTerminal(rule, a, 1, 1);
    }
}
