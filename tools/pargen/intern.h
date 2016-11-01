// intern.h - pargen


/****************************************************************************
*
*   Grammar rules
*
***/

const char kOptionRoot[] = "%root";
const char kOptionApiPrefix[] = "%api.prefix";
const char kOptionApiHeaderFile[] = "%api.file.h";
const char kOptionApiCppFile[] = "%api.file.cpp";
const char kOptionApiParserClass[] = "%api.parser.className";
const char kOptionApiNotifyClass[] = "%api.notify.className";
const char kOptionApiNamespace[] = "%api.namespace";

const char kDoneRuleName[] = "%%DONE";

const unsigned kUnlimited = unsigned(-1);

struct Element {
    enum Flags : uint8_t {
        kOnStart = 1,
        kOnChar = 2,
        kOnEnd = 4,
    };

    enum Type : uint8_t {
        kRule,
        kSequence,
        kChoice,
        kTerminal,
    } type{kRule};
    std::string name;
    std::string eventName; // only present if different from name
    unsigned m{1};
    unsigned n{1};
    unsigned id{0};
    unsigned pos{0}; // position in sequence, 0 for non-sequences

    std::vector<Element> elements;
    std::string value;
    const Element * rule{nullptr};
    const Element * eventRule{nullptr}; // only present if different from rule
    unsigned flags{0};
    bool recurse{false};

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

    void addOption(const std::string & name, const std::string & value);
    const char *
    optionString(const std::string & name, const char * def = "") const;
    unsigned optionUnsigned(const std::string & name, unsigned def = 0) const;
    const char * operator[](const std::string & name) const;

    Element * addSequenceRule(
        const std::string & name,
        unsigned m,
        unsigned n,
        unsigned flags = 0, // Element::kOn*
        bool recurse = false);
    Element * addChoiceRule(
        const std::string & name,
        unsigned m,
        unsigned n,
        unsigned flags = 0, // Element::kOn*
        bool recurse = false);
    Element * addSequence(Element * rule, unsigned m, unsigned n);
    Element * addChoice(Element * rule, unsigned m, unsigned n);
    void
    addRule(Element * rule, const std::string & name, unsigned m, unsigned n);

    // case insensitive
    void
    addText(Element * rule, const std::string & value, unsigned m, unsigned n);

    void addLiteral(
        Element * rule, const std::string & value, unsigned m, unsigned n);
    void addRange(Element * rule, unsigned char a, unsigned char b);
    void addTerminal(Element * rule, unsigned char ch, unsigned m, unsigned n);

    Element * element(const std::string & name);
    const Element * element(const std::string & name) const;

    const std::set<Element> & rules() const { return m_rules; }

    size_t errpos() const { return m_errpos; }
    void errpos(size_t where) { m_errpos = where; }

private:
    Element * addElement(Element * rule, unsigned m, unsigned n);

    unsigned m_nextElemId{0};
    std::set<Element> m_rules;
    size_t m_errpos{0};
};

// modify options
bool processOptions(Grammar & rules);

// modify rules
bool copyRules(
    Grammar & out,
    const Grammar & src,
    const std::string & root,
    bool failIfExists);
void normalize(Grammar & rules);
void markRecursion(Grammar & rules, Element & rule, bool resetFirst);


/****************************************************************************
*
*   Parse state
*
***/

const char kRootStateName[] = "<ROOT>";
const char kDoneStateName[] = "<DONE>";
const char kFailedStateName[] = "<FAILED>";

struct StateElement {
    const Element * elem;
    unsigned rep;
    bool started{false};

    int compare(const StateElement & right) const;
    bool operator<(const StateElement & right) const;
    bool operator==(const StateElement & right) const;

    bool operator!=(const StateElement & right) const {
        return !operator==(right);
    }
};

struct StateEvent {
    const Element * elem;
    unsigned flags{0}; // Element::kOn*

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
    bool recurse{false};

    bool operator<(const StatePosition & right) const;
    bool operator==(const StatePosition & right) const;
};

struct State {
    unsigned id;
    std::string name;
    std::vector<std::string> aliases;
    std::set<StatePosition> positions;
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
    bool dedupStates);
void dedupStateTree(std::unordered_set<State> & states);


/****************************************************************************
*
*   Public API
*
***/

struct RunOptions {
    int markRecursion;
    bool includeCallbacks;
    bool buildStateTree;
    bool dedupStateTree;
    bool writeStatePositions;
    bool writeFunctions;
};

// write generated code
void writeParser(
    std::ostream & hfile,
    std::ostream & cppfile,
    const Grammar & rules,
    const RunOptions & opts);

bool parseAbnf(Grammar & rules, const std::string & src);
void writeRule(
    std::ostream & os,
    const Element & rule,
    size_t maxWidth,
    const std::string & prefix);
