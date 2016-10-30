# pargen
Simplistic parser generator

Generates high performance text parsers from tagged ABNF. The state machine 
is encoded as raw C++ switch statements, think of it as loop unrolling taken 
to an extreme.

For an example of what this looks like, see the abnfparse.cpp & .h files in 
this project, which were created from abnf.abnf.

The input format follows RFC 5234 (including the RFC 7405 update), with 
the some modifications. 
- Special "rules" starting with a percent "%" are options for the generator
- Rules can be followed by brace "{}" enclosed tags, tags are used by the
  generator
- Prose values containing the rule as a text description such as "\<all nouns>"

The "Core Rules" defined in RFC 5234 ("ALPHA", "BIT", etc) are always 
included.

## Options

| Name | Default | Description |
|------|---------|-------------|
| %root | - | Name of the top level rule that matches the input to be parsed |
| %api.prefix | - | Used in default definitions of other options |
| %api.file.h | lowercase(%api.prefix) + "parse.h" | Header file to generate |
| %api.file.cpp | lowercase(%api.prefix) + "parse.cpp" | C++ file to generate |
| %api.parser.className | %api.prefix + "Parser" | Name to give parser class |
| %api.notify.className | "I" + %api.prefix + "ParserNotify" | Name to give notify class |
| %api.namespace | "" (global) | Namespace to contain parser classes |

All values must be defined either directly or indirectly via their defaults.

## Tags

Triggering callbacks during parsing:
- start
- end
- char

Change the name of the callback invoked
- as = \<rulename>

Root of separate function that can be recursively called
- function
