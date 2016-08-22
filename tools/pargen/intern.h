// intern.h - pargen


/****************************************************************************
*
*   Grammar rules
*
***/

const unsigned kUnlimited = unsigned(-1);

struct Element {
    enum Flags : uint8_t {
        kOnStart = 1,
        kOnChar = 2,
        kOnEnd = 4,
    };

    enum Type : uint8_t {
        kSequence,
        kRule,
        kChoice,
        kTerminal,
    } type;
    std::string name;
    unsigned m{1};
    unsigned n{1};
    unsigned id{0};

    std::vector<Element> elements;
    std::string value;
    const Element * rule{nullptr};
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

// create rules
Element * addSequenceRule(
    std::set<Element> & rules,
    const std::string & name,
    unsigned m,
    unsigned n,
    unsigned flags = 0 // Element::kOn*
    );
Element * addChoiceRule(
    std::set<Element> & rules,
    const std::string & name,
    unsigned m,
    unsigned n,
    unsigned flags = 0 // Element::kOn*
    );
Element * addSequence(Element * rule, unsigned m, unsigned n);
Element * addChoice(Element * rule, unsigned m, unsigned n);
void addRule(Element * rule, const std::string & name, unsigned m, unsigned n);
void addLiteral(
    Element * rule, const std::string & value, unsigned m, unsigned n);
void addRange(Element * rule, unsigned char a, unsigned char b);
void addTerminal(Element * rule, unsigned char ch, unsigned m, unsigned n);

// modify rules
bool copyRules(
    std::set<Element> & rules,
    const std::set<Element> & src,
    const std::string & root,
    bool failIfExists);
void normalize(std::set<Element> & rules);
void markRecursion(std::set<Element> & rules, Element & rule, bool resetFirst);


/****************************************************************************
*
*   Parse state
*
***/

const char kRootElement[] = "<ROOT>";
const char kDoneElement[] = "<DONE>";
const char kFailedElement[] = "<FAILED>";
const char kNullElement[] = "<NULL>";

struct StateElement {
    const Element * elem;
    unsigned rep;
    bool started{false};

    int compare(const StateElement & right) const;
    bool operator<(const StateElement & right) const;
    bool operator==(const StateElement & right) const;
};

struct StateEvent {
    const Element * elem;
    unsigned flags{0}; // Element::kOn*

    int compare(const StateEvent & right) const;
    bool operator<(const StateEvent & right) const;
    bool operator==(const StateEvent & right) const;
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
    const std::set<Element> & rules,
    const std::string & root,
    bool inclDeps);


/****************************************************************************
*
*   Public API
*
***/

// write generated code
void writeParser(
    std::ostream & hfile,
    std::ostream & cppfile,
    const std::set<Element> & rules,
    const std::string & root);
