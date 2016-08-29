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
void getTestRules(set<Element> & rules, const string & root) {
    const char s_testRules[] = R"(
left-recorse = left-recurse %x73 / %x79
right-recurse = %x73 right-recurse / %x79
simple-recurse = simple-recurse / %x79
)";
    addOption(rules, kOptionRoot, root);
    bool valid = parseAbnf(&rules, s_testRules);
    assert(valid);
    valid = false;
}

static bool s_allRules = true;

//===========================================================================
static void getCoreRules(set<Element> & rules) {
    const char s_coreRules[] = R"(
ALPHA   =  %x41-5A / %x61-7A   ; A-Z / a-z
BIT     =  "0" / "1"
CHAR    =  %x01-7F
CR      =  %x0D
CRLF    =  CR LF
CTL     =  %x00-1F / %x7F
DIGIT   =  %x30-39
DQUOTE  =  %x22
HEXDIG  =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
HTAB    =  %x09
LF      =  %x0A
LWSP    =  *(WSP / NEWLINE WSP)
NEWLINE =  LF / CRLF
OCTET   =  %x00-FF
SP      =  %x20
VCHAR   =  %x21-7E
WSP     =  SP / HTAB
)";
    bool valid = parseAbnf(&rules, s_coreRules);
    assert(valid);

    if (!s_allRules) {
        const char s_reducedRules[] = R"(
ALPHA   =  %x5a
CRLF    =  CR
DIGIT   =  %x39
NEWLINE =  CR
WSP     =  SP
)";
        set<Element> replacements;
        valid = parseAbnf(&replacements, s_reducedRules);
        assert(valid);
        for (auto && rule : replacements) {
            rules.erase(rule);
            rules.insert(rule);
        }
    }
}

//===========================================================================
static void getAbnfRules(set<Element> & rules) {
    Element * rule;
    Element * elem;
    Element * elem2;

    addOption(rules, kOptionApiPrefix, "Abnf");
    if (s_allRules) {
        addOption(rules, kOptionRoot, "rulelist");
    } else {
        addOption(rules, kOptionRoot, "bin-val");
    }

    // definitions taken from rfc5234

    // rulelist       =  1*( rule / option / (*c-wsp c-nl) )
    rule = addChoiceRule(rules, "rulelist", 1, kUnlimited);
    addRule(rule, "rule", 1, 1);
    addRule(rule, "option", 1, 1);
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

    // option = "%" option-tail
    rule = addSequenceRule(rules, "option", 1, 1);
    addTerminal(rule, '%', 1, 1);
    addRule(rule, "option-tail", 1, 1);
    // option-tail* = optionname defined-as optionlist c-nl
    rule = addSequenceRule(rules, "option-tail", 1, 1, 0, false);
    addRule(rule, "optionname", 1, 1);
    addRule(rule, "defined-as", 1, 1);
    addRule(rule, "optionlist", 1, 1);
    addRule(rule, "c-nl", 1, 1);
    // optionname = ALPHA *(ALPHA / "." / "-") {Char}
    rule = addSequenceRule(rules, "optionname", 1, 1, Element::kOnChar);
    addRule(rule, "ALPHA", 1, 1);
    elem = addChoice(rule, 0, kUnlimited);
    addRule(elem, "ALPHA", 1, 1);
    addLiteral(elem, ".", 1, 1);
    addLiteral(elem, "-", 1, 1);
    // optionlist = optiondef *(1*c-wsp optiondef) {Start, End}
    rule = addSequenceRule(
        rules, "optionlist", 1, 1, Element::kOnStart | Element::kOnEnd);
    addRule(rule, "optiondef", 1, 1);
    elem = addSequence(rule, 0, kUnlimited);
    addRule(elem, "c-wsp", 1, kUnlimited);
    addRule(elem, "optiondef", 1, 1);
    // optiondef = option-unquoted / DQUOTE option-quoted DQUOTE {End}
    rule = addChoiceRule(rules, "optiondef", 1, 1, Element::kOnEnd);
    addRule(rule, "option-unquoted", 1, 1);
    elem = addSequence(rule, 1, 1);
    addRule(elem, "DQUOTE", 1, 1);
    addRule(elem, "option-quoted", 1, 1);
    addRule(elem, "DQUOTE", 1, 1);
    // option-char = ALPHA / DIGIT / "." / "-" / "_" / ":"
    rule = addChoiceRule(rules, "option-char", 1, 1);
    addRule(rule, "ALPHA", 1, 1);
    addRule(rule, "DIGIT", 1, 1);
    addTerminal(rule, '.', 1, 1);
    addTerminal(rule, '-', 1, 1);
    addTerminal(rule, '_', 1, 1);
    addTerminal(rule, ':', 1, 1);
    // option-unquoted = 1*option-char {Char}
    rule = addSequenceRule(rules, "option-unquoted", 1, 1, Element::kOnChar);
    addRule(rule, "option-char", 1, kUnlimited);
    // option-quoted = 1*(option-char / " " / "/") {Char}
    rule =
        addChoiceRule(rules, "option-quoted", 1, kUnlimited, Element::kOnChar);
    addRule(rule, "option-char", 1, 1);
    addTerminal(rule, ' ', 1, 1);
    addTerminal(rule, '/', 1, 1);

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
        addRange(rule, 0x20, 0x21);
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

//===========================================================================
static void writeParserFiles(set<Element> & rules) {
    TimePoint start = Clock::now();
    if (processOptions(rules)) {
        ofstream oh(getOptionString(rules, kOptionApiHeaderFile));
        ofstream ocpp(getOptionString(rules, kOptionApiCppFile));
        writeParser(oh, ocpp, rules);
        oh.close();
        ocpp.close();
    }
    TimePoint finish = Clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    cout << "Elapsed time: " << elapsed.count() << " seconds" << endl;
}


/****************************************************************************
 *
 *   Application
 *
 ***/

namespace {
class Application : public ITaskNotify, public IFileReadNotify {
    int m_argc;
    char ** m_argv;
    string m_source;

  public:
    Application(int argc, char * argv[]);
    void onTask() override;

    // IFileReadNotify
    void onFileEnd(int64_t offset, IFile * file) override;
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

    if (m_argc > 1) {
        fileReadBinary(this, m_source, m_argv[1]);
        return;
    }

    set<Element> rules;
    getCoreRules(rules);
    getAbnfRules(rules);
    // getTestRules(rules, "left-recurse");

    writeParserFiles(rules);

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

//===========================================================================
void Application::onFileEnd(int64_t offset, IFile * file) {
    set<Element> rules;
    getCoreRules(rules);
    m_source = "%a = x ; tmp\r\n%b = 2\n";
    if (parseAbnf(&rules, m_source)) {
        writeParserFiles(rules);
    }
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
void addOption(
    set<Element> & rules, const string & name, const string & value) {
    auto e = addChoiceRule(rules, name, 1, 1);
    e->type = Element::kRule;
    e->value = value;
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
    for (unsigned i = a; i <= b; ++i) {
        addTerminal(rule, (unsigned char)i, 1, 1);
    }
}

//===========================================================================
const char * getOptionString(const set<Element> & src, const string & name) {
    Element key;
    key.name = name;
    auto it = src.find(key);
    if (it == src.end())
        return "";
    const Element * elem = &*it;
    while (!elem->elements.empty())
        elem = &elem->elements.front();
    assert(elem->type == Element::kRule);
    return elem->value.c_str();
}

//===========================================================================
unsigned getOptionUnsigned(const set<Element> & src, const string & name) {
    return (unsigned)strtoul(getOptionString(src, name), nullptr, 10);
}
