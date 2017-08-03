// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - pargen
#pragma once


/****************************************************************************
*
*   Grammar rules
*
***/

const char kOptionRoot[] = "%root";
const char kOptionApiPrefix[] = "%api.prefix";
const char kOptionApiParserHeader[] = "%api.parser.file.h";
const char kOptionApiParserCpp[] = "%api.parser.file.cpp";
const char kOptionApiParserClass[] = "%api.parser.className";
const char kOptionApiBaseHeader[] = "%api.base.file.h";
const char kOptionApiBaseClass[] = "%api.base.className";
const char kOptionApiNamespace[] = "%api.namespace";

const char kDoneRuleName[] = "%%DONE";

const unsigned kUnlimited = unsigned(-1);

struct Element {
    enum Flags : uint8_t {
        fOnStart = 1,
        fOnChar = 2,
        fOnEnd = 4,
        fFunction = 8,

        // If complexity has been tested one of these will be set:
        fSimple = 16,   // is single terminal or choice of terminals and 
                        //   no other flags
        fComplex = 32,  // is not fSimple

        fCallbackFlags = fOnStart | fOnChar | fOnEnd,
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
    unsigned id{0};
    unsigned pos{0}; // position in sequence, 0 for non-sequences

    std::vector<Element> elements;
    const Element * rule{nullptr};
    const Element * eventRule{nullptr}; // only present if different from rule
    Flags flags{};
    std::string eventName; // only present if different from name

    bool operator<(const Element & right) const { return name < right.name; }
};
std::ostream & operator<<(std::ostream & os, const Element & elem);

struct ElementDone : Element {
    ElementDone();
    static ElementDone s_elem;
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
    const char * optionString(
        std::string_view name, 
        const char * def = ""
    ) const;
    unsigned optionUnsigned(std::string_view name, unsigned def = 0) const;
    const char * operator[](std::string_view name) const;

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

    const Element * eventAlways(std::string_view name);

    Element * element(std::string_view name);
    const Element * element(std::string_view name) const;

    const std::set<Element> & rules() const { return m_rules; }
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
    const Grammar & src,
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

const char kLeftQ[] = ": ";
const char kRightQ[] = " :";

const char kRootStateName[] = "<ROOT>";
const char kDoneStateName[] = "<DONE>";
const char kFailedStateName[] = "<FAILED>";

struct StateElement {
    const Element * elem;
    unsigned rep;
    bool started{false};
    bool recurse{false};

    int compare(const StateElement & right) const;
    bool operator<(const StateElement & right) const;
    bool operator==(const StateElement & right) const;

    bool operator!=(const StateElement & right) const {
        return !operator==(right);
    }
};

struct StateEvent {
    const Element * elem{nullptr};
    Element::Flags flags{};
    unsigned distance{0};

    int compare(const StateEvent & right) const;
    bool operator<(const StateEvent & right) const;
    bool operator==(const StateEvent & right) const;

    bool operator!=(const StateEvent & right) const {
        return !operator==(right);
    }
};

struct StatePosition {
    std::vector<StateElement> elems;
    std::vector<StateEvent> events;
    std::vector<StateEvent> delayedEvents;
    int recurseSe{0}; // state element of recursion entry point (0 for none)
    int recursePos{-1}; // position in kSequence of recursion entry point

    int compare(const StatePosition & right) const;
    bool operator<(const StatePosition & right) const;
    bool operator==(const StatePosition & right) const;
};

struct State {
    unsigned id;
    std::string name;
    std::vector<std::string> aliases;

    // bitset is 256 for the possible terminals, count() == 0 if
    // there are no terminals because it's a function.
    std::map<StatePosition, std::bitset<256>> positions;

    std::vector<unsigned> next;

    void clear();
    bool operator==(const State & right) const;
};

namespace std {
template <> struct hash<StateElement> {
    size_t operator()(const StateElement & val) const;
};
template <> struct hash<StateEvent> {
    size_t operator()(const StateEvent & val) const;
};
template <> struct hash<StatePosition> {
    size_t operator()(const StatePosition & val) const;
};
template <> struct hash<State> { size_t operator()(const State & val) const; };
} // namespace std


// build state tree
void buildStateTree(
    std::unordered_set<State> * states,
    const Element & root,
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
    const Grammar & rules,
    const RunOptions & opts
);

void writeRule(
    std::ostream & os,
    const Element & rule,
    size_t maxWidth,
    std::string_view prefix
);
