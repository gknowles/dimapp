// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - pargen
#pragma once


/****************************************************************************
*
*   Declarations
*
***/

char const kVersion[] = "2.2.0";


/****************************************************************************
*
*   Grammar rules
*
***/

char const kOptionRoot[] = "%root";
char const kOptionInclude[] = "%include";
char const kOptionApiPrefix[] = "%api.prefix";
char const kOptionApiParserHeader[] = "%api.parser.file.h";
char const kOptionApiParserCpp[] = "%api.parser.file.cpp";
char const kOptionApiParserClass[] = "%api.parser.className";
char const kOptionApiBaseHeader[] = "%api.base.file.h";
char const kOptionApiBaseClass[] = "%api.base.className";
char const kOptionApiNamespace[] = "%api.namespace";

char const kDoneRuleName[] = "%%DONE";

unsigned const kUnlimited = unsigned(-1);

struct Element {
    enum Flags : uint16_t {
        // Simple events with no context
        fOnStart = 1,
        fOnChar = 2,
        fOnEnd = 4,

        // Events with context data, requires more work tracking internal
        // state. Not supported when the grammar allows position to be
        // extended indefinitely.
        fOnStartW = 8,
        fOnCharW = 16,
        fOnEndW = 32,

        fFunction = 64,

        // If complexity has been tested one of these will be set:
        fSimple = 128,  // is single terminal or choice of terminals and
                        //   no other flags
        fComplex = 256, // is not fSimple

        fStartEvents = fOnStart | fOnStartW,
        fCharEvents = fOnChar | fOnCharW,
        fEndEvents = fOnEnd | fOnEndW,
        fEvents = fStartEvents | fCharEvents | fEndEvents,
        fComplexityFlags = fSimple | fComplex,
    };

    enum Type : int8_t {
        kRule,
        kSequence,
        kChoice,
        kTerminal,
    };

    Type type{kRule};
    std::string name;
    std::string value;
    unsigned m{1};
    unsigned n{1};
    unsigned id{};
    unsigned pos{}; // position in sequence, 0 for non-sequences

    std::vector<Element> elements;
    Element const * rule{};
    Element const * eventRule{}; // only present if different from rule
    Flags flags{};
    std::string eventName; // only present if different from name

    bool operator<(Element const & right) const { return name < right.name; }

private:
    friend std::ostream & operator<<(std::ostream & os, Element const & elem);
};

struct ElementDone : Element {
    ElementDone();
    static ElementDone s_consume;
    static ElementDone s_abort;
};

struct ElementNull : Element {
    ElementNull();
    static ElementNull s_elem;
};

class Grammar {
public:
    void clear();
    void clearEvents();

    void addOption(std::string_view name, std::string_view value);
    void setOption(std::string_view name, std::string_view value);
    void eraseOption(std::string_view name);

    std::vector<std::string_view> optionStrings(std::string_view name) const;
    char const * optionString(
        std::string_view name,
        char const * def = ""
    ) const;
    unsigned optionUnsigned(std::string_view name, unsigned def = 0) const;
    char const * operator[](std::string_view name) const;

    Element * addSequenceRule(
        std::string_view name,
        unsigned m,
        unsigned n,
        Element::Flags flags = {}
    );
    Element * addChoiceRule(
        std::string_view name,
        unsigned m,
        unsigned n,
        Element::Flags flags = {}
    );
    Element * addSequence(Element * rule, unsigned m, unsigned n);
    Element * addChoice(Element * rule, unsigned m, unsigned n);
    void addRule(
        Element * rule,
        std::string_view name,
        unsigned m,
        unsigned n
    );

    // case insensitive
    void addText(
        Element * rule,
        std::string_view value,
        unsigned m,
        unsigned n
    );

    void addLiteral(
        Element * rule,
        std::string_view value,
        unsigned m,
        unsigned n
    );
    void addRange(Element * rule, unsigned char a, unsigned char b);
    void addTerminal(Element * rule, unsigned char ch);

