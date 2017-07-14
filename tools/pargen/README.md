# pargen
Simplistic parser generator

Generates high performance text parsers from tagged ABNF. The state machine 
is encoded as raw C++ switch statements, think of it as loop unrolling taken 
to an extreme.

For an example of what this looks like, see the abnfparse.cpp & .h files in 
this project, which were created from abnf.abnf.

The input format follows RFC 5234 (including the RFC 7405 update), with 
the some modifications. 
- Special "rules" starting with a percent "%" are options for the generator.
- Rules can be followed by brace "{}" enclosed tags. Tags are used to tell 
  the generator to do things, just as insert event callbacks.
- Prose values containing the rule as a text description such as "\<all nouns>"
  are not allowed.

The "Core Rules" defined in RFC 5234 ("ALPHA", "BIT", etc) are always 
included.

## Options

| Name | Default | Description |
|------|---------|-------------|
| %root | - | Name of the top level rule that matches the input to be parsed |
| %api.prefix | - | Used in default definitions of other options |
| %api.parser.file.h | lowercase(%api.prefix) + "parse.h" | Header file to generate |
| %api.parser.file.cpp | lowercase(%api.prefix) + "parse.cpp" | C++ file to generate |
| %api.parser.className | %api.prefix + "Parser" | Name to give parser class |
| %api.base.file.h | lowercase(%api.prefix) + "parsebase.h" | Header defining base class |
| %api.base.className | "I" + %api.prefix + "ParserNotify" | Name to give base class |
| %api.namespace | "" (global) | Namespace to contain parser classes |

All values must be defined either directly or indirectly via their defaults.

## Tags

Triggering callbacks during parsing:
- Start
- End
- Char

Change the name of the callback invoked
- As \<rulename>

Root of separate function that can be recursively called
- Function

Only include rule if corresponding --min-rules or --no-min-rules is explicitly 
enabled. This provides a way to have a small set of states to reduce the 
clutter during development, especially with --state-detail.
- MinRules
- NoMinRules
