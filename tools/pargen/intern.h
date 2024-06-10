// Copyright Glen Knowles 2016 - 2024.
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
const char kOptionInclude[] = "%include";
const char kOptionApiPrefix[] = "%api.prefix";
const char kOptionApiOutputHeader[] = "%api.output.file.h";
const char kOptionApiOutputCpp[] = "%api.output.file.cpp";
const char kOptionApiOutputClass[] = "%api.output.className";
const char kOptionApiBaseHeader[] = "%api.base.file.h";
const char kOptionApiBaseClass[] = "%api.base.className";
const char kOptionApiNamespace[] = "%api.namespace";

const char kDoneRuleName[] = "%%DONE";

const unsigned kUnlimited = unsigned(-1);

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
    const Element * rule{};
    const Element * eventRule{}; // only present if different from rule
    Dim::EnumFlags<Flags> flags{};
    std::string eventName; // only present if different from name

    auto operator<=>(const Element & right) const {
        if (auto cmp = name.compare(right.name)) {
            return cmp < 0
                ? std::strong_ordering::less
                : std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

private:
    friend std::ostream & operator<<(std::ostream & os, const Element & elem);
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
        Dim::EnumFlags<Element::Flags> flags = {}
    );
    Element * addChoiceRule(
        std::string_view name,
        unsigned m,
        unsigned n,
        Dim::EnumFlags<Element::Flags> flags = {}
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
    const Element * elem{};
    unsigned rep{};
    bool started{};
    bool recurse{};

    std::strong_ordering operator<=>(const StateElement & right) const;
    bool operator==(const StateElement & right) const;
};

struct StateEvent {
    const Element * elem{};
    Dim::EnumFlags<Element::Flags> flags{};
    int distance{};

    std::strong_ordering operator<=>(const StateEvent & right) const;
    bool operator==(const StateEvent & right) const;
};

struct StatePosition {
    std::vector<StateElement> elems;
    std::vector<StateEvent> events;
    std::vector<StateEvent> delayedEvents;
    int recurseSe{}; // state element of recursion entry point (0 for none)
    int recursePos{-1}; // position in kSequence of recursion entry point

    std::strong_ordering operator<=>(const StatePosition & right) const;
    bool operator==(const StatePosition & right) const;
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
    bool operator==(const State & right) const;
};

//===========================================================================
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
template <> struct hash<State> {
    size_t operator()(const State & val) const;
};

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
