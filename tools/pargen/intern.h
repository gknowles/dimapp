// intern.h - pargen

struct Element {
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
    bool recurse{false};

    bool operator< (const Element & right) const {
        return name < right.name;
    }
};
std::ostream & operator<< (std::ostream & os, const Element & elem);

const unsigned kUnlimited = unsigned(-1);

Element * addSequenceRule (
    std::set<Element> & rules, 
    const std::string & name,
    unsigned m,
    unsigned n,
    bool recurse = false
);
Element * addChoiceRule (
    std::set<Element> & rules, 
    const std::string & name,
    unsigned m,
    unsigned n,
    bool recurse = false
);
Element * addSequence (Element * rule, unsigned m, unsigned n);
Element * addChoice (Element * rule, unsigned m, unsigned n);
void addRule (
    Element * rule, 
    const std::string & name, 
    unsigned m, 
    unsigned n
);
void addLiteral (
    Element * rule, 
    const std::string & value,
    unsigned m,
    unsigned n
);
void addRange (Element * rule, unsigned char a, unsigned char b);
void addTerminal (Element * rule, unsigned char ch, unsigned m, unsigned n);

void writeParser (
    std::ostream & os, 
    const std::set<Element> & rules,
    const std::string & root
);
