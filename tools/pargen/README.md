# pargen
Simplistic parser generator

Generates high performance text parsers from tagged ABNF. The state machine 
is encoded as raw C++ switch statements, think of it as loop unrolling taken 
to an extreme.

For an example of what this looks like, see the abnfparser.cpp & .h files in 
this project, which were created by this generator.
