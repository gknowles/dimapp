// intern.h - pargen


struct Element {
    std::string name;
    Element * parent{nullptr};

    unsigned m{1};
    unsigned n{1};

    enum Type {
        kSequence,
        kRule,
        kChoice,
        kTerminal,
    } type;
    std::vector<Element> elements;
    std::string value;

    bool operator< (const Element & right) const {
        return name < right.name;
    }
};

const unsigned kUnlimited = unsigned(-1);

Element * addSequenceRule (
    std::set<Element> & rules, 
    const std::string & name,
    unsigned m,
    unsigned n
);
Element * addChoiceRule (
    std::set<Element> & rules, 
    const std::string & name,
    unsigned m,
    unsigned n
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


void writeParser (
    std::ostream & os, 
    const std::set<Element> & rules,
    const std::string & root
);
