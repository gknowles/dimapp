// Copyright Glen Knowles 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// globint.h - dim file
#pragma once

#include <bitset>

namespace Dim::Glob {


/****************************************************************************
*
*   Declarations
*
***/

enum GlobType {
    kInvalid,
    kRuby,
};

enum PathType {
    kExact,         // Literal
    kCondition,     // Char choice, string choice, or embedded blot.
    kAny,           // Can be any value.
    kDynamicAny,    // Matches zero or more segments of any value.
    kEndMark,       // End of matching path, indicating the next segment is
                    //  optional.
};
enum MatchResult {
    kNoMatch = 0,
    kMatch = 1,

    // Matches this segment and also any number of following segments.
    kMatchRest = 2,
};
enum NodeType {
    kPathSeg,
    kPartEmpty,
    kPartLiteral,
    kPartBlot,
    kPartDoubleBlot,
    kPartCharChoice,
    kPartChoice,
    kPartList,
};

struct Node : Dim::ListLink<> {
    NodeType type;
};

struct PathSegment {
    union {
        // for kExact and kCondition, prefix enforced by condition
        std::string_view prefix;

        // for kDynamicAny, segments spanned in current permutation
        unsigned count;
    };
    PathType type = kExact;
    const Node * node = {};

    PathSegment() { prefix = {}; }
};

struct Info {
    GlobType type = kInvalid;
    char * text = {}; // normalized query string
    List<Node> roots;
    PathType pathType = kExact;
    TempHeap heap;

    ~Info();
};


/****************************************************************************
*
*   Querying abstract syntax tree
*
***/

// Returns an entry for each segment of path.
std::span<PathSegment> pathSegments(const Info & glob);
// Use the node values returned by pathSegments()
MatchResult matchSegment(const Node & node, std::string_view val);

NodeType getType(const Node & node);

std::string toString(const Node & node, GlobType type = kRuby);


/****************************************************************************
*
*   Abstract syntax tree builders
*
***/

Node * addRootSeg(Info * glob);
Node * addSeg(Info * glob, Node * node);
void endSeg(Info * glob, Node * node);
Node * addPartEmpty(Info * glob, Node * seg);
Node * addPartLiteral(Info * glob, Node * seg, std::string_view val);
Node * addPartBlot(Info * glob, Node * seg);
Node * addPartCharChoice(Info * glob, Node * seg, std::bitset<256> && vals);
Node * addPartChoice(Info * glob, Node * seg);


/****************************************************************************
*
*   Parsing
*
***/

// Parses src into glob struct, logs error and returns false on parsing
// failures.
bool parse(Info * glob, std::string_view src, GlobType type = kRuby);
bool parse(
    Info * glob,
    const std::vector<std::string> & src,
    GlobType type = kRuby
);

std::string toString(const Info & glob);

} // namespace