    Element const * eventAlways(std::string_view name);

    Element * element(std::string_view name);
    Element const * element(std::string_view name) const;

    std::set<Element> const & rules() const { return m_rules; }
    std::set<Element> & rules() { return m_rules; }

    size_t errpos() const { return m_errpos; }
    void errpos(size_t where) { m_errpos = where; }

private:
    Element * addElement(Element * rule, unsigned m, unsigned n);

    unsigned m_nextElemId{0};
    std::set<Element> m_rules;
    std::set<Element> m_eventNames; // names referenced without backing rules
    size_t m_errpos{0};
};

// modify options
bool processOptions(Grammar & rules);

// modify rules
bool copyRules(
    Grammar & out,
    Grammar const & src,
    std::string_view root,
    bool failIfExists
);
void normalize(Grammar & rules);
void merge(Grammar & rules);
void functionTags(Grammar & rules, Element & rule, bool reset, bool mark);


/****************************************************************************
*
*   Parse state
*
***/

char const kLeftQ[] = ": ";
char const kRightQ[] = " :";

char const kRootStateName[] = "<ROOT>";
char const kDoneStateName[] = "<DONE>";
char const kFailedStateName[] = "<FAILED>";

struct StateElement {
    Element const * elem{};
    unsigned rep{};
    bool started{};
    bool recurse{};

    int compare(StateElement const & right) const;
    bool operator<(StateElement const & right) const;
    bool operator==(StateElement const & right) const;

    bool operator!=(StateElement const & right) const {
        return !operator==(right);
    }
};

struct StateEvent {
    Element const * elem{};
    Element::Flags flags{};
    int distance{};

    int compare(StateEvent const & right) const;
    bool operator<(StateEvent const & right) const;
    bool operator==(StateEvent const & right) const;

    bool operator!=(StateEvent const & right) const {
        return !operator==(right);
    }
};

struct StatePosition {
    std::vector<StateElement> elems;
    std::vector<StateEvent> events;
    std::vector<StateEvent> delayedEvents;
    int recurseSe{}; // state element of recursion entry point (0 for none)
    int recursePos{-1}; // position in kSequence of recursion entry point

    int compare(StatePosition const & right) const;
    bool operator<(StatePosition const & right) const;
    bool operator==(StatePosition const & right) const;
};

struct State {
    unsigned id{};
    std::string name;
    std::vector<std::string> aliases;

    // bitset is 256 to allow space for all possible terminals, count() == 0
    // if there are no terminals because it's a function.
    std::map<StatePosition, std::bitset<256>> positions;

    std::vector<unsigned> next;

    void clear();
    bool operator==(State const & right) const;
};

//===========================================================================
namespace std {

template <> struct hash<StateElement> {
    size_t operator()(StateElement const & val) const;
};
template <> struct hash<StateEvent> {
    size_t operator()(StateEvent const & val) const;
};
template <> struct hash<StatePosition> {
    size_t operator()(StatePosition const & val) const;
};
template <> struct hash<State> {
    size_t operator()(State const & val) const;
};

} // namespace std


// build state tree
void buildStateTree(
    std::unordered_set<State> * states,
    Element const & root,
    bool inclDeps,
    bool dedupStates,
    unsigned depthLimit
);
void dedupStateTree(std::unordered_set<State> & states);


/****************************************************************************
*
*   Public API
*
***/

struct RunOptions {
    bool minRules;
    bool mergeRules;
    bool resetFunctions;
    bool markFunctions;
    bool includeCallbacks;
    bool buildStateTree;
    unsigned stateTreeDepthLimit;
    bool dedupStateTree;
    bool writeStatePositions;
    bool writeFunctions;
    bool verbose;
};

// write generated code
void writeParser(
    std::ostream & hfile,
    std::ostream & cppfile,
    Grammar const & rules,
    RunOptions const & opts
);

void writeRule(
    std::ostream & os,
    Element const & rule,
    size_t maxWidth,
    std::string_view prefix
);
