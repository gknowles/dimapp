// abnfparse.cpp - pargen
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   AbnfParser
*
*   Normalized ABNF of syntax (recursive rules are marked with asterisks):
*   <ROOT> = rulelist 
*   ALPHA = ( %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / 
*       %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / 
*       %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x61 / %x62 / %x63 / 
*       %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / 
*       %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / 
*       %x78 / %x79 / %x7a ) 
*   BIT = ( %x30 / %x31 ) 
*   CR = %xd 
*   CRLF = ( CR LF ) 
*   DIGIT = ( %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / 
*       %x39 ) 
*   DQUOTE = %x22 
*   HEXDIG = ( DIGIT / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x41 / %x42 / 
*       %x43 / %x44 / %x45 / %x46 ) 
*   HTAB = %x9 
*   LF = %xa 
*   NEWLINE = ( LF / CRLF ) 
*   SP = %x20 
*   VCHAR = ( %x21 / %x22 / %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / 
*       %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / 
*       %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / 
*       %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / 
*       %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / 
*       %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / 
*       %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / 
*       %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / 
*       %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / 
*       %x7a / %x7b / %x7c / %x7d / %x7e ) 
*   WSP = ( SP / HTAB ) 
*   action = ( action-start / action-end / action-char ) 
*   action-char = ( ( %x63 / %x43 ) ( %x68 / %x48 ) ( %x61 / %x41 ) ( %x72 / 
*       %x52 ) ) { End } 
*   action-end = ( ( %x65 / %x45 ) ( %x6e / %x4e ) ( %x64 / %x44 ) ) { End } 
*   action-start = ( ( %x73 / %x53 ) ( %x74 / %x54 ) ( %x61 / %x41 ) ( %x72 / 
*       %x52 ) ( %x74 / %x54 ) ) { End } 
*   actions = ( %x7b *c-wsp *1( action *( *c-wsp %x2c *c-wsp action ) ) *c-wsp 
*       %x7d ) 
*   alternation = ( concatenation *( *c-wsp %x2f *c-wsp concatenation ) ) { 
*       Start, End } 
*   bin-val = ( ( %x62 / %x42 ) ( bin-val-simple / bin-val-concatenation / 
*       bin-val-alternation ) ) 
*   bin-val-alt-first = bin-val-sequence { End } 
*   bin-val-alt-second = bin-val-sequence { End } 
*   bin-val-alternation = ( bin-val-alt-first %x2d bin-val-alt-second ) 
*   bin-val-concat-each = bin-val-sequence { End } 
*   bin-val-concatenation = ( bin-val-concat-each 1*( %x2e bin-val-concat-each 
*       ) ) { Start, End } 
*   bin-val-sequence = 1*BIT { Char } 
*   bin-val-simple = bin-val-sequence { End } 
*   c-nl = ( comment / NEWLINE ) 
*   c-wsp = ( WSP / ( c-nl WSP ) ) 
*   char-val = ( DQUOTE char-val-sequence DQUOTE ) { End } 
*   char-val-sequence = *( %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / 
*       %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / 
*       %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / 
*       %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / 
*       %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / 
*       %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / 
*       %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / 
*       %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / 
*       %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / 
*       %x7a / %x7b / %x7c / %x7d / %x7e ) { Start, Char } 
*   comment = ( %x3b *( WSP / VCHAR ) NEWLINE ) 
*   concatenation = ( repetition *( 1*c-wsp repetition ) ) { Start, End } 
*   dec-val = ( ( %x64 / %x44 ) ( dec-val-simple / dec-val-concatenation / 
*       dec-val-alternation ) ) 
*   dec-val-alt-first = dec-val-sequence { End } 
*   dec-val-alt-second = dec-val-sequence { End } 
*   dec-val-alternation = ( dec-val-alt-first %x2d dec-val-alt-second ) 
*   dec-val-concat-each = dec-val-sequence { End } 
*   dec-val-concatenation = ( dec-val-concat-each 1*( %x2e dec-val-concat-each 
*       ) ) { Start, End } 
*   dec-val-sequence = 1*DIGIT { Char } 
*   dec-val-simple = dec-val-sequence { End } 
*   defined-as = ( *c-wsp ( defined-as-set / defined-as-incremental ) *c-wsp ) 
*   defined-as-incremental = ( %x3d %x2f ) { End } 
*   defined-as-set = %x3d { End } 
*   element = ( ruleref / group / char-val / num-val ) 
*   elements = ( alternation *c-wsp ) 
*   group = ( ( %x28 / %x5b ) group-tail ) { Start, End } 
*   group-tail* = ( *c-wsp alternation *c-wsp ( %x29 / %x5d ) ) 
*   hex-val = ( ( %x78 / %x58 ) ( hex-val-simple / hex-val-concatenation / 
*       hex-val-alternation ) ) 
*   hex-val-alt-first = hex-val-sequence { End } 
*   hex-val-alt-second = hex-val-sequence { End } 
*   hex-val-alternation = ( hex-val-alt-first %x2d hex-val-alt-second ) 
*   hex-val-concat-each = hex-val-sequence { End } 
*   hex-val-concatenation = ( hex-val-concat-each 1*( %x2e hex-val-concat-each 
*       ) ) { Start, End } 
*   hex-val-sequence = 1*HEXDIG { Char } 
*   hex-val-simple = hex-val-sequence { End } 
*   num-val = ( %x25 ( bin-val / dec-val / hex-val ) ) 
*   repeat = ( repeat-minmax / repeat-range ) 
*   repeat-max = dec-val-sequence { End } 
*   repeat-minmax = dec-val-sequence { End } 
*   repeat-range = ( *1dec-val-sequence %x2a *1repeat-max ) { Start } 
*   repetition = ( *1repeat element ) { Start } 
*   rule = ( rulename defined-as elements *1actions c-nl ) { End } 
*   rulelist = 1*( rule / ( *c-wsp c-nl ) ) 
*   rulename = ( ALPHA *( ALPHA / DIGIT / %x2d ) ) { Start, Char } 
*   ruleref = rulename { End } 
*
***/

//===========================================================================
bool AbnfParser::parse (const char src[]) {
    const char * ptr = src;
    char ch;
    goto state2;

state0: // <FAILED>
    return false;

state1: // <DONE>
    return true;

state2: // 
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state3;
    case 10: 
        goto state4;
    case 13: 
        goto state445;
    case ';': 
        goto state447;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state451;
    }
    goto state0;

state3: // \t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state3;
    case 10: 
        goto state4;
    case 13: 
        goto state445;
    case ';': 
        goto state447;
    }
    goto state0;

state4: // \t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state5: // \t\n\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state439;
    case ';': 
        goto state441;
    }
    goto state0;

state6: // \t\n\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state7: // \t\n\t\n\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state8: // \t\n\t\n\n\t
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    }
    goto state0;

state9: // \t\n\t\n\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state10;
    }
    goto state0;

state10: // \t\n\t\n\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state11: // \t\n\t\n\n\t\r\n;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state11;
    case 10: 
        goto state12;
    case 13: 
        goto state36;
    }
    goto state0;

state12: // \t\n\t\n\n\t\r\n;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state13: // \t\n\t\n\n\t\r\n;\nA
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state14;
    case 10: 
        goto state15;
    case 13: 
        goto state16;
    case ';': 
        goto state17;
    case '=': 
        goto state19;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state39;
    }
    goto state0;

state14: // \t\n\t\n\n\t\r\n;\nA\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state14;
    case 10: 
        goto state15;
    case 13: 
        goto state16;
    case ';': 
        goto state17;
    case '=': 
        goto state19;
    }
    goto state0;

state15: // \t\n\t\n\n\t\r\n;\nA\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state14;
    }
    goto state0;

state16: // \t\n\t\n\n\t\r\n;\nA\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state15;
    }
    goto state0;

state17: // \t\n\t\n\n\t\r\n;\nA\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state15;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state17;
    case 13: 
        goto state18;
    }
    goto state0;

state18: // \t\n\t\n\n\t\r\n;\nA\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state15;
    }
    goto state0;

state19: // \t\n\t\n\n\t\r\n;\nA\t=
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state20;
    case 10: 
        goto state429;
    case 13: 
        goto state430;
    case '"': 
        goto state431;
    case '%': 
        goto state432;
    case '(': case '[': 
        goto state433;
    case '*': 
        goto state434;
    case '/': 
        goto state435;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state436;
    case ';': 
        goto state437;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state438;
    }
    goto state0;

state20: // \t\n\t\n\n\t\r\n;\nA\t=\t
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state21;
    case 10: 
        goto state22;
    case 13: 
        goto state23;
    case '"': 
        goto state24;
    case '%': 
        goto state382;
    case '(': case '[': 
        goto state407;
    case '*': 
        goto state408;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state419;
    case ';': 
        goto state426;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state428;
    }
    goto state0;

state21: // \t\n\t\n\n\t\r\n;\nA\t=\t\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state21;
    case 10: 
        goto state22;
    case 13: 
        goto state23;
    case '"': 
        goto state24;
    case '%': 
        goto state382;
    case '(': case '[': 
        goto state407;
    case '*': 
        goto state408;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state419;
    case ';': 
        goto state426;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state428;
    }
    goto state0;

state22: // \t\n\t\n\n\t\r\n;\nA\t=\t\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state21;
    }
    goto state0;

state23: // \t\n\t\n\n\t\r\n;\nA\t=\t\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state22;
    }
    goto state0;

state24: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state25;
    }
    goto state0;

state25: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state26;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state381;
    }
    goto state0;

state26: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state53;
    case 13: 
        goto state54;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case ';': 
        goto state289;
    }
    goto state0;

state27: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state28: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state29: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\0
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    goto state1;

state30: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state40;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    case ';': 
        goto state377;
    }
    goto state0;

state31: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state32: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n\0
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    goto state1;

state33: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n\n
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state34: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n\r
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state10;
    }
    goto state0;

state35: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n;
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state11;
    case 10: 
        goto state12;
    case 13: 
        goto state36;
    }
    goto state0;

state36: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state37;
    }
    goto state0;

state37: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\n;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state38: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\nA
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    m_notify->onRuleEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state14;
    case 10: 
        goto state15;
    case 13: 
        goto state16;
    case ';': 
        goto state17;
    case '=': 
        goto state19;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state39;
    }
    goto state0;

state39: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\nA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state14;
    case 10: 
        goto state15;
    case 13: 
        goto state16;
    case ';': 
        goto state17;
    case '=': 
        goto state19;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state39;
    }
    goto state0;

state40: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state41;
    }
    goto state0;

state41: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state42: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state43;
    }
    goto state0;

state43: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state26;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state44;
    }
    goto state0;

state44: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state26;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state44;
    }
    goto state0;

state45: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state46;
    case 'D': case 'd': 
        goto state296;
    case 'X': case 'x': 
        goto state302;
    }
    goto state0;

state46: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state47;
    }
    goto state0;

state47: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state47;
    case 9: case ' ': 
        goto state48;
    case '/': 
        goto state252;
    case '{': 
        goto state254;
    case 10: 
        goto state360;
    case 13: 
        goto state361;
    case '-': 
        goto state362;
    case '.': 
        goto state368;
    case ';': 
        goto state376;
    }
    goto state0;

state48: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state49: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state50;
    }
    goto state0;

state50: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state51: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state52: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state53;
    case 13: 
        goto state54;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case ';': 
        goto state289;
    }
    goto state0;

state53: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state54: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state55: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state56: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state57: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state58: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    }
    goto state0;

state59: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state58;
    }
    goto state0;

state60: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state61;
    }
    goto state0;

state61: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state62;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state288;
    }
    goto state0;

state62: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state80;
    case 13: 
        goto state81;
    case ';': 
        goto state83;
    case '{': 
        goto state87;
    }
    goto state0;

state63: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state64: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state65: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state65;
    case 10: 
        goto state66;
    case 13: 
        goto state67;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    case ';': 
        goto state284;
    }
    goto state0;

state66: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state67: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state68;
    }
    goto state0;

state68: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state69: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state70;
    }
    goto state0;

state70: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state62;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state71;
    }
    goto state0;

state71: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state62;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state71;
    }
    goto state0;

state72: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state73;
    case 'D': case 'd': 
        goto state136;
    case 'X': case 'x': 
        goto state142;
    }
    goto state0;

state73: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state74;
    }
    goto state0;

state74: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state74;
    case 9: case ' ': 
        goto state75;
    case 10: 
        goto state158;
    case 13: 
        goto state159;
    case '/': 
        goto state252;
    case ';': 
        goto state253;
    case '{': 
        goto state254;
    case '-': 
        goto state278;
    case '.': 
        goto state280;
    }
    goto state0;

state75: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state76: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state77;
    }
    goto state0;

state77: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state78: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state79: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state80;
    case 13: 
        goto state81;
    case ';': 
        goto state83;
    case '{': 
        goto state87;
    }
    goto state0;

state80: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state81: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state82: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state83: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state84: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state85: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state86;
    }
    goto state0;

state86: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state87: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state88: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state89: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    }
    goto state0;

state90: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state89;
    }
    goto state0;

state91: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state89;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state91;
    case 13: 
        goto state92;
    }
    goto state0;

state92: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state89;
    }
    goto state0;

state93: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tC
    ch = *ptr++;
    switch (ch) {
    case 'H': case 'h': 
        goto state94;
    }
    goto state0;

state94: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCH
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state95;
    }
    goto state0;

state95: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state96;
    }
    goto state0;

state96: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR
    m_notify->onActionCharEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state97;
    case 10: 
        goto state98;
    case 13: 
        goto state99;
    case ',': 
        goto state100;
    case ';': 
        goto state111;
    case '}': 
        goto state113;
    }
    goto state0;

state97: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state97;
    case 10: 
        goto state98;
    case 13: 
        goto state99;
    case ',': 
        goto state100;
    case ';': 
        goto state111;
    case '}': 
        goto state113;
    }
    goto state0;

state98: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state97;
    }
    goto state0;

state99: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state98;
    }
    goto state0;

state100: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state100;
    case 10: 
        goto state101;
    case 13: 
        goto state102;
    case ';': 
        goto state103;
    case 'C': case 'c': 
        goto state105;
    case 'E': case 'e': 
        goto state108;
    case 'S': case 's': 
        goto state122;
    }
    goto state0;

state101: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state100;
    }
    goto state0;

state102: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state101;
    }
    goto state0;

state103: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state101;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state103;
    case 13: 
        goto state104;
    }
    goto state0;

state104: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state101;
    }
    goto state0;

state105: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,C
    ch = *ptr++;
    switch (ch) {
    case 'H': case 'h': 
        goto state106;
    }
    goto state0;

state106: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,CH
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state107;
    }
    goto state0;

state107: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,CHA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state96;
    }
    goto state0;

state108: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,E
    ch = *ptr++;
    switch (ch) {
    case 'N': case 'n': 
        goto state109;
    }
    goto state0;

state109: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,EN
    ch = *ptr++;
    switch (ch) {
    case 'D': case 'd': 
        goto state110;
    }
    goto state0;

state110: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END
    m_notify->onActionEndEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state97;
    case 10: 
        goto state98;
    case 13: 
        goto state99;
    case ',': 
        goto state100;
    case ';': 
        goto state111;
    case '}': 
        goto state113;
    }
    goto state0;

state111: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state98;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state111;
    case 13: 
        goto state112;
    }
    goto state0;

state112: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state98;
    }
    goto state0;

state113: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state114;
    case 13: 
        goto state116;
    case ';': 
        goto state118;
    }
    goto state0;

state114: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}\n
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    case 0: 
        goto state115;
    }
    goto state0;

state115: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}\n\0
    m_notify->onRuleEnd(ptr);
    goto state1;

state116: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state117;
    }
    goto state0;

state117: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}\r\n
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    case 0: 
        goto state115;
    }
    goto state0;

state118: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state118;
    case 10: 
        goto state119;
    case 13: 
        goto state120;
    }
    goto state0;

state119: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};\n
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    case 0: 
        goto state115;
    }
    goto state0;

state120: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state121;
    }
    goto state0;

state121: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};\r\n
    m_notify->onRuleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    case 0: 
        goto state115;
    }
    goto state0;

state122: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,S
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state123;
    }
    goto state0;

state123: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,ST
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state124;
    }
    goto state0;

state124: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,STA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state125;
    }
    goto state0;

state125: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,STAR
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state126;
    }
    goto state0;

state126: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,START
    m_notify->onActionStartEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state97;
    case 10: 
        goto state98;
    case 13: 
        goto state99;
    case ',': 
        goto state100;
    case ';': 
        goto state111;
    case '}': 
        goto state113;
    }
    goto state0;

state127: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tE
    ch = *ptr++;
    switch (ch) {
    case 'N': case 'n': 
        goto state128;
    }
    goto state0;

state128: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tEN
    ch = *ptr++;
    switch (ch) {
    case 'D': case 'd': 
        goto state110;
    }
    goto state0;

state129: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tS
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state130;
    }
    goto state0;

state130: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tST
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state131;
    }
    goto state0;

state131: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tSTA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state132;
    }
    goto state0;

state132: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tSTAR
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state126;
    }
    goto state0;

state133: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state134;
    case '%': 
        goto state135;
    case '(': case '[': 
        goto state263;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state264;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state269;
    }
    goto state0;

state134: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state70;
    }
    goto state0;

state135: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state73;
    case 'D': case 'd': 
        goto state136;
    case 'X': case 'x': 
        goto state142;
    }
    goto state0;

state136: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state137;
    }
    goto state0;

state137: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state137;
    case 9: case ' ': 
        goto state138;
    case 10: 
        goto state172;
    case 13: 
        goto state173;
    case '/': 
        goto state237;
    case ';': 
        goto state238;
    case '{': 
        goto state239;
    case '-': 
        goto state272;
    case '.': 
        goto state274;
    }
    goto state0;

state138: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state139: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state140;
    case '%': 
        goto state141;
    case '(': case '[': 
        goto state261;
    case '*': 
        goto state262;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state270;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state271;
    }
    goto state0;

state140: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state70;
    }
    goto state0;

state141: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state73;
    case 'D': case 'd': 
        goto state136;
    case 'X': case 'x': 
        goto state142;
    }
    goto state0;

state142: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state143;
    }
    goto state0;

state143: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state143;
    case 9: case ' ': 
        goto state144;
    case 10: 
        goto state185;
    case 13: 
        goto state186;
    case '/': 
        goto state211;
    case ';': 
        goto state212;
    case '{': 
        goto state213;
    case '-': 
        goto state255;
    case '.': 
        goto state257;
    }
    goto state0;

state144: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state145: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state145;
    case 10: 
        goto state146;
    case 13: 
        goto state147;
    }
    goto state0;

state146: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state147: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state148;
    }
    goto state0;

state148: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state149: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state153;
    case '/': 
        goto state154;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state150: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state151: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state152: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state153: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state153;
    case '/': 
        goto state154;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state154: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state155: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state156;
    case 'D': case 'd': 
        goto state170;
    case 'X': case 'x': 
        goto state183;
    }
    goto state0;

state156: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state157;
    }
    goto state0;

state157: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state75;
    case '0': case '1': 
        goto state157;
    case 10: 
        goto state158;
    case 13: 
        goto state159;
    case '-': 
        goto state160;
    case '.': 
        goto state242;
    case '/': 
        goto state252;
    case ';': 
        goto state253;
    case '{': 
        goto state254;
    }
    goto state0;

state158: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state159: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state160: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state161;
    }
    goto state0;

state161: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state161;
    case 9: case ' ': 
        goto state162;
    case 10: 
        goto state163;
    case 13: 
        goto state164;
    case '/': 
        goto state165;
    case ';': 
        goto state240;
    case '{': 
        goto state241;
    }
    goto state0;

state162: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state163: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state164: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state165: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state166: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/(
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state167: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state168;
    case '%': 
        goto state169;
    case '(': case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state217;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state222;
    }
    goto state0;

state168: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state61;
    }
    goto state0;

state169: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state156;
    case 'D': case 'd': 
        goto state170;
    case 'X': case 'x': 
        goto state183;
    }
    goto state0;

state170: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state171;
    }
    goto state0;

state171: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state138;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state171;
    case 10: 
        goto state172;
    case 13: 
        goto state173;
    case '-': 
        goto state174;
    case '.': 
        goto state227;
    case '/': 
        goto state237;
    case ';': 
        goto state238;
    case '{': 
        goto state239;
    }
    goto state0;

state172: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state173: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state174: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state175;
    }
    goto state0;

state175: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state175;
    case 9: case ' ': 
        goto state176;
    case 10: 
        goto state177;
    case 13: 
        goto state178;
    case '/': 
        goto state179;
    case ';': 
        goto state225;
    case '{': 
        goto state226;
    }
    goto state0;

state176: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state177: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state178: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state179: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state180: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state181;
    case '%': 
        goto state182;
    case '(': case '[': 
        goto state214;
    case '*': 
        goto state215;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state223;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state224;
    }
    goto state0;

state181: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state61;
    }
    goto state0;

state182: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state156;
    case 'D': case 'd': 
        goto state170;
    case 'X': case 'x': 
        goto state183;
    }
    goto state0;

state183: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state184;
    }
    goto state0;

state184: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state144;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state184;
    case 10: 
        goto state185;
    case 13: 
        goto state186;
    case '-': 
        goto state187;
    case '.': 
        goto state201;
    case '/': 
        goto state211;
    case ';': 
        goto state212;
    case '{': 
        goto state213;
    }
    goto state0;

state185: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state186: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state187: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state188;
    }
    goto state0;

state188: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state188;
    case 9: case ' ': 
        goto state189;
    case 10: 
        goto state190;
    case 13: 
        goto state191;
    case '/': 
        goto state192;
    case ';': 
        goto state199;
    case '{': 
        goto state200;
    }
    goto state0;

state189: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state190: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state191: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state192: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state193: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state58;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state193;
    case 13: 
        goto state194;
    }
    goto state0;

state194: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state58;
    }
    goto state0;

state195: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '/': 
        goto state154;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state196;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state196: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '/': 
        goto state154;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state196;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state197: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state198: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-{
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state199: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state200: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0{
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state201: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state202;
    }
    goto state0;

state202: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state202;
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state204;
    case 13: 
        goto state205;
    case '.': 
        goto state206;
    case '/': 
        goto state208;
    case ';': 
        goto state209;
    case '{': 
        goto state210;
    }
    goto state0;

state203: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state204: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state205: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state206: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state207;
    }
    goto state0;

state207: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state204;
    case 13: 
        goto state205;
    case '.': 
        goto state206;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state207;
    case '/': 
        goto state208;
    case ';': 
        goto state209;
    case '{': 
        goto state210;
    }
    goto state0;

state208: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0/
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state209: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state210: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0{
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state211: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0/
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state212: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state213: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0{
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state214: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state215: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state168;
    case '%': 
        goto state169;
    case '(': case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state217;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state222;
    }
    goto state0;

state216: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state217: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state217;
    case '"': 
        goto state218;
    case '%': 
        goto state219;
    case '(': case '[': 
        goto state220;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state221;
    }
    goto state0;

state218: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state61;
    }
    goto state0;

state219: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state156;
    case 'D': case 'd': 
        goto state170;
    case 'X': case 'x': 
        goto state183;
    }
    goto state0;

state220: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state221: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '/': 
        goto state154;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state196;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state222: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '/': 
        goto state154;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state196;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state223: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state181;
    case '%': 
        goto state182;
    case '(': case '[': 
        goto state214;
    case '*': 
        goto state215;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state223;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state224;
    }
    goto state0;

state224: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '/': 
        goto state154;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state196;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state225: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state226: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0{
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state227: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state228;
    }
    goto state0;

state228: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state228;
    case 9: case ' ': 
        goto state229;
    case 10: 
        goto state230;
    case 13: 
        goto state231;
    case '.': 
        goto state232;
    case '/': 
        goto state234;
    case ';': 
        goto state235;
    case '{': 
        goto state236;
    }
    goto state0;

state229: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state230: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state231: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state232: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state233;
    }
    goto state0;

state233: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state229;
    case 10: 
        goto state230;
    case 13: 
        goto state231;
    case '.': 
        goto state232;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state233;
    case '/': 
        goto state234;
    case ';': 
        goto state235;
    case '{': 
        goto state236;
    }
    goto state0;

state234: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0/
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state235: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state236: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0{
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state237: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0/
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state238: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state239: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0{
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state240: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state241: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0{
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state242: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state243;
    }
    goto state0;

state243: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state243;
    case 9: case ' ': 
        goto state244;
    case 10: 
        goto state245;
    case 13: 
        goto state246;
    case '.': 
        goto state247;
    case '/': 
        goto state249;
    case ';': 
        goto state250;
    case '{': 
        goto state251;
    }
    goto state0;

state244: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state56;
    case 9: case ' ': 
        goto state63;
    case 10: 
        goto state64;
    case '"': 
        goto state69;
    case '%': 
        goto state72;
    case 13: 
        goto state76;
    case '(': case '[': 
        goto state78;
    case '{': 
        goto state87;
    case '*': 
        goto state133;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state139;
    case ';': 
        goto state145;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state149;
    }
    goto state0;

state245: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state246: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state82;
    }
    goto state0;

state247: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state248;
    }
    goto state0;

state248: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state244;
    case 10: 
        goto state245;
    case 13: 
        goto state246;
    case '.': 
        goto state247;
    case '0': case '1': 
        goto state248;
    case '/': 
        goto state249;
    case ';': 
        goto state250;
    case '{': 
        goto state251;
    }
    goto state0;

state249: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0/
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state250: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state251: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0{
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state252: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0/
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state58;
    case 13: 
        goto state59;
    case '"': 
        goto state60;
    case '%': 
        goto state155;
    case '(': case '[': 
        goto state166;
    case '*': 
        goto state167;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state180;
    case ';': 
        goto state193;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state195;
    }
    goto state0;

state253: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state83;
    case 10: 
        goto state84;
    case 13: 
        goto state85;
    }
    goto state0;

state254: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0{
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state88;
    case 10: 
        goto state89;
    case 13: 
        goto state90;
    case ';': 
        goto state91;
    case 'C': case 'c': 
        goto state93;
    case '}': 
        goto state113;
    case 'E': case 'e': 
        goto state127;
    case 'S': case 's': 
        goto state129;
    }
    goto state0;

state255: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state256;
    }
    goto state0;

state256: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state189;
    case 10: 
        goto state190;
    case 13: 
        goto state191;
    case '/': 
        goto state192;
    case ';': 
        goto state199;
    case '{': 
        goto state200;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state256;
    }
    goto state0;

state257: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state258;
    }
    goto state0;

state258: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state204;
    case 13: 
        goto state205;
    case '/': 
        goto state208;
    case ';': 
        goto state209;
    case '{': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state258;
    case '.': 
        goto state259;
    }
    goto state0;

state259: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state260;
    }
    goto state0;

state260: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state204;
    case 13: 
        goto state205;
    case '/': 
        goto state208;
    case ';': 
        goto state209;
    case '{': 
        goto state210;
    case '.': 
        goto state259;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state260;
    }
    goto state0;

state261: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state262: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state134;
    case '%': 
        goto state135;
    case '(': case '[': 
        goto state263;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state264;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state269;
    }
    goto state0;

state263: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state264: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state264;
    case '"': 
        goto state265;
    case '%': 
        goto state266;
    case '(': case '[': 
        goto state267;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state268;
    }
    goto state0;

state265: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state70;
    }
    goto state0;

state266: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state73;
    case 'D': case 'd': 
        goto state136;
    case 'X': case 'x': 
        goto state142;
    }
    goto state0;

state267: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state79;
    }
    goto state0;

state268: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state153;
    case '/': 
        goto state154;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state269: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state153;
    case '/': 
        goto state154;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state270: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state140;
    case '%': 
        goto state141;
    case '(': case '[': 
        goto state261;
    case '*': 
        goto state262;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state270;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state271;
    }
    goto state0;

state271: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state150;
    case 10: 
        goto state151;
    case 13: 
        goto state152;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state153;
    case '/': 
        goto state154;
    case ';': 
        goto state197;
    case '{': 
        goto state198;
    }
    goto state0;

state272: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state273;
    }
    goto state0;

state273: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state176;
    case 10: 
        goto state177;
    case 13: 
        goto state178;
    case '/': 
        goto state179;
    case ';': 
        goto state225;
    case '{': 
        goto state226;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state273;
    }
    goto state0;

state274: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state275;
    }
    goto state0;

state275: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state229;
    case 10: 
        goto state230;
    case 13: 
        goto state231;
    case '/': 
        goto state234;
    case ';': 
        goto state235;
    case '{': 
        goto state236;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state275;
    case '.': 
        goto state276;
    }
    goto state0;

state276: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state277;
    }
    goto state0;

state277: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state229;
    case 10: 
        goto state230;
    case 13: 
        goto state231;
    case '/': 
        goto state234;
    case ';': 
        goto state235;
    case '{': 
        goto state236;
    case '.': 
        goto state276;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state277;
    }
    goto state0;

state278: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state279;
    }
    goto state0;

state279: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state162;
    case 10: 
        goto state163;
    case 13: 
        goto state164;
    case '/': 
        goto state165;
    case ';': 
        goto state240;
    case '{': 
        goto state241;
    case '0': case '1': 
        goto state279;
    }
    goto state0;

state280: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state281;
    }
    goto state0;

state281: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state244;
    case 10: 
        goto state245;
    case 13: 
        goto state246;
    case '/': 
        goto state249;
    case ';': 
        goto state250;
    case '{': 
        goto state251;
    case '0': case '1': 
        goto state281;
    case '.': 
        goto state282;
    }
    goto state0;

state282: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state283;
    }
    goto state0;

state283: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state244;
    case 10: 
        goto state245;
    case 13: 
        goto state246;
    case '/': 
        goto state249;
    case ';': 
        goto state250;
    case '{': 
        goto state251;
    case '.': 
        goto state282;
    case '0': case '1': 
        goto state283;
    }
    goto state0;

state284: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state284;
    case 10: 
        goto state285;
    case 13: 
        goto state286;
    }
    goto state0;

state285: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state286: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state287;
    }
    goto state0;

state287: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state288: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*/\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state62;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state288;
    }
    goto state0;

state289: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state290: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state291: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state292;
    }
    goto state0;

state292: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t(\*;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state293: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state294;
    case '%': 
        goto state295;
    case '(': case '[': 
        goto state334;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state335;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state340;
    }
    goto state0;

state294: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state43;
    }
    goto state0;

state295: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state46;
    case 'D': case 'd': 
        goto state296;
    case 'X': case 'x': 
        goto state302;
    }
    goto state0;

state296: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state297;
    }
    goto state0;

state297: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state237;
    case '{': 
        goto state239;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state297;
    case 9: case ' ': 
        goto state298;
    case 10: 
        goto state343;
    case 13: 
        goto state344;
    case '-': 
        goto state345;
    case '.': 
        goto state351;
    case ';': 
        goto state359;
    }
    goto state0;

state298: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state299: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state300;
    case '%': 
        goto state301;
    case '(': case '[': 
        goto state332;
    case '*': 
        goto state333;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state341;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state342;
    }
    goto state0;

state300: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state43;
    }
    goto state0;

state301: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state46;
    case 'D': case 'd': 
        goto state296;
    case 'X': case 'x': 
        goto state302;
    }
    goto state0;

state302: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state303;
    }
    goto state0;

state303: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state211;
    case '{': 
        goto state213;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state303;
    case 9: case ' ': 
        goto state304;
    case 10: 
        goto state315;
    case 13: 
        goto state316;
    case '-': 
        goto state317;
    case '.': 
        goto state323;
    case ';': 
        goto state331;
    }
    goto state0;

state304: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state305: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state305;
    case 10: 
        goto state306;
    case 13: 
        goto state307;
    }
    goto state0;

state306: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state307: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state308;
    }
    goto state0;

state308: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state309: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state313;
    case ';': 
        goto state314;
    }
    goto state0;

state310: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state311: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state312: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state313: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state313;
    case ';': 
        goto state314;
    }
    goto state0;

state314: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state315: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state316: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state317: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state318;
    }
    goto state0;

state318: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state192;
    case '{': 
        goto state200;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state318;
    case 9: case ' ': 
        goto state319;
    case 10: 
        goto state320;
    case 13: 
        goto state321;
    case ';': 
        goto state322;
    }
    goto state0;

state319: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state320: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state321: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state322: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state323: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state324;
    }
    goto state0;

state324: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state208;
    case '{': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state324;
    case 9: case ' ': 
        goto state325;
    case 10: 
        goto state326;
    case 13: 
        goto state327;
    case '.': 
        goto state328;
    case ';': 
        goto state330;
    }
    goto state0;

state325: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state326: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state327: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state328: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state329;
    }
    goto state0;

state329: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state208;
    case '{': 
        goto state210;
    case 9: case ' ': 
        goto state325;
    case 10: 
        goto state326;
    case 13: 
        goto state327;
    case '.': 
        goto state328;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state329;
    case ';': 
        goto state330;
    }
    goto state0;

state330: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state331: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state332: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state333: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state294;
    case '%': 
        goto state295;
    case '(': case '[': 
        goto state334;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state335;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state340;
    }
    goto state0;

state334: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state335: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state335;
    case '"': 
        goto state336;
    case '%': 
        goto state337;
    case '(': case '[': 
        goto state338;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state339;
    }
    goto state0;

state336: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state43;
    }
    goto state0;

state337: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state46;
    case 'D': case 'd': 
        goto state296;
    case 'X': case 'x': 
        goto state302;
    }
    goto state0;

state338: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state339: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state313;
    case ';': 
        goto state314;
    }
    goto state0;

state340: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state313;
    case ';': 
        goto state314;
    }
    goto state0;

state341: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state300;
    case '%': 
        goto state301;
    case '(': case '[': 
        goto state332;
    case '*': 
        goto state333;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state341;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state342;
    }
    goto state0;

state342: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state313;
    case ';': 
        goto state314;
    }
    goto state0;

state343: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state344: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state345: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state346;
    }
    goto state0;

state346: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state179;
    case '{': 
        goto state226;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state346;
    case 9: case ' ': 
        goto state347;
    case 10: 
        goto state348;
    case 13: 
        goto state349;
    case ';': 
        goto state350;
    }
    goto state0;

state347: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state348: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state349: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state350: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state351: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state352;
    }
    goto state0;

state352: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state234;
    case '{': 
        goto state236;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state352;
    case 9: case ' ': 
        goto state353;
    case 10: 
        goto state354;
    case 13: 
        goto state355;
    case '.': 
        goto state356;
    case ';': 
        goto state358;
    }
    goto state0;

state353: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state354: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state355: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state356: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state357;
    }
    goto state0;

state357: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state234;
    case '{': 
        goto state236;
    case 9: case ' ': 
        goto state353;
    case 10: 
        goto state354;
    case 13: 
        goto state355;
    case '.': 
        goto state356;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state357;
    case ';': 
        goto state358;
    }
    goto state0;

state358: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state359: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\t*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state360: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state361: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state362: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state363;
    }
    goto state0;

state363: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state165;
    case '{': 
        goto state241;
    case '0': case '1': 
        goto state363;
    case 9: case ' ': 
        goto state364;
    case 10: 
        goto state365;
    case 13: 
        goto state366;
    case ';': 
        goto state367;
    }
    goto state0;

state364: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state365: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state366: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state367: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state368: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state369;
    }
    goto state0;

state369: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state249;
    case '{': 
        goto state251;
    case '0': case '1': 
        goto state369;
    case 9: case ' ': 
        goto state370;
    case 10: 
        goto state371;
    case 13: 
        goto state372;
    case '.': 
        goto state373;
    case ';': 
        goto state375;
    }
    goto state0;

state370: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state27;
    case 10: 
        goto state28;
    case '"': 
        goto state42;
    case '%': 
        goto state45;
    case 13: 
        goto state49;
    case '(': case '[': 
        goto state51;
    case '/': 
        goto state56;
    case '{': 
        goto state87;
    case '*': 
        goto state293;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state299;
    case ';': 
        goto state305;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state309;
    }
    goto state0;

state371: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state372: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state55;
    }
    goto state0;

state373: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state374;
    }
    goto state0;

state374: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state249;
    case '{': 
        goto state251;
    case 9: case ' ': 
        goto state370;
    case 10: 
        goto state371;
    case 13: 
        goto state372;
    case '.': 
        goto state373;
    case '0': case '1': 
        goto state374;
    case ';': 
        goto state375;
    }
    goto state0;

state375: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state376: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state289;
    case 10: 
        goto state290;
    case 13: 
        goto state291;
    }
    goto state0;

state377: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state377;
    case 10: 
        goto state378;
    case 13: 
        goto state379;
    }
    goto state0;

state378: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t;\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state379: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state380;
    }
    goto state0;

state380: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"#"\t\n\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    }
    goto state0;

state381: // \t\n\t\n\n\t\r\n;\nA\t=\t\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state26;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state381;
    }
    goto state0;

state382: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state383;
    case 'D': case 'd': 
        goto state391;
    case 'X': case 'x': 
        goto state399;
    }
    goto state0;

state383: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state384;
    }
    goto state0;

state384: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state48;
    case '/': 
        goto state252;
    case '{': 
        goto state254;
    case 10: 
        goto state360;
    case 13: 
        goto state361;
    case ';': 
        goto state376;
    case '0': case '1': 
        goto state384;
    case '-': 
        goto state385;
    case '.': 
        goto state387;
    }
    goto state0;

state385: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state386;
    }
    goto state0;

state386: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state165;
    case '{': 
        goto state241;
    case 9: case ' ': 
        goto state364;
    case 10: 
        goto state365;
    case 13: 
        goto state366;
    case ';': 
        goto state367;
    case '0': case '1': 
        goto state386;
    }
    goto state0;

state387: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state388;
    }
    goto state0;

state388: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state249;
    case '{': 
        goto state251;
    case 9: case ' ': 
        goto state370;
    case 10: 
        goto state371;
    case 13: 
        goto state372;
    case ';': 
        goto state375;
    case '0': case '1': 
        goto state388;
    case '.': 
        goto state389;
    }
    goto state0;

state389: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state390;
    }
    goto state0;

state390: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state249;
    case '{': 
        goto state251;
    case 9: case ' ': 
        goto state370;
    case 10: 
        goto state371;
    case 13: 
        goto state372;
    case ';': 
        goto state375;
    case '.': 
        goto state389;
    case '0': case '1': 
        goto state390;
    }
    goto state0;

state391: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state392;
    }
    goto state0;

state392: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state237;
    case '{': 
        goto state239;
    case 9: case ' ': 
        goto state298;
    case 10: 
        goto state343;
    case 13: 
        goto state344;
    case ';': 
        goto state359;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state392;
    case '-': 
        goto state393;
    case '.': 
        goto state395;
    }
    goto state0;

state393: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state394;
    }
    goto state0;

state394: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state179;
    case '{': 
        goto state226;
    case 9: case ' ': 
        goto state347;
    case 10: 
        goto state348;
    case 13: 
        goto state349;
    case ';': 
        goto state350;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state394;
    }
    goto state0;

state395: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state396;
    }
    goto state0;

state396: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state234;
    case '{': 
        goto state236;
    case 9: case ' ': 
        goto state353;
    case 10: 
        goto state354;
    case 13: 
        goto state355;
    case ';': 
        goto state358;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state396;
    case '.': 
        goto state397;
    }
    goto state0;

state397: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state398;
    }
    goto state0;

state398: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state234;
    case '{': 
        goto state236;
    case 9: case ' ': 
        goto state353;
    case 10: 
        goto state354;
    case 13: 
        goto state355;
    case ';': 
        goto state358;
    case '.': 
        goto state397;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state398;
    }
    goto state0;

state399: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state400;
    }
    goto state0;

state400: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state211;
    case '{': 
        goto state213;
    case 9: case ' ': 
        goto state304;
    case 10: 
        goto state315;
    case 13: 
        goto state316;
    case ';': 
        goto state331;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state400;
    case '-': 
        goto state401;
    case '.': 
        goto state403;
    }
    goto state0;

state401: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state402;
    }
    goto state0;

state402: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state192;
    case '{': 
        goto state200;
    case 9: case ' ': 
        goto state319;
    case 10: 
        goto state320;
    case 13: 
        goto state321;
    case ';': 
        goto state322;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state402;
    }
    goto state0;

state403: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state404;
    }
    goto state0;

state404: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state208;
    case '{': 
        goto state210;
    case 9: case ' ': 
        goto state325;
    case 10: 
        goto state326;
    case 13: 
        goto state327;
    case ';': 
        goto state330;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state404;
    case '.': 
        goto state405;
    }
    goto state0;

state405: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state406;
    }
    goto state0;

state406: // \t\n\t\n\n\t\r\n;\nA\t=\t\t%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state208;
    case '{': 
        goto state210;
    case 9: case ' ': 
        goto state325;
    case 10: 
        goto state326;
    case 13: 
        goto state327;
    case ';': 
        goto state330;
    case '.': 
        goto state405;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state406;
    }
    goto state0;

state407: // \t\n\t\n\n\t\r\n;\nA\t=\t\t(
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state408: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state409;
    case '%': 
        goto state410;
    case '(': case '[': 
        goto state411;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state412;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state418;
    }
    goto state0;

state409: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state25;
    }
    goto state0;

state410: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state383;
    case 'D': case 'd': 
        goto state391;
    case 'X': case 'x': 
        goto state399;
    }
    goto state0;

state411: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state412: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state412;
    case '"': 
        goto state413;
    case '%': 
        goto state414;
    case '(': case '[': 
        goto state415;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state416;
    }
    goto state0;

state413: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state25;
    }
    goto state0;

state414: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state383;
    case 'D': case 'd': 
        goto state391;
    case 'X': case 'x': 
        goto state399;
    }
    goto state0;

state415: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state416: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state417: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*0A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state418: // \t\n\t\n\n\t\r\n;\nA\t=\t\t*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state419: // \t\n\t\n\n\t\r\n;\nA\t=\t\t0
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state420;
    case '%': 
        goto state421;
    case '(': case '[': 
        goto state422;
    case '*': 
        goto state423;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state424;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state425;
    }
    goto state0;

state420: // \t\n\t\n\n\t\r\n;\nA\t=\t\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state25;
    }
    goto state0;

state421: // \t\n\t\n\n\t\r\n;\nA\t=\t\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state383;
    case 'D': case 'd': 
        goto state391;
    case 'X': case 'x': 
        goto state399;
    }
    goto state0;

state422: // \t\n\t\n\n\t\r\n;\nA\t=\t\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state423: // \t\n\t\n\n\t\r\n;\nA\t=\t\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state409;
    case '%': 
        goto state410;
    case '(': case '[': 
        goto state411;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state412;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state418;
    }
    goto state0;

state424: // \t\n\t\n\n\t\r\n;\nA\t=\t\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state420;
    case '%': 
        goto state421;
    case '(': case '[': 
        goto state422;
    case '*': 
        goto state423;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state424;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state425;
    }
    goto state0;

state425: // \t\n\t\n\n\t\r\n;\nA\t=\t\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state426: // \t\n\t\n\n\t\r\n;\nA\t=\t\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state22;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state426;
    case 13: 
        goto state427;
    }
    goto state0;

state427: // \t\n\t\n\n\t\r\n;\nA\t=\t\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state22;
    }
    goto state0;

state428: // \t\n\t\n\n\t\r\n;\nA\t=\t\tA
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state429: // \t\n\t\n\n\t\r\n;\nA\t=\n
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state21;
    }
    goto state0;

state430: // \t\n\t\n\n\t\r\n;\nA\t=\r
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state22;
    }
    goto state0;

state431: // \t\n\t\n\n\t\r\n;\nA\t="
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state25;
    }
    goto state0;

state432: // \t\n\t\n\n\t\r\n;\nA\t=%
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state383;
    case 'D': case 'd': 
        goto state391;
    case 'X': case 'x': 
        goto state399;
    }
    goto state0;

state433: // \t\n\t\n\n\t\r\n;\nA\t=(
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state52;
    }
    goto state0;

state434: // \t\n\t\n\n\t\r\n;\nA\t=*
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state409;
    case '%': 
        goto state410;
    case '(': case '[': 
        goto state411;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state412;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state418;
    }
    goto state0;

state435: // \t\n\t\n\n\t\r\n;\nA\t=/
    m_notify->onDefinedAsIncrementalEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state21;
    case 10: 
        goto state22;
    case 13: 
        goto state23;
    case '"': 
        goto state24;
    case '%': 
        goto state382;
    case '(': case '[': 
        goto state407;
    case '*': 
        goto state408;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state419;
    case ';': 
        goto state426;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state428;
    }
    goto state0;

state436: // \t\n\t\n\n\t\r\n;\nA\t=0
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state420;
    case '%': 
        goto state421;
    case '(': case '[': 
        goto state422;
    case '*': 
        goto state423;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state424;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state425;
    }
    goto state0;

state437: // \t\n\t\n\n\t\r\n;\nA\t=;
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state22;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state426;
    case 13: 
        goto state427;
    }
    goto state0;

state438: // \t\n\t\n\n\t\r\n;\nA\t=A
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state154;
    case '{': 
        goto state198;
    case 9: case ' ': 
        goto state310;
    case 10: 
        goto state311;
    case 13: 
        goto state312;
    case ';': 
        goto state314;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state417;
    }
    goto state0;

state439: // \t\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state440;
    }
    goto state0;

state440: // \t\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state441: // \t\n\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state441;
    case 10: 
        goto state442;
    case 13: 
        goto state443;
    }
    goto state0;

state442: // \t\n\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state443: // \t\n\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state444;
    }
    goto state0;

state444: // \t\n\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state445: // \t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state446;
    }
    goto state0;

state446: // \t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state447: // \t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state447;
    case 10: 
        goto state448;
    case 13: 
        goto state449;
    }
    goto state0;

state448: // \t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state449: // \t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state450;
    }
    goto state0;

state450: // \t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state11;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state13;
    }
    goto state0;

state451: // A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state452;
    case 10: 
        goto state453;
    case 13: 
        goto state454;
    case ';': 
        goto state455;
    case '=': 
        goto state457;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state863;
    }
    goto state0;

state452: // A\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state452;
    case 10: 
        goto state453;
    case 13: 
        goto state454;
    case ';': 
        goto state455;
    case '=': 
        goto state457;
    }
    goto state0;

state453: // A\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state452;
    }
    goto state0;

state454: // A\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state453;
    }
    goto state0;

state455: // A\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state453;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state455;
    case 13: 
        goto state456;
    }
    goto state0;

state456: // A\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state453;
    }
    goto state0;

state457: // A\t=
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state458;
    case 10: 
        goto state853;
    case 13: 
        goto state854;
    case '"': 
        goto state855;
    case '%': 
        goto state856;
    case '(': case '[': 
        goto state857;
    case '*': 
        goto state858;
    case '/': 
        goto state859;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state860;
    case ';': 
        goto state861;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state862;
    }
    goto state0;

state458: // A\t=\t
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state459;
    case 10: 
        goto state460;
    case 13: 
        goto state461;
    case '"': 
        goto state462;
    case '%': 
        goto state806;
    case '(': case '[': 
        goto state831;
    case '*': 
        goto state832;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state843;
    case ';': 
        goto state850;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state852;
    }
    goto state0;

state459: // A\t=\t\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state459;
    case 10: 
        goto state460;
    case 13: 
        goto state461;
    case '"': 
        goto state462;
    case '%': 
        goto state806;
    case '(': case '[': 
        goto state831;
    case '*': 
        goto state832;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state843;
    case ';': 
        goto state850;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state852;
    }
    goto state0;

state460: // A\t=\t\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state459;
    }
    goto state0;

state461: // A\t=\t\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state460;
    }
    goto state0;

state462: // A\t=\t\t"
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state463;
    }
    goto state0;

state463: // A\t=\t\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state464;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state805;
    }
    goto state0;

state464: // A\t=\t\t"#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state482;
    case 13: 
        goto state483;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case ';': 
        goto state713;
    }
    goto state0;

state465: // A\t=\t\t"#"\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state466: // A\t=\t\t"#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state467: // A\t=\t\t"#"\t\n\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state467;
    case 10: 
        goto state468;
    case 13: 
        goto state469;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    case ';': 
        goto state801;
    }
    goto state0;

state468: // A\t=\t\t"#"\t\n\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state469: // A\t=\t\t"#"\t\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state470;
    }
    goto state0;

state470: // A\t=\t\t"#"\t\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state471: // A\t=\t\t"#"\t\n\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state472;
    }
    goto state0;

state472: // A\t=\t\t"#"\t\n\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state464;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state473;
    }
    goto state0;

state473: // A\t=\t\t"#"\t\n\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state464;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state473;
    }
    goto state0;

state474: // A\t=\t\t"#"\t\n\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state475;
    case 'D': case 'd': 
        goto state720;
    case 'X': case 'x': 
        goto state726;
    }
    goto state0;

state475: // A\t=\t\t"#"\t\n\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state476;
    }
    goto state0;

state476: // A\t=\t\t"#"\t\n\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state476;
    case 9: case ' ': 
        goto state477;
    case '/': 
        goto state676;
    case '{': 
        goto state678;
    case 10: 
        goto state784;
    case 13: 
        goto state785;
    case '-': 
        goto state786;
    case '.': 
        goto state792;
    case ';': 
        goto state800;
    }
    goto state0;

state477: // A\t=\t\t"#"\t\n\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state478: // A\t=\t\t"#"\t\n\t%B0\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state479;
    }
    goto state0;

state479: // A\t=\t\t"#"\t\n\t%B0\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state480: // A\t=\t\t"#"\t\n\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state481: // A\t=\t\t"#"\t\n\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state482;
    case 13: 
        goto state483;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case ';': 
        goto state713;
    }
    goto state0;

state482: // A\t=\t\t"#"\t\n\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state483: // A\t=\t\t"#"\t\n\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state484: // A\t=\t\t"#"\t\n\t%B0\t(\*\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state485: // A\t=\t\t"#"\t\n\t%B0\t(\*/
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state486: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state487: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    }
    goto state0;

state488: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state487;
    }
    goto state0;

state489: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state490;
    }
    goto state0;

state490: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state491;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state712;
    }
    goto state0;

state491: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state509;
    case 13: 
        goto state510;
    case ';': 
        goto state512;
    case '{': 
        goto state516;
    }
    goto state0;

state492: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state493: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state494: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state494;
    case 10: 
        goto state495;
    case 13: 
        goto state496;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    case ';': 
        goto state708;
    }
    goto state0;

state495: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state496: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state497;
    }
    goto state0;

state497: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state498: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state499;
    }
    goto state0;

state499: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state491;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state500;
    }
    goto state0;

state500: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state491;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state500;
    }
    goto state0;

state501: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state502;
    case 'D': case 'd': 
        goto state560;
    case 'X': case 'x': 
        goto state566;
    }
    goto state0;

state502: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state503;
    }
    goto state0;

state503: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state503;
    case 9: case ' ': 
        goto state504;
    case 10: 
        goto state582;
    case 13: 
        goto state583;
    case '/': 
        goto state676;
    case ';': 
        goto state677;
    case '{': 
        goto state678;
    case '-': 
        goto state702;
    case '.': 
        goto state704;
    }
    goto state0;

state504: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state505: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state506;
    }
    goto state0;

state506: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state507: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state508: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state509;
    case 13: 
        goto state510;
    case ';': 
        goto state512;
    case '{': 
        goto state516;
    }
    goto state0;

state509: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state510: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state511: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state512: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state513: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state514: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state515;
    }
    goto state0;

state515: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state516: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state517: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state518: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    }
    goto state0;

state519: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state518;
    }
    goto state0;

state520: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state518;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state520;
    case 13: 
        goto state521;
    }
    goto state0;

state521: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state518;
    }
    goto state0;

state522: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tC
    ch = *ptr++;
    switch (ch) {
    case 'H': case 'h': 
        goto state523;
    }
    goto state0;

state523: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCH
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state524;
    }
    goto state0;

state524: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state525;
    }
    goto state0;

state525: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR
    m_notify->onActionCharEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state526;
    case 10: 
        goto state527;
    case 13: 
        goto state528;
    case ',': 
        goto state529;
    case ';': 
        goto state540;
    case '}': 
        goto state542;
    }
    goto state0;

state526: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state526;
    case 10: 
        goto state527;
    case 13: 
        goto state528;
    case ',': 
        goto state529;
    case ';': 
        goto state540;
    case '}': 
        goto state542;
    }
    goto state0;

state527: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state526;
    }
    goto state0;

state528: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state527;
    }
    goto state0;

state529: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state529;
    case 10: 
        goto state530;
    case 13: 
        goto state531;
    case ';': 
        goto state532;
    case 'C': case 'c': 
        goto state534;
    case 'E': case 'e': 
        goto state537;
    case 'S': case 's': 
        goto state546;
    }
    goto state0;

state530: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state529;
    }
    goto state0;

state531: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state530;
    }
    goto state0;

state532: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state530;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state532;
    case 13: 
        goto state533;
    }
    goto state0;

state533: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state530;
    }
    goto state0;

state534: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,C
    ch = *ptr++;
    switch (ch) {
    case 'H': case 'h': 
        goto state535;
    }
    goto state0;

state535: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,CH
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state536;
    }
    goto state0;

state536: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,CHA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state525;
    }
    goto state0;

state537: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,E
    ch = *ptr++;
    switch (ch) {
    case 'N': case 'n': 
        goto state538;
    }
    goto state0;

state538: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,EN
    ch = *ptr++;
    switch (ch) {
    case 'D': case 'd': 
        goto state539;
    }
    goto state0;

state539: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END
    m_notify->onActionEndEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state526;
    case 10: 
        goto state527;
    case 13: 
        goto state528;
    case ',': 
        goto state529;
    case ';': 
        goto state540;
    case '}': 
        goto state542;
    }
    goto state0;

state540: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state527;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state540;
    case 13: 
        goto state541;
    }
    goto state0;

state541: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state527;
    }
    goto state0;

state542: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state114;
    case 13: 
        goto state543;
    case ';': 
        goto state544;
    }
    goto state0;

state543: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END}\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state117;
    }
    goto state0;

state544: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state119;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state544;
    case 13: 
        goto state545;
    }
    goto state0;

state545: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,END};\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state121;
    }
    goto state0;

state546: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,S
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state547;
    }
    goto state0;

state547: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,ST
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state548;
    }
    goto state0;

state548: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,STA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state549;
    }
    goto state0;

state549: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,STAR
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state550;
    }
    goto state0;

state550: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tCHAR\t,START
    m_notify->onActionStartEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state526;
    case 10: 
        goto state527;
    case 13: 
        goto state528;
    case ',': 
        goto state529;
    case ';': 
        goto state540;
    case '}': 
        goto state542;
    }
    goto state0;

state551: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tE
    ch = *ptr++;
    switch (ch) {
    case 'N': case 'n': 
        goto state552;
    }
    goto state0;

state552: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tEN
    ch = *ptr++;
    switch (ch) {
    case 'D': case 'd': 
        goto state539;
    }
    goto state0;

state553: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tS
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state554;
    }
    goto state0;

state554: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tST
    ch = *ptr++;
    switch (ch) {
    case 'A': case 'a': 
        goto state555;
    }
    goto state0;

state555: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tSTA
    ch = *ptr++;
    switch (ch) {
    case 'R': case 'r': 
        goto state556;
    }
    goto state0;

state556: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t(\*{\tSTAR
    ch = *ptr++;
    switch (ch) {
    case 'T': case 't': 
        goto state550;
    }
    goto state0;

state557: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state558;
    case '%': 
        goto state559;
    case '(': case '[': 
        goto state687;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state688;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state693;
    }
    goto state0;

state558: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state499;
    }
    goto state0;

state559: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state502;
    case 'D': case 'd': 
        goto state560;
    case 'X': case 'x': 
        goto state566;
    }
    goto state0;

state560: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state561;
    }
    goto state0;

state561: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state561;
    case 9: case ' ': 
        goto state562;
    case 10: 
        goto state596;
    case 13: 
        goto state597;
    case '/': 
        goto state661;
    case ';': 
        goto state662;
    case '{': 
        goto state663;
    case '-': 
        goto state696;
    case '.': 
        goto state698;
    }
    goto state0;

state562: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state563: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state564;
    case '%': 
        goto state565;
    case '(': case '[': 
        goto state685;
    case '*': 
        goto state686;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state694;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state695;
    }
    goto state0;

state564: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state499;
    }
    goto state0;

state565: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state502;
    case 'D': case 'd': 
        goto state560;
    case 'X': case 'x': 
        goto state566;
    }
    goto state0;

state566: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state567;
    }
    goto state0;

state567: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state567;
    case 9: case ' ': 
        goto state568;
    case 10: 
        goto state609;
    case 13: 
        goto state610;
    case '/': 
        goto state635;
    case ';': 
        goto state636;
    case '{': 
        goto state637;
    case '-': 
        goto state679;
    case '.': 
        goto state681;
    }
    goto state0;

state568: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state569: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state569;
    case 10: 
        goto state570;
    case 13: 
        goto state571;
    }
    goto state0;

state570: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state571: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state572;
    }
    goto state0;

state572: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state573: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state577;
    case '/': 
        goto state578;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state574: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state575: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state576: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state577: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state577;
    case '/': 
        goto state578;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state578: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state579: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state580;
    case 'D': case 'd': 
        goto state594;
    case 'X': case 'x': 
        goto state607;
    }
    goto state0;

state580: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state581;
    }
    goto state0;

state581: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state504;
    case '0': case '1': 
        goto state581;
    case 10: 
        goto state582;
    case 13: 
        goto state583;
    case '-': 
        goto state584;
    case '.': 
        goto state666;
    case '/': 
        goto state676;
    case ';': 
        goto state677;
    case '{': 
        goto state678;
    }
    goto state0;

state582: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state583: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state584: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state585;
    }
    goto state0;

state585: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state585;
    case 9: case ' ': 
        goto state586;
    case 10: 
        goto state587;
    case 13: 
        goto state588;
    case '/': 
        goto state589;
    case ';': 
        goto state664;
    case '{': 
        goto state665;
    }
    goto state0;

state586: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state587: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state588: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state589: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state590: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/(
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state591: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state592;
    case '%': 
        goto state593;
    case '(': case '[': 
        goto state640;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state641;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state646;
    }
    goto state0;

state592: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state490;
    }
    goto state0;

state593: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state580;
    case 'D': case 'd': 
        goto state594;
    case 'X': case 'x': 
        goto state607;
    }
    goto state0;

state594: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state595;
    }
    goto state0;

state595: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state562;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state595;
    case 10: 
        goto state596;
    case 13: 
        goto state597;
    case '-': 
        goto state598;
    case '.': 
        goto state651;
    case '/': 
        goto state661;
    case ';': 
        goto state662;
    case '{': 
        goto state663;
    }
    goto state0;

state596: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state597: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state598: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state599;
    }
    goto state0;

state599: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state599;
    case 9: case ' ': 
        goto state600;
    case 10: 
        goto state601;
    case 13: 
        goto state602;
    case '/': 
        goto state603;
    case ';': 
        goto state649;
    case '{': 
        goto state650;
    }
    goto state0;

state600: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state601: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state602: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state603: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state604: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state605;
    case '%': 
        goto state606;
    case '(': case '[': 
        goto state638;
    case '*': 
        goto state639;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state647;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state648;
    }
    goto state0;

state605: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state490;
    }
    goto state0;

state606: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state580;
    case 'D': case 'd': 
        goto state594;
    case 'X': case 'x': 
        goto state607;
    }
    goto state0;

state607: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state608;
    }
    goto state0;

state608: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state568;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state608;
    case 10: 
        goto state609;
    case 13: 
        goto state610;
    case '-': 
        goto state611;
    case '.': 
        goto state625;
    case '/': 
        goto state635;
    case ';': 
        goto state636;
    case '{': 
        goto state637;
    }
    goto state0;

state609: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state610: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state611: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state612;
    }
    goto state0;

state612: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state612;
    case 9: case ' ': 
        goto state613;
    case 10: 
        goto state614;
    case 13: 
        goto state615;
    case '/': 
        goto state616;
    case ';': 
        goto state623;
    case '{': 
        goto state624;
    }
    goto state0;

state613: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state614: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state615: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state616: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state617: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state487;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state617;
    case 13: 
        goto state618;
    }
    goto state0;

state618: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state487;
    }
    goto state0;

state619: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '/': 
        goto state578;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state620;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state620: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '/': 
        goto state578;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state620;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state621: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state622: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-{
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state623: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state624: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0{
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state625: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state626;
    }
    goto state0;

state626: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state626;
    case 9: case ' ': 
        goto state627;
    case 10: 
        goto state628;
    case 13: 
        goto state629;
    case '.': 
        goto state630;
    case '/': 
        goto state632;
    case ';': 
        goto state633;
    case '{': 
        goto state634;
    }
    goto state0;

state627: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state628: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state629: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state630: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state631;
    }
    goto state0;

state631: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state627;
    case 10: 
        goto state628;
    case 13: 
        goto state629;
    case '.': 
        goto state630;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state631;
    case '/': 
        goto state632;
    case ';': 
        goto state633;
    case '{': 
        goto state634;
    }
    goto state0;

state632: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0/
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state633: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state634: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0{
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state635: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0/
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state636: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state637: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0{
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state638: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state639: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state592;
    case '%': 
        goto state593;
    case '(': case '[': 
        goto state640;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state641;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state646;
    }
    goto state0;

state640: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state641: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state641;
    case '"': 
        goto state642;
    case '%': 
        goto state643;
    case '(': case '[': 
        goto state644;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state645;
    }
    goto state0;

state642: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state490;
    }
    goto state0;

state643: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state580;
    case 'D': case 'd': 
        goto state594;
    case 'X': case 'x': 
        goto state607;
    }
    goto state0;

state644: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state645: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '/': 
        goto state578;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state620;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state646: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '/': 
        goto state578;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state620;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state647: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state605;
    case '%': 
        goto state606;
    case '(': case '[': 
        goto state638;
    case '*': 
        goto state639;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state647;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state648;
    }
    goto state0;

state648: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '/': 
        goto state578;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state620;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state649: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state650: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0{
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state651: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state652;
    }
    goto state0;

state652: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state652;
    case 9: case ' ': 
        goto state653;
    case 10: 
        goto state654;
    case 13: 
        goto state655;
    case '.': 
        goto state656;
    case '/': 
        goto state658;
    case ';': 
        goto state659;
    case '{': 
        goto state660;
    }
    goto state0;

state653: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state654: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state655: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state656: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state657;
    }
    goto state0;

state657: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state653;
    case 10: 
        goto state654;
    case 13: 
        goto state655;
    case '.': 
        goto state656;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state657;
    case '/': 
        goto state658;
    case ';': 
        goto state659;
    case '{': 
        goto state660;
    }
    goto state0;

state658: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0/
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state659: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state660: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0{
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state661: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0/
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state662: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state663: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0{
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state664: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state665: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0-0{
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state666: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state667;
    }
    goto state0;

state667: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state667;
    case 9: case ' ': 
        goto state668;
    case 10: 
        goto state669;
    case 13: 
        goto state670;
    case '.': 
        goto state671;
    case '/': 
        goto state673;
    case ';': 
        goto state674;
    case '{': 
        goto state675;
    }
    goto state0;

state668: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state485;
    case 9: case ' ': 
        goto state492;
    case 10: 
        goto state493;
    case '"': 
        goto state498;
    case '%': 
        goto state501;
    case 13: 
        goto state505;
    case '(': case '[': 
        goto state507;
    case '{': 
        goto state516;
    case '*': 
        goto state557;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state563;
    case ';': 
        goto state569;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state573;
    }
    goto state0;

state669: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state670: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state511;
    }
    goto state0;

state671: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state672;
    }
    goto state0;

state672: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state668;
    case 10: 
        goto state669;
    case 13: 
        goto state670;
    case '.': 
        goto state671;
    case '0': case '1': 
        goto state672;
    case '/': 
        goto state673;
    case ';': 
        goto state674;
    case '{': 
        goto state675;
    }
    goto state0;

state673: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0/
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state674: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state675: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0.0.0{
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state676: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0/
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state486;
    case 10: 
        goto state487;
    case 13: 
        goto state488;
    case '"': 
        goto state489;
    case '%': 
        goto state579;
    case '(': case '[': 
        goto state590;
    case '*': 
        goto state591;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state604;
    case ';': 
        goto state617;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state619;
    }
    goto state0;

state677: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state512;
    case 10: 
        goto state513;
    case 13: 
        goto state514;
    }
    goto state0;

state678: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-/%B0{
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state517;
    case 10: 
        goto state518;
    case 13: 
        goto state519;
    case ';': 
        goto state520;
    case 'C': case 'c': 
        goto state522;
    case '}': 
        goto state542;
    case 'E': case 'e': 
        goto state551;
    case 'S': case 's': 
        goto state553;
    }
    goto state0;

state679: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state680;
    }
    goto state0;

state680: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state613;
    case 10: 
        goto state614;
    case 13: 
        goto state615;
    case '/': 
        goto state616;
    case ';': 
        goto state623;
    case '{': 
        goto state624;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state680;
    }
    goto state0;

state681: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state682;
    }
    goto state0;

state682: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state627;
    case 10: 
        goto state628;
    case 13: 
        goto state629;
    case '/': 
        goto state632;
    case ';': 
        goto state633;
    case '{': 
        goto state634;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state682;
    case '.': 
        goto state683;
    }
    goto state0;

state683: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state684;
    }
    goto state0;

state684: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state627;
    case 10: 
        goto state628;
    case 13: 
        goto state629;
    case '/': 
        goto state632;
    case ';': 
        goto state633;
    case '{': 
        goto state634;
    case '.': 
        goto state683;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state684;
    }
    goto state0;

state685: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state686: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state558;
    case '%': 
        goto state559;
    case '(': case '[': 
        goto state687;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state688;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state693;
    }
    goto state0;

state687: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state688: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state688;
    case '"': 
        goto state689;
    case '%': 
        goto state690;
    case '(': case '[': 
        goto state691;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state692;
    }
    goto state0;

state689: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state499;
    }
    goto state0;

state690: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state502;
    case 'D': case 'd': 
        goto state560;
    case 'X': case 'x': 
        goto state566;
    }
    goto state0;

state691: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state508;
    }
    goto state0;

state692: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state577;
    case '/': 
        goto state578;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state693: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state577;
    case '/': 
        goto state578;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state694: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state564;
    case '%': 
        goto state565;
    case '(': case '[': 
        goto state685;
    case '*': 
        goto state686;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state694;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state695;
    }
    goto state0;

state695: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state574;
    case 10: 
        goto state575;
    case 13: 
        goto state576;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state577;
    case '/': 
        goto state578;
    case ';': 
        goto state621;
    case '{': 
        goto state622;
    }
    goto state0;

state696: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state697;
    }
    goto state0;

state697: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state600;
    case 10: 
        goto state601;
    case 13: 
        goto state602;
    case '/': 
        goto state603;
    case ';': 
        goto state649;
    case '{': 
        goto state650;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state697;
    }
    goto state0;

state698: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state699;
    }
    goto state0;

state699: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state653;
    case 10: 
        goto state654;
    case 13: 
        goto state655;
    case '/': 
        goto state658;
    case ';': 
        goto state659;
    case '{': 
        goto state660;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state699;
    case '.': 
        goto state700;
    }
    goto state0;

state700: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state701;
    }
    goto state0;

state701: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state653;
    case 10: 
        goto state654;
    case 13: 
        goto state655;
    case '/': 
        goto state658;
    case ';': 
        goto state659;
    case '{': 
        goto state660;
    case '.': 
        goto state700;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state701;
    }
    goto state0;

state702: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state703;
    }
    goto state0;

state703: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state586;
    case 10: 
        goto state587;
    case 13: 
        goto state588;
    case '/': 
        goto state589;
    case ';': 
        goto state664;
    case '{': 
        goto state665;
    case '0': case '1': 
        goto state703;
    }
    goto state0;

state704: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state705;
    }
    goto state0;

state705: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state668;
    case 10: 
        goto state669;
    case 13: 
        goto state670;
    case '/': 
        goto state673;
    case ';': 
        goto state674;
    case '{': 
        goto state675;
    case '0': case '1': 
        goto state705;
    case '.': 
        goto state706;
    }
    goto state0;

state706: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state707;
    }
    goto state0;

state707: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state668;
    case 10: 
        goto state669;
    case 13: 
        goto state670;
    case '/': 
        goto state673;
    case ';': 
        goto state674;
    case '{': 
        goto state675;
    case '.': 
        goto state706;
    case '0': case '1': 
        goto state707;
    }
    goto state0;

state708: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state708;
    case 10: 
        goto state709;
    case 13: 
        goto state710;
    }
    goto state0;

state709: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state710: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state711;
    }
    goto state0;

state711: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"#"\t\n\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state494;
    }
    goto state0;

state712: // A\t=\t\t"#"\t\n\t%B0\t(\*/\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state491;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state712;
    }
    goto state0;

state713: // A\t=\t\t"#"\t\n\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state714: // A\t=\t\t"#"\t\n\t%B0\t(\*;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state715: // A\t=\t\t"#"\t\n\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state716;
    }
    goto state0;

state716: // A\t=\t\t"#"\t\n\t%B0\t(\*;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state717: // A\t=\t\t"#"\t\n\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state718;
    case '%': 
        goto state719;
    case '(': case '[': 
        goto state758;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state759;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state764;
    }
    goto state0;

state718: // A\t=\t\t"#"\t\n\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state472;
    }
    goto state0;

state719: // A\t=\t\t"#"\t\n\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state475;
    case 'D': case 'd': 
        goto state720;
    case 'X': case 'x': 
        goto state726;
    }
    goto state0;

state720: // A\t=\t\t"#"\t\n\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state721;
    }
    goto state0;

state721: // A\t=\t\t"#"\t\n\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state661;
    case '{': 
        goto state663;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state721;
    case 9: case ' ': 
        goto state722;
    case 10: 
        goto state767;
    case 13: 
        goto state768;
    case '-': 
        goto state769;
    case '.': 
        goto state775;
    case ';': 
        goto state783;
    }
    goto state0;

state722: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state723: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state724;
    case '%': 
        goto state725;
    case '(': case '[': 
        goto state756;
    case '*': 
        goto state757;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state765;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state766;
    }
    goto state0;

state724: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state472;
    }
    goto state0;

state725: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state475;
    case 'D': case 'd': 
        goto state720;
    case 'X': case 'x': 
        goto state726;
    }
    goto state0;

state726: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state727;
    }
    goto state0;

state727: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state635;
    case '{': 
        goto state637;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state727;
    case 9: case ' ': 
        goto state728;
    case 10: 
        goto state739;
    case 13: 
        goto state740;
    case '-': 
        goto state741;
    case '.': 
        goto state747;
    case ';': 
        goto state755;
    }
    goto state0;

state728: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state729: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state729;
    case 10: 
        goto state730;
    case 13: 
        goto state731;
    }
    goto state0;

state730: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state731: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state732;
    }
    goto state0;

state732: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state733: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state737;
    case ';': 
        goto state738;
    }
    goto state0;

state734: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state735: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state736: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state737: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state737;
    case ';': 
        goto state738;
    }
    goto state0;

state738: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\tA-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state739: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state740: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state741: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state742;
    }
    goto state0;

state742: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state616;
    case '{': 
        goto state624;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state742;
    case 9: case ' ': 
        goto state743;
    case 10: 
        goto state744;
    case 13: 
        goto state745;
    case ';': 
        goto state746;
    }
    goto state0;

state743: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state744: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state745: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state746: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state747: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state748;
    }
    goto state0;

state748: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state632;
    case '{': 
        goto state634;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state748;
    case 9: case ' ': 
        goto state749;
    case 10: 
        goto state750;
    case 13: 
        goto state751;
    case '.': 
        goto state752;
    case ';': 
        goto state754;
    }
    goto state0;

state749: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state750: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state751: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state752: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state753;
    }
    goto state0;

state753: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state632;
    case '{': 
        goto state634;
    case 9: case ' ': 
        goto state749;
    case 10: 
        goto state750;
    case 13: 
        goto state751;
    case '.': 
        goto state752;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state753;
    case ';': 
        goto state754;
    }
    goto state0;

state754: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state755: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state756: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state757: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state718;
    case '%': 
        goto state719;
    case '(': case '[': 
        goto state758;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state759;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state764;
    }
    goto state0;

state758: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state759: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state759;
    case '"': 
        goto state760;
    case '%': 
        goto state761;
    case '(': case '[': 
        goto state762;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state763;
    }
    goto state0;

state760: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state472;
    }
    goto state0;

state761: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state475;
    case 'D': case 'd': 
        goto state720;
    case 'X': case 'x': 
        goto state726;
    }
    goto state0;

state762: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state763: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state737;
    case ';': 
        goto state738;
    }
    goto state0;

state764: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state737;
    case ';': 
        goto state738;
    }
    goto state0;

state765: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state724;
    case '%': 
        goto state725;
    case '(': case '[': 
        goto state756;
    case '*': 
        goto state757;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state765;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state766;
    }
    goto state0;

state766: // A\t=\t\t"#"\t\n\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state737;
    case ';': 
        goto state738;
    }
    goto state0;

state767: // A\t=\t\t"#"\t\n\t%B0\t*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state768: // A\t=\t\t"#"\t\n\t%B0\t*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state769: // A\t=\t\t"#"\t\n\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state770;
    }
    goto state0;

state770: // A\t=\t\t"#"\t\n\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state603;
    case '{': 
        goto state650;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state770;
    case 9: case ' ': 
        goto state771;
    case 10: 
        goto state772;
    case 13: 
        goto state773;
    case ';': 
        goto state774;
    }
    goto state0;

state771: // A\t=\t\t"#"\t\n\t%B0\t*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state772: // A\t=\t\t"#"\t\n\t%B0\t*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state773: // A\t=\t\t"#"\t\n\t%B0\t*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state774: // A\t=\t\t"#"\t\n\t%B0\t*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state775: // A\t=\t\t"#"\t\n\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state776;
    }
    goto state0;

state776: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state658;
    case '{': 
        goto state660;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state776;
    case 9: case ' ': 
        goto state777;
    case 10: 
        goto state778;
    case 13: 
        goto state779;
    case '.': 
        goto state780;
    case ';': 
        goto state782;
    }
    goto state0;

state777: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state778: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state779: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state780: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state781;
    }
    goto state0;

state781: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state658;
    case '{': 
        goto state660;
    case 9: case ' ': 
        goto state777;
    case 10: 
        goto state778;
    case 13: 
        goto state779;
    case '.': 
        goto state780;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state781;
    case ';': 
        goto state782;
    }
    goto state0;

state782: // A\t=\t\t"#"\t\n\t%B0\t*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state783: // A\t=\t\t"#"\t\n\t%B0\t*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state784: // A\t=\t\t"#"\t\n\t%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state785: // A\t=\t\t"#"\t\n\t%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state786: // A\t=\t\t"#"\t\n\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state787;
    }
    goto state0;

state787: // A\t=\t\t"#"\t\n\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state589;
    case '{': 
        goto state665;
    case '0': case '1': 
        goto state787;
    case 9: case ' ': 
        goto state788;
    case 10: 
        goto state789;
    case 13: 
        goto state790;
    case ';': 
        goto state791;
    }
    goto state0;

state788: // A\t=\t\t"#"\t\n\t%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state789: // A\t=\t\t"#"\t\n\t%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state790: // A\t=\t\t"#"\t\n\t%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state791: // A\t=\t\t"#"\t\n\t%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state792: // A\t=\t\t"#"\t\n\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state793;
    }
    goto state0;

state793: // A\t=\t\t"#"\t\n\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state673;
    case '{': 
        goto state675;
    case '0': case '1': 
        goto state793;
    case 9: case ' ': 
        goto state794;
    case 10: 
        goto state795;
    case 13: 
        goto state796;
    case '.': 
        goto state797;
    case ';': 
        goto state799;
    }
    goto state0;

state794: // A\t=\t\t"#"\t\n\t%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state465;
    case 10: 
        goto state466;
    case '"': 
        goto state471;
    case '%': 
        goto state474;
    case 13: 
        goto state478;
    case '(': case '[': 
        goto state480;
    case '/': 
        goto state485;
    case '{': 
        goto state516;
    case '*': 
        goto state717;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state723;
    case ';': 
        goto state729;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state733;
    }
    goto state0;

state795: // A\t=\t\t"#"\t\n\t%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state29;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state796: // A\t=\t\t"#"\t\n\t%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state484;
    }
    goto state0;

state797: // A\t=\t\t"#"\t\n\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state798;
    }
    goto state0;

state798: // A\t=\t\t"#"\t\n\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state673;
    case '{': 
        goto state675;
    case 9: case ' ': 
        goto state794;
    case 10: 
        goto state795;
    case 13: 
        goto state796;
    case '.': 
        goto state797;
    case '0': case '1': 
        goto state798;
    case ';': 
        goto state799;
    }
    goto state0;

state799: // A\t=\t\t"#"\t\n\t%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state800: // A\t=\t\t"#"\t\n\t%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state713;
    case 10: 
        goto state714;
    case 13: 
        goto state715;
    }
    goto state0;

state801: // A\t=\t\t"#"\t\n\t;
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state801;
    case 10: 
        goto state802;
    case 13: 
        goto state803;
    }
    goto state0;

state802: // A\t=\t\t"#"\t\n\t;\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state803: // A\t=\t\t"#"\t\n\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state804;
    }
    goto state0;

state804: // A\t=\t\t"#"\t\n\t;\r\n
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state32;
    case 10: 
        goto state33;
    case 13: 
        goto state34;
    case ';': 
        goto state35;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state38;
    case 9: case ' ': 
        goto state467;
    }
    goto state0;

state805: // A\t=\t\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state464;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state805;
    }
    goto state0;

state806: // A\t=\t\t%
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state807;
    case 'D': case 'd': 
        goto state815;
    case 'X': case 'x': 
        goto state823;
    }
    goto state0;

state807: // A\t=\t\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state808;
    }
    goto state0;

state808: // A\t=\t\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state477;
    case '/': 
        goto state676;
    case '{': 
        goto state678;
    case 10: 
        goto state784;
    case 13: 
        goto state785;
    case ';': 
        goto state800;
    case '0': case '1': 
        goto state808;
    case '-': 
        goto state809;
    case '.': 
        goto state811;
    }
    goto state0;

state809: // A\t=\t\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state810;
    }
    goto state0;

state810: // A\t=\t\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state589;
    case '{': 
        goto state665;
    case 9: case ' ': 
        goto state788;
    case 10: 
        goto state789;
    case 13: 
        goto state790;
    case ';': 
        goto state791;
    case '0': case '1': 
        goto state810;
    }
    goto state0;

state811: // A\t=\t\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state812;
    }
    goto state0;

state812: // A\t=\t\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state673;
    case '{': 
        goto state675;
    case 9: case ' ': 
        goto state794;
    case 10: 
        goto state795;
    case 13: 
        goto state796;
    case ';': 
        goto state799;
    case '0': case '1': 
        goto state812;
    case '.': 
        goto state813;
    }
    goto state0;

state813: // A\t=\t\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state814;
    }
    goto state0;

state814: // A\t=\t\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state673;
    case '{': 
        goto state675;
    case 9: case ' ': 
        goto state794;
    case 10: 
        goto state795;
    case 13: 
        goto state796;
    case ';': 
        goto state799;
    case '.': 
        goto state813;
    case '0': case '1': 
        goto state814;
    }
    goto state0;

state815: // A\t=\t\t%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state816;
    }
    goto state0;

state816: // A\t=\t\t%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state661;
    case '{': 
        goto state663;
    case 9: case ' ': 
        goto state722;
    case 10: 
        goto state767;
    case 13: 
        goto state768;
    case ';': 
        goto state783;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state816;
    case '-': 
        goto state817;
    case '.': 
        goto state819;
    }
    goto state0;

state817: // A\t=\t\t%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state818;
    }
    goto state0;

state818: // A\t=\t\t%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state603;
    case '{': 
        goto state650;
    case 9: case ' ': 
        goto state771;
    case 10: 
        goto state772;
    case 13: 
        goto state773;
    case ';': 
        goto state774;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state818;
    }
    goto state0;

state819: // A\t=\t\t%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state820;
    }
    goto state0;

state820: // A\t=\t\t%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state658;
    case '{': 
        goto state660;
    case 9: case ' ': 
        goto state777;
    case 10: 
        goto state778;
    case 13: 
        goto state779;
    case ';': 
        goto state782;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state820;
    case '.': 
        goto state821;
    }
    goto state0;

state821: // A\t=\t\t%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state822;
    }
    goto state0;

state822: // A\t=\t\t%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state658;
    case '{': 
        goto state660;
    case 9: case ' ': 
        goto state777;
    case 10: 
        goto state778;
    case 13: 
        goto state779;
    case ';': 
        goto state782;
    case '.': 
        goto state821;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state822;
    }
    goto state0;

state823: // A\t=\t\t%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state824;
    }
    goto state0;

state824: // A\t=\t\t%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state635;
    case '{': 
        goto state637;
    case 9: case ' ': 
        goto state728;
    case 10: 
        goto state739;
    case 13: 
        goto state740;
    case ';': 
        goto state755;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state824;
    case '-': 
        goto state825;
    case '.': 
        goto state827;
    }
    goto state0;

state825: // A\t=\t\t%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state826;
    }
    goto state0;

state826: // A\t=\t\t%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state616;
    case '{': 
        goto state624;
    case 9: case ' ': 
        goto state743;
    case 10: 
        goto state744;
    case 13: 
        goto state745;
    case ';': 
        goto state746;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state826;
    }
    goto state0;

state827: // A\t=\t\t%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state828;
    }
    goto state0;

state828: // A\t=\t\t%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state632;
    case '{': 
        goto state634;
    case 9: case ' ': 
        goto state749;
    case 10: 
        goto state750;
    case 13: 
        goto state751;
    case ';': 
        goto state754;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state828;
    case '.': 
        goto state829;
    }
    goto state0;

state829: // A\t=\t\t%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state830;
    }
    goto state0;

state830: // A\t=\t\t%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state632;
    case '{': 
        goto state634;
    case 9: case ' ': 
        goto state749;
    case 10: 
        goto state750;
    case 13: 
        goto state751;
    case ';': 
        goto state754;
    case '.': 
        goto state829;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state830;
    }
    goto state0;

state831: // A\t=\t\t(
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state832: // A\t=\t\t*
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state833;
    case '%': 
        goto state834;
    case '(': case '[': 
        goto state835;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state836;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state842;
    }
    goto state0;

state833: // A\t=\t\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state463;
    }
    goto state0;

state834: // A\t=\t\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state807;
    case 'D': case 'd': 
        goto state815;
    case 'X': case 'x': 
        goto state823;
    }
    goto state0;

state835: // A\t=\t\t*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state836: // A\t=\t\t*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state836;
    case '"': 
        goto state837;
    case '%': 
        goto state838;
    case '(': case '[': 
        goto state839;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state840;
    }
    goto state0;

state837: // A\t=\t\t*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state463;
    }
    goto state0;

state838: // A\t=\t\t*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state807;
    case 'D': case 'd': 
        goto state815;
    case 'X': case 'x': 
        goto state823;
    }
    goto state0;

state839: // A\t=\t\t*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state840: // A\t=\t\t*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state841: // A\t=\t\t*0A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state842: // A\t=\t\t*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state843: // A\t=\t\t0
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state844;
    case '%': 
        goto state845;
    case '(': case '[': 
        goto state846;
    case '*': 
        goto state847;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state848;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state849;
    }
    goto state0;

state844: // A\t=\t\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state463;
    }
    goto state0;

state845: // A\t=\t\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state807;
    case 'D': case 'd': 
        goto state815;
    case 'X': case 'x': 
        goto state823;
    }
    goto state0;

state846: // A\t=\t\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state847: // A\t=\t\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state833;
    case '%': 
        goto state834;
    case '(': case '[': 
        goto state835;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state836;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state842;
    }
    goto state0;

state848: // A\t=\t\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state844;
    case '%': 
        goto state845;
    case '(': case '[': 
        goto state846;
    case '*': 
        goto state847;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state848;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state849;
    }
    goto state0;

state849: // A\t=\t\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state850: // A\t=\t\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state460;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state850;
    case 13: 
        goto state851;
    }
    goto state0;

state851: // A\t=\t\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state460;
    }
    goto state0;

state852: // A\t=\t\tA
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state853: // A\t=\n
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state459;
    }
    goto state0;

state854: // A\t=\r
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state460;
    }
    goto state0;

state855: // A\t="
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state463;
    }
    goto state0;

state856: // A\t=%
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state807;
    case 'D': case 'd': 
        goto state815;
    case 'X': case 'x': 
        goto state823;
    }
    goto state0;

state857: // A\t=(
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state481;
    }
    goto state0;

state858: // A\t=*
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state833;
    case '%': 
        goto state834;
    case '(': case '[': 
        goto state835;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state836;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state842;
    }
    goto state0;

state859: // A\t=/
    m_notify->onDefinedAsIncrementalEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state459;
    case 10: 
        goto state460;
    case 13: 
        goto state461;
    case '"': 
        goto state462;
    case '%': 
        goto state806;
    case '(': case '[': 
        goto state831;
    case '*': 
        goto state832;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state843;
    case ';': 
        goto state850;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state852;
    }
    goto state0;

state860: // A\t=0
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state844;
    case '%': 
        goto state845;
    case '(': case '[': 
        goto state846;
    case '*': 
        goto state847;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state848;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state849;
    }
    goto state0;

state861: // A\t=;
    m_notify->onDefinedAsSetEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state460;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state850;
    case 13: 
        goto state851;
    }
    goto state0;

state862: // A\t=A
    m_notify->onDefinedAsSetEnd(ptr);
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '/': 
        goto state578;
    case '{': 
        goto state622;
    case 9: case ' ': 
        goto state734;
    case 10: 
        goto state735;
    case 13: 
        goto state736;
    case ';': 
        goto state738;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state841;
    }
    goto state0;

state863: // A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state452;
    case 10: 
        goto state453;
    case 13: 
        goto state454;
    case ';': 
        goto state455;
    case '=': 
        goto state457;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state863;
    }
    goto state0;
}

//===========================================================================
bool AbnfParser::stateGroupTail (const char *& ptr) {
    const char * last{nullptr};
    char ch;
    goto state2;

state0: // <FAILED>
    if (last) {
        ptr = last;
        goto state1;
    }
    return false;

state1: // <DONE>
    return true;

state2: // 
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state2;
    case 10: 
        goto state3;
    case 13: 
        goto state4;
    case '"': 
        goto state5;
    case '%': 
        goto state281;
    case '(': case '[': 
        goto state306;
    case '*': 
        goto state307;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state318;
    case ';': 
        goto state325;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state327;
    }
    goto state0;

state3: // \n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state2;
    }
    goto state0;

state4: // \r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state3;
    }
    goto state0;

state5: // "
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state6;
    }
    goto state0;

state6: // "#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state7;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state280;
    }
    goto state0;

state7: // "#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state20;
    case 13: 
        goto state21;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case ';': 
        goto state196;
    }
    goto state0;

state8: // "#"\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state9: // "#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state10: // "#"\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state9;
    }
    goto state0;

state11: // "#"\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state12;
    }
    goto state0;

state12: // "#"\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state7;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state13;
    }
    goto state0;

state13: // "#"\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state7;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state13;
    }
    goto state0;

state14: // "#"\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state15;
    case 'D': case 'd': 
        goto state201;
    case 'X': case 'x': 
        goto state207;
    }
    goto state0;

state15: // "#"\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state16;
    }
    goto state0;

state16: // "#"\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state16;
    case 9: case ' ': 
        goto state17;
    case ')': case ']': 
        goto state72;
    case '/': 
        goto state164;
    case 10: 
        goto state263;
    case 13: 
        goto state264;
    case '-': 
        goto state265;
    case '.': 
        goto state271;
    case ';': 
        goto state279;
    }
    goto state0;

state17: // "#"\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state18: // "#"\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state19: // "#"\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state20;
    case 13: 
        goto state21;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case ';': 
        goto state196;
    }
    goto state0;

state20: // "#"\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state21: // "#"\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state22: // "#"\t%B0\t(\*)
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state23: // "#"\t%B0\t(\*/
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state24: // "#"\t%B0\t(\*/\t
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state25: // "#"\t%B0\t(\*/\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    }
    goto state0;

state26: // "#"\t%B0\t(\*/\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state25;
    }
    goto state0;

state27: // "#"\t%B0\t(\*/\t"
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state28;
    }
    goto state0;

state28: // "#"\t%B0\t(\*/\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state29;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state195;
    }
    goto state0;

state29: // "#"\t%B0\t(\*/\t"#"
    m_notify->onCharValEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state42;
    case 13: 
        goto state43;
    case ';': 
        goto state44;
    }
    goto state0;

state30: // "#"\t%B0\t(\*/\t"#"\t
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state31: // "#"\t%B0\t(\*/\t"#"\t\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state32: // "#"\t%B0\t(\*/\t"#"\t\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state31;
    }
    goto state0;

state33: // "#"\t%B0\t(\*/\t"#"\t"
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state34;
    }
    goto state0;

state34: // "#"\t%B0\t(\*/\t"#"\t"#
    m_notify->onCharValSequenceStart(ptr - 1);
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state29;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state35;
    }
    goto state0;

state35: // "#"\t%B0\t(\*/\t"#"\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state29;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state35;
    }
    goto state0;

state36: // "#"\t%B0\t(\*/\t"#"\t%
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state37;
    case 'D': case 'd': 
        goto state49;
    case 'X': case 'x': 
        goto state55;
    }
    goto state0;

state37: // "#"\t%B0\t(\*/\t"#"\t%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state38;
    }
    goto state0;

state38: // "#"\t%B0\t(\*/\t"#"\t%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state38;
    case 9: case ' ': 
        goto state39;
    case 10: 
        goto state70;
    case 13: 
        goto state71;
    case ')': case ']': 
        goto state72;
    case '/': 
        goto state164;
    case ';': 
        goto state165;
    case '-': 
        goto state189;
    case '.': 
        goto state191;
    }
    goto state0;

state39: // "#"\t%B0\t(\*/\t"#"\t%B0\t
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state40: // "#"\t%B0\t(\*/\t"#"\t%B0\t(
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state41: // "#"\t%B0\t(\*/\t"#"\t%B0\t(\*
    m_notify->onGroupEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state42;
    case 13: 
        goto state43;
    case ';': 
        goto state44;
    }
    goto state0;

state42: // "#"\t%B0\t(\*/\t"#"\t%B0\t(\*\n
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state43: // "#"\t%B0\t(\*/\t"#"\t%B0\t(\*\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state44: // "#"\t%B0\t(\*/\t"#"\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state45: // "#"\t%B0\t(\*/\t"#"\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state46: // "#"\t%B0\t(\*/\t"#"\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state47;
    case '%': 
        goto state48;
    case '(': case '[': 
        goto state174;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state175;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state180;
    }
    goto state0;

state47: // "#"\t%B0\t(\*/\t"#"\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state34;
    }
    goto state0;

state48: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state37;
    case 'D': case 'd': 
        goto state49;
    case 'X': case 'x': 
        goto state55;
    }
    goto state0;

state49: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state50;
    }
    goto state0;

state50: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state50;
    case 9: case ' ': 
        goto state51;
    case 10: 
        goto state86;
    case 13: 
        goto state87;
    case ')': case ']': 
        goto state88;
    case '/': 
        goto state151;
    case ';': 
        goto state152;
    case '-': 
        goto state183;
    case '.': 
        goto state185;
    }
    goto state0;

state51: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state52: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state53;
    case '%': 
        goto state54;
    case '(': case '[': 
        goto state172;
    case '*': 
        goto state173;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state181;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state182;
    }
    goto state0;

state53: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state34;
    }
    goto state0;

state54: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state37;
    case 'D': case 'd': 
        goto state49;
    case 'X': case 'x': 
        goto state55;
    }
    goto state0;

state55: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state56;
    }
    goto state0;

state56: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state56;
    case 9: case ' ': 
        goto state57;
    case 10: 
        goto state101;
    case 13: 
        goto state102;
    case ')': case ']': 
        goto state103;
    case '/': 
        goto state127;
    case ';': 
        goto state128;
    case '-': 
        goto state166;
    case '.': 
        goto state168;
    }
    goto state0;

state57: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state58: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state31;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state58;
    case 13: 
        goto state59;
    }
    goto state0;

state59: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state31;
    }
    goto state0;

state60: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state65;
    case '/': 
        goto state66;
    case ';': 
        goto state115;
    }
    goto state0;

state61: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state62: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state63: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state64: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA)
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state65: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state65;
    case '/': 
        goto state66;
    case ';': 
        goto state115;
    }
    goto state0;

state66: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/
    m_notify->onRulerefEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state67: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state68;
    case 'D': case 'd': 
        goto state84;
    case 'X': case 'x': 
        goto state99;
    }
    goto state0;

state68: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state69;
    }
    goto state0;

state69: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state39;
    case '0': case '1': 
        goto state69;
    case 10: 
        goto state70;
    case 13: 
        goto state71;
    case ')': case ']': 
        goto state72;
    case '-': 
        goto state73;
    case '.': 
        goto state154;
    case '/': 
        goto state164;
    case ';': 
        goto state165;
    }
    goto state0;

state70: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state71: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state72: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0)
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state73: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state74;
    }
    goto state0;

state74: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state74;
    case 9: case ' ': 
        goto state75;
    case 10: 
        goto state76;
    case 13: 
        goto state77;
    case ')': case ']': 
        goto state78;
    case '/': 
        goto state79;
    case ';': 
        goto state153;
    }
    goto state0;

state75: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state76: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state77: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state78: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0)
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state79: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/
    m_notify->onBinValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state80: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/(
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state81: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state82;
    case '%': 
        goto state83;
    case '(': case '[': 
        goto state131;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state132;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state137;
    }
    goto state0;

state82: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state28;
    }
    goto state0;

state83: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state68;
    case 'D': case 'd': 
        goto state84;
    case 'X': case 'x': 
        goto state99;
    }
    goto state0;

state84: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state85;
    }
    goto state0;

state85: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state51;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state85;
    case 10: 
        goto state86;
    case 13: 
        goto state87;
    case ')': case ']': 
        goto state88;
    case '-': 
        goto state89;
    case '.': 
        goto state141;
    case '/': 
        goto state151;
    case ';': 
        goto state152;
    }
    goto state0;

state86: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state87: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state88: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0)
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state89: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state90;
    }
    goto state0;

state90: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state90;
    case 9: case ' ': 
        goto state91;
    case 10: 
        goto state92;
    case 13: 
        goto state93;
    case ')': case ']': 
        goto state94;
    case '/': 
        goto state95;
    case ';': 
        goto state140;
    }
    goto state0;

state91: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state92: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state93: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state94: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0)
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state95: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/
    m_notify->onDecValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state96: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state97;
    case '%': 
        goto state98;
    case '(': case '[': 
        goto state129;
    case '*': 
        goto state130;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state138;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state139;
    }
    goto state0;

state97: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state28;
    }
    goto state0;

state98: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state68;
    case 'D': case 'd': 
        goto state84;
    case 'X': case 'x': 
        goto state99;
    }
    goto state0;

state99: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state100;
    }
    goto state0;

state100: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state100;
    case 10: 
        goto state101;
    case 13: 
        goto state102;
    case ')': case ']': 
        goto state103;
    case '-': 
        goto state104;
    case '.': 
        goto state117;
    case '/': 
        goto state127;
    case ';': 
        goto state128;
    }
    goto state0;

state101: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state102: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state103: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0)
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state104: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state105;
    }
    goto state0;

state105: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state105;
    case 9: case ' ': 
        goto state106;
    case 10: 
        goto state107;
    case 13: 
        goto state108;
    case ')': case ']': 
        goto state109;
    case '/': 
        goto state110;
    case ';': 
        goto state116;
    }
    goto state0;

state106: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state107: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state108: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state109: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0)
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state110: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/
    m_notify->onHexValAltSecondEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state111: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state25;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state111;
    case 13: 
        goto state112;
    }
    goto state0;

state112: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state25;
    }
    goto state0;

state113: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state114: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state115: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0/A-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state116: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state117: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state118;
    }
    goto state0;

state118: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state118;
    case 9: case ' ': 
        goto state119;
    case 10: 
        goto state120;
    case 13: 
        goto state121;
    case ')': case ']': 
        goto state122;
    case '.': 
        goto state123;
    case '/': 
        goto state125;
    case ';': 
        goto state126;
    }
    goto state0;

state119: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state120: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state121: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state122: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0)
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state123: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state124;
    }
    goto state0;

state124: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state119;
    case 10: 
        goto state120;
    case 13: 
        goto state121;
    case ')': case ']': 
        goto state122;
    case '.': 
        goto state123;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state124;
    case '/': 
        goto state125;
    case ';': 
        goto state126;
    }
    goto state0;

state125: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0/
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state126: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state127: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0/
    m_notify->onHexValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state128: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state129: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state130: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state82;
    case '%': 
        goto state83;
    case '(': case '[': 
        goto state131;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state132;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state137;
    }
    goto state0;

state131: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state132: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state132;
    case '"': 
        goto state133;
    case '%': 
        goto state134;
    case '(': case '[': 
        goto state135;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state136;
    }
    goto state0;

state133: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state28;
    }
    goto state0;

state134: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state68;
    case 'D': case 'd': 
        goto state84;
    case 'X': case 'x': 
        goto state99;
    }
    goto state0;

state135: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state136: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state137: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state138: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state97;
    case '%': 
        goto state98;
    case '(': case '[': 
        goto state129;
    case '*': 
        goto state130;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state138;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state139;
    }
    goto state0;

state139: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0/00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state140: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state141: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state142;
    }
    goto state0;

state142: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state142;
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    case 13: 
        goto state145;
    case ')': case ']': 
        goto state146;
    case '.': 
        goto state147;
    case '/': 
        goto state149;
    case ';': 
        goto state150;
    }
    goto state0;

state143: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state144: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state145: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state146: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0)
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state147: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state148;
    }
    goto state0;

state148: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    case 13: 
        goto state145;
    case ')': case ']': 
        goto state146;
    case '.': 
        goto state147;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state148;
    case '/': 
        goto state149;
    case ';': 
        goto state150;
    }
    goto state0;

state149: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0/
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state150: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state151: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0/
    m_notify->onDecValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state152: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0/*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state153: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state154: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state155;
    }
    goto state0;

state155: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state155;
    case 9: case ' ': 
        goto state156;
    case 10: 
        goto state157;
    case 13: 
        goto state158;
    case ')': case ']': 
        goto state159;
    case '.': 
        goto state160;
    case '/': 
        goto state162;
    case ';': 
        goto state163;
    }
    goto state0;

state156: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case 9: case ' ': 
        goto state30;
    case 10: 
        goto state31;
    case 13: 
        goto state32;
    case '"': 
        goto state33;
    case '%': 
        goto state36;
    case '(': case '[': 
        goto state40;
    case '*': 
        goto state46;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state52;
    case ';': 
        goto state58;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state60;
    }
    goto state0;

state157: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state30;
    }
    goto state0;

state158: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    }
    goto state0;

state159: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0)
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    m_notify->onAlternationEnd(ptr);
    last = ptr;
    ch = *ptr++;
    switch (ch) {
    case 0: 
        goto state1;
    }
    goto state0;

state160: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state161;
    }
    goto state0;

state161: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state156;
    case 10: 
        goto state157;
    case 13: 
        goto state158;
    case ')': case ']': 
        goto state159;
    case '.': 
        goto state160;
    case '0': case '1': 
        goto state161;
    case '/': 
        goto state162;
    case ';': 
        goto state163;
    }
    goto state0;

state162: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0.0/
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state163: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state164: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0/
    m_notify->onBinValSimpleEnd(ptr);
    m_notify->onConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state67;
    case '(': case '[': 
        goto state80;
    case '*': 
        goto state81;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state96;
    case ';': 
        goto state111;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state113;
    }
    goto state0;

state165: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0\tA-/%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state42;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state166: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state167;
    }
    goto state0;

state167: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state106;
    case 10: 
        goto state107;
    case 13: 
        goto state108;
    case ')': case ']': 
        goto state109;
    case '/': 
        goto state110;
    case ';': 
        goto state116;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state167;
    }
    goto state0;

state168: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state169;
    }
    goto state0;

state169: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state119;
    case 10: 
        goto state120;
    case 13: 
        goto state121;
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case ';': 
        goto state126;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state169;
    case '.': 
        goto state170;
    }
    goto state0;

state170: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state171;
    }
    goto state0;

state171: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state119;
    case 10: 
        goto state120;
    case 13: 
        goto state121;
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case ';': 
        goto state126;
    case '.': 
        goto state170;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state171;
    }
    goto state0;

state172: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state173: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state47;
    case '%': 
        goto state48;
    case '(': case '[': 
        goto state174;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state175;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state180;
    }
    goto state0;

state174: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state175: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state175;
    case '"': 
        goto state176;
    case '%': 
        goto state177;
    case '(': case '[': 
        goto state178;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state179;
    }
    goto state0;

state176: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state34;
    }
    goto state0;

state177: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state37;
    case 'D': case 'd': 
        goto state49;
    case 'X': case 'x': 
        goto state55;
    }
    goto state0;

state178: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state41;
    }
    goto state0;

state179: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state65;
    case '/': 
        goto state66;
    case ';': 
        goto state115;
    }
    goto state0;

state180: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state65;
    case '/': 
        goto state66;
    case ';': 
        goto state115;
    }
    goto state0;

state181: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state53;
    case '%': 
        goto state54;
    case '(': case '[': 
        goto state172;
    case '*': 
        goto state173;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state181;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state182;
    }
    goto state0;

state182: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ')': case ']': 
        goto state64;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state65;
    case '/': 
        goto state66;
    case ';': 
        goto state115;
    }
    goto state0;

state183: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state184;
    }
    goto state0;

state184: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state91;
    case 10: 
        goto state92;
    case 13: 
        goto state93;
    case ')': case ']': 
        goto state94;
    case '/': 
        goto state95;
    case ';': 
        goto state140;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state184;
    }
    goto state0;

state185: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state186;
    }
    goto state0;

state186: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    case 13: 
        goto state145;
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case ';': 
        goto state150;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state186;
    case '.': 
        goto state187;
    }
    goto state0;

state187: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state188;
    }
    goto state0;

state188: // "#"\t%B0\t(\*/\t"#"\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    case 13: 
        goto state145;
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case ';': 
        goto state150;
    case '.': 
        goto state187;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state188;
    }
    goto state0;

state189: // "#"\t%B0\t(\*/\t"#"\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state190;
    }
    goto state0;

state190: // "#"\t%B0\t(\*/\t"#"\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state75;
    case 10: 
        goto state76;
    case 13: 
        goto state77;
    case ')': case ']': 
        goto state78;
    case '/': 
        goto state79;
    case ';': 
        goto state153;
    case '0': case '1': 
        goto state190;
    }
    goto state0;

state191: // "#"\t%B0\t(\*/\t"#"\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state192;
    }
    goto state0;

state192: // "#"\t%B0\t(\*/\t"#"\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state156;
    case 10: 
        goto state157;
    case 13: 
        goto state158;
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case ';': 
        goto state163;
    case '0': case '1': 
        goto state192;
    case '.': 
        goto state193;
    }
    goto state0;

state193: // "#"\t%B0\t(\*/\t"#"\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state194;
    }
    goto state0;

state194: // "#"\t%B0\t(\*/\t"#"\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state156;
    case 10: 
        goto state157;
    case 13: 
        goto state158;
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case ';': 
        goto state163;
    case '.': 
        goto state193;
    case '0': case '1': 
        goto state194;
    }
    goto state0;

state195: // "#"\t%B0\t(\*/\t"##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state29;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state195;
    }
    goto state0;

state196: // "#"\t%B0\t(\*;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state197: // "#"\t%B0\t(\*;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state198: // "#"\t%B0\t*
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state199;
    case '%': 
        goto state200;
    case '(': case '[': 
        goto state237;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state238;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state243;
    }
    goto state0;

state199: // "#"\t%B0\t*"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state12;
    }
    goto state0;

state200: // "#"\t%B0\t*%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state15;
    case 'D': case 'd': 
        goto state201;
    case 'X': case 'x': 
        goto state207;
    }
    goto state0;

state201: // "#"\t%B0\t*%D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state202;
    }
    goto state0;

state202: // "#"\t%B0\t*%D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state88;
    case '/': 
        goto state151;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state202;
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state246;
    case 13: 
        goto state247;
    case '-': 
        goto state248;
    case '.': 
        goto state254;
    case ';': 
        goto state262;
    }
    goto state0;

state203: // "#"\t%B0\t*%D0\t
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state204: // "#"\t%B0\t*%D0\t0
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state205;
    case '%': 
        goto state206;
    case '(': case '[': 
        goto state235;
    case '*': 
        goto state236;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state244;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state245;
    }
    goto state0;

state205: // "#"\t%B0\t*%D0\t0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state12;
    }
    goto state0;

state206: // "#"\t%B0\t*%D0\t0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state15;
    case 'D': case 'd': 
        goto state201;
    case 'X': case 'x': 
        goto state207;
    }
    goto state0;

state207: // "#"\t%B0\t*%D0\t0%X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state208;
    }
    goto state0;

state208: // "#"\t%B0\t*%D0\t0%X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state103;
    case '/': 
        goto state127;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state208;
    case 9: case ' ': 
        goto state209;
    case 10: 
        goto state218;
    case 13: 
        goto state219;
    case '-': 
        goto state220;
    case '.': 
        goto state226;
    case ';': 
        goto state234;
    }
    goto state0;

state209: // "#"\t%B0\t*%D0\t0%X0\t
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state210: // "#"\t%B0\t*%D0\t0%X0\t;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state9;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state210;
    case 13: 
        goto state211;
    }
    goto state0;

state211: // "#"\t%B0\t*%D0\t0%X0\t;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state9;
    }
    goto state0;

state212: // "#"\t%B0\t*%D0\t0%X0\tA
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state216;
    case ';': 
        goto state217;
    }
    goto state0;

state213: // "#"\t%B0\t*%D0\t0%X0\tA\t
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state214: // "#"\t%B0\t*%D0\t0%X0\tA\n
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state215: // "#"\t%B0\t*%D0\t0%X0\tA\r
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state216: // "#"\t%B0\t*%D0\t0%X0\tA-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state216;
    case ';': 
        goto state217;
    }
    goto state0;

state217: // "#"\t%B0\t*%D0\t0%X0\tA-;
    m_notify->onRulerefEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state218: // "#"\t%B0\t*%D0\t0%X0\n
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state219: // "#"\t%B0\t*%D0\t0%X0\r
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state220: // "#"\t%B0\t*%D0\t0%X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state221;
    }
    goto state0;

state221: // "#"\t%B0\t*%D0\t0%X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state109;
    case '/': 
        goto state110;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state221;
    case 9: case ' ': 
        goto state222;
    case 10: 
        goto state223;
    case 13: 
        goto state224;
    case ';': 
        goto state225;
    }
    goto state0;

state222: // "#"\t%B0\t*%D0\t0%X0-0\t
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state223: // "#"\t%B0\t*%D0\t0%X0-0\n
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state224: // "#"\t%B0\t*%D0\t0%X0-0\r
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state225: // "#"\t%B0\t*%D0\t0%X0-0;
    m_notify->onHexValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state226: // "#"\t%B0\t*%D0\t0%X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state227;
    }
    goto state0;

state227: // "#"\t%B0\t*%D0\t0%X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state227;
    case 9: case ' ': 
        goto state228;
    case 10: 
        goto state229;
    case 13: 
        goto state230;
    case '.': 
        goto state231;
    case ';': 
        goto state233;
    }
    goto state0;

state228: // "#"\t%B0\t*%D0\t0%X0.0\t
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state229: // "#"\t%B0\t*%D0\t0%X0.0\n
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state230: // "#"\t%B0\t*%D0\t0%X0.0\r
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state231: // "#"\t%B0\t*%D0\t0%X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state232;
    }
    goto state0;

state232: // "#"\t%B0\t*%D0\t0%X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case 9: case ' ': 
        goto state228;
    case 10: 
        goto state229;
    case 13: 
        goto state230;
    case '.': 
        goto state231;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state232;
    case ';': 
        goto state233;
    }
    goto state0;

state233: // "#"\t%B0\t*%D0\t0%X0.0.0;
    m_notify->onHexValConcatEachEnd(ptr);
    m_notify->onHexValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state234: // "#"\t%B0\t*%D0\t0%X0;
    m_notify->onHexValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state235: // "#"\t%B0\t*%D0\t0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state236: // "#"\t%B0\t*%D0\t0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state199;
    case '%': 
        goto state200;
    case '(': case '[': 
        goto state237;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state238;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state243;
    }
    goto state0;

state237: // "#"\t%B0\t*%D0\t0*(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state238: // "#"\t%B0\t*%D0\t0*0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state238;
    case '"': 
        goto state239;
    case '%': 
        goto state240;
    case '(': case '[': 
        goto state241;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state242;
    }
    goto state0;

state239: // "#"\t%B0\t*%D0\t0*0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state12;
    }
    goto state0;

state240: // "#"\t%B0\t*%D0\t0*0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state15;
    case 'D': case 'd': 
        goto state201;
    case 'X': case 'x': 
        goto state207;
    }
    goto state0;

state241: // "#"\t%B0\t*%D0\t0*0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state242: // "#"\t%B0\t*%D0\t0*0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state216;
    case ';': 
        goto state217;
    }
    goto state0;

state243: // "#"\t%B0\t*%D0\t0*A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state216;
    case ';': 
        goto state217;
    }
    goto state0;

state244: // "#"\t%B0\t*%D0\t00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state205;
    case '%': 
        goto state206;
    case '(': case '[': 
        goto state235;
    case '*': 
        goto state236;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state244;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state245;
    }
    goto state0;

state245: // "#"\t%B0\t*%D0\t00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state216;
    case ';': 
        goto state217;
    }
    goto state0;

state246: // "#"\t%B0\t*%D0\n
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state247: // "#"\t%B0\t*%D0\r
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state248: // "#"\t%B0\t*%D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state249;
    }
    goto state0;

state249: // "#"\t%B0\t*%D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state94;
    case '/': 
        goto state95;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state249;
    case 9: case ' ': 
        goto state250;
    case 10: 
        goto state251;
    case 13: 
        goto state252;
    case ';': 
        goto state253;
    }
    goto state0;

state250: // "#"\t%B0\t*%D0-0\t
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state251: // "#"\t%B0\t*%D0-0\n
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state252: // "#"\t%B0\t*%D0-0\r
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state253: // "#"\t%B0\t*%D0-0;
    m_notify->onDecValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state254: // "#"\t%B0\t*%D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state255;
    }
    goto state0;

state255: // "#"\t%B0\t*%D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state255;
    case 9: case ' ': 
        goto state256;
    case 10: 
        goto state257;
    case 13: 
        goto state258;
    case '.': 
        goto state259;
    case ';': 
        goto state261;
    }
    goto state0;

state256: // "#"\t%B0\t*%D0.0\t
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state257: // "#"\t%B0\t*%D0.0\n
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state258: // "#"\t%B0\t*%D0.0\r
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state259: // "#"\t%B0\t*%D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state260;
    }
    goto state0;

state260: // "#"\t%B0\t*%D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case 9: case ' ': 
        goto state256;
    case 10: 
        goto state257;
    case 13: 
        goto state258;
    case '.': 
        goto state259;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state260;
    case ';': 
        goto state261;
    }
    goto state0;

state261: // "#"\t%B0\t*%D0.0.0;
    m_notify->onDecValConcatEachEnd(ptr);
    m_notify->onDecValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state262: // "#"\t%B0\t*%D0;
    m_notify->onDecValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state263: // "#"\t%B0\n
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state264: // "#"\t%B0\r
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state265: // "#"\t%B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state266;
    }
    goto state0;

state266: // "#"\t%B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state78;
    case '/': 
        goto state79;
    case '0': case '1': 
        goto state266;
    case 9: case ' ': 
        goto state267;
    case 10: 
        goto state268;
    case 13: 
        goto state269;
    case ';': 
        goto state270;
    }
    goto state0;

state267: // "#"\t%B0-0\t
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state268: // "#"\t%B0-0\n
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state269: // "#"\t%B0-0\r
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state270: // "#"\t%B0-0;
    m_notify->onBinValAltSecondEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state271: // "#"\t%B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state272;
    }
    goto state0;

state272: // "#"\t%B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case '0': case '1': 
        goto state272;
    case 9: case ' ': 
        goto state273;
    case 10: 
        goto state274;
    case 13: 
        goto state275;
    case '.': 
        goto state276;
    case ';': 
        goto state278;
    }
    goto state0;

state273: // "#"\t%B0.0\t
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    case 10: 
        goto state9;
    case 13: 
        goto state10;
    case '"': 
        goto state11;
    case '%': 
        goto state14;
    case '(': case '[': 
        goto state18;
    case ')': case ']': 
        goto state22;
    case '/': 
        goto state23;
    case '*': 
        goto state198;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state204;
    case ';': 
        goto state210;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state212;
    }
    goto state0;

state274: // "#"\t%B0.0\n
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state8;
    }
    goto state0;

state275: // "#"\t%B0.0\r
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    }
    goto state0;

state276: // "#"\t%B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state277;
    }
    goto state0;

state277: // "#"\t%B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case 9: case ' ': 
        goto state273;
    case 10: 
        goto state274;
    case 13: 
        goto state275;
    case '.': 
        goto state276;
    case '0': case '1': 
        goto state277;
    case ';': 
        goto state278;
    }
    goto state0;

state278: // "#"\t%B0.0.0;
    m_notify->onBinValConcatEachEnd(ptr);
    m_notify->onBinValConcatenationEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state279: // "#"\t%B0;
    m_notify->onBinValSimpleEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state20;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state196;
    case 13: 
        goto state197;
    }
    goto state0;

state280: // "##
    m_notify->onCharValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state7;
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state280;
    }
    goto state0;

state281: // %
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state282;
    case 'D': case 'd': 
        goto state290;
    case 'X': case 'x': 
        goto state298;
    }
    goto state0;

state282: // %B
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state283;
    }
    goto state0;

state283: // %B0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case 9: case ' ': 
        goto state17;
    case ')': case ']': 
        goto state72;
    case '/': 
        goto state164;
    case 10: 
        goto state263;
    case 13: 
        goto state264;
    case ';': 
        goto state279;
    case '0': case '1': 
        goto state283;
    case '-': 
        goto state284;
    case '.': 
        goto state286;
    }
    goto state0;

state284: // %B0-
    m_notify->onBinValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state285;
    }
    goto state0;

state285: // %B0-0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state78;
    case '/': 
        goto state79;
    case 9: case ' ': 
        goto state267;
    case 10: 
        goto state268;
    case 13: 
        goto state269;
    case ';': 
        goto state270;
    case '0': case '1': 
        goto state285;
    }
    goto state0;

state286: // %B0.
    m_notify->onBinValConcatenationStart(ptr - 1);
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state287;
    }
    goto state0;

state287: // %B0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case 9: case ' ': 
        goto state273;
    case 10: 
        goto state274;
    case 13: 
        goto state275;
    case ';': 
        goto state278;
    case '0': case '1': 
        goto state287;
    case '.': 
        goto state288;
    }
    goto state0;

state288: // %B0.0.
    m_notify->onBinValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': 
        goto state289;
    }
    goto state0;

state289: // %B0.0.0
    m_notify->onBinValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state159;
    case '/': 
        goto state162;
    case 9: case ' ': 
        goto state273;
    case 10: 
        goto state274;
    case 13: 
        goto state275;
    case ';': 
        goto state278;
    case '.': 
        goto state288;
    case '0': case '1': 
        goto state289;
    }
    goto state0;

state290: // %D
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state291;
    }
    goto state0;

state291: // %D0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state88;
    case '/': 
        goto state151;
    case 9: case ' ': 
        goto state203;
    case 10: 
        goto state246;
    case 13: 
        goto state247;
    case ';': 
        goto state262;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state291;
    case '-': 
        goto state292;
    case '.': 
        goto state294;
    }
    goto state0;

state292: // %D0-
    m_notify->onDecValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state293;
    }
    goto state0;

state293: // %D0-0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state94;
    case '/': 
        goto state95;
    case 9: case ' ': 
        goto state250;
    case 10: 
        goto state251;
    case 13: 
        goto state252;
    case ';': 
        goto state253;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state293;
    }
    goto state0;

state294: // %D0.
    m_notify->onDecValConcatenationStart(ptr - 1);
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state295;
    }
    goto state0;

state295: // %D0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case 9: case ' ': 
        goto state256;
    case 10: 
        goto state257;
    case 13: 
        goto state258;
    case ';': 
        goto state261;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state295;
    case '.': 
        goto state296;
    }
    goto state0;

state296: // %D0.0.
    m_notify->onDecValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state297;
    }
    goto state0;

state297: // %D0.0.0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state146;
    case '/': 
        goto state149;
    case 9: case ' ': 
        goto state256;
    case 10: 
        goto state257;
    case 13: 
        goto state258;
    case ';': 
        goto state261;
    case '.': 
        goto state296;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state297;
    }
    goto state0;

state298: // %X
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state299;
    }
    goto state0;

state299: // %X0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state103;
    case '/': 
        goto state127;
    case 9: case ' ': 
        goto state209;
    case 10: 
        goto state218;
    case 13: 
        goto state219;
    case ';': 
        goto state234;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state299;
    case '-': 
        goto state300;
    case '.': 
        goto state302;
    }
    goto state0;

state300: // %X0-
    m_notify->onHexValAltFirstEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state301;
    }
    goto state0;

state301: // %X0-0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state109;
    case '/': 
        goto state110;
    case 9: case ' ': 
        goto state222;
    case 10: 
        goto state223;
    case 13: 
        goto state224;
    case ';': 
        goto state225;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state301;
    }
    goto state0;

state302: // %X0.
    m_notify->onHexValConcatenationStart(ptr - 1);
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state303;
    }
    goto state0;

state303: // %X0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case 9: case ' ': 
        goto state228;
    case 10: 
        goto state229;
    case 13: 
        goto state230;
    case ';': 
        goto state233;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state303;
    case '.': 
        goto state304;
    }
    goto state0;

state304: // %X0.0.
    m_notify->onHexValConcatEachEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state305;
    }
    goto state0;

state305: // %X0.0.0
    m_notify->onHexValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state122;
    case '/': 
        goto state125;
    case 9: case ' ': 
        goto state228;
    case 10: 
        goto state229;
    case 13: 
        goto state230;
    case ';': 
        goto state233;
    case '.': 
        goto state304;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state305;
    }
    goto state0;

state306: // (
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state307: // *
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state308;
    case '%': 
        goto state309;
    case '(': case '[': 
        goto state310;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state311;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state317;
    }
    goto state0;

state308: // *"
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state6;
    }
    goto state0;

state309: // *%
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state282;
    case 'D': case 'd': 
        goto state290;
    case 'X': case 'x': 
        goto state298;
    }
    goto state0;

state310: // *(
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state311: // *0
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state311;
    case '"': 
        goto state312;
    case '%': 
        goto state313;
    case '(': case '[': 
        goto state314;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state315;
    }
    goto state0;

state312: // *0"
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state6;
    }
    goto state0;

state313: // *0%
    m_notify->onRepeatMaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state282;
    case 'D': case 'd': 
        goto state290;
    case 'X': case 'x': 
        goto state298;
    }
    goto state0;

state314: // *0(
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state315: // *0A
    m_notify->onRepeatMaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case ';': 
        goto state217;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state316;
    }
    goto state0;

state316: // *0A-
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case ';': 
        goto state217;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state316;
    }
    goto state0;

state317: // *A
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case ';': 
        goto state217;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state316;
    }
    goto state0;

state318: // 0
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state319;
    case '%': 
        goto state320;
    case '(': case '[': 
        goto state321;
    case '*': 
        goto state322;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state323;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state324;
    }
    goto state0;

state319: // 0"
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case '#': case '$': case '%': case '&': case '\'': case '(':
    case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^':
    case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': case '{': case '|':
    case '}': case '~': 
        goto state6;
    }
    goto state0;

state320: // 0%
    m_notify->onRepeatMinmaxEnd(ptr);
    ch = *ptr++;
    switch (ch) {
    case 'B': case 'b': 
        goto state282;
    case 'D': case 'd': 
        goto state290;
    case 'X': case 'x': 
        goto state298;
    }
    goto state0;

state321: // 0(
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onGroupStart(ptr - 1);
    if (stateGroupTail(ptr)) {
        goto state19;
    }
    goto state0;

state322: // 0*
    m_notify->onRepeatRangeStart(ptr - 1);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state308;
    case '%': 
        goto state309;
    case '(': case '[': 
        goto state310;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state311;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state317;
    }
    goto state0;

state323: // 00
    m_notify->onDecValSequenceChar(ch);
    ch = *ptr++;
    switch (ch) {
    case '"': 
        goto state319;
    case '%': 
        goto state320;
    case '(': case '[': 
        goto state321;
    case '*': 
        goto state322;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state323;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state324;
    }
    goto state0;

state324: // 00A
    m_notify->onRepeatMinmaxEnd(ptr);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case ';': 
        goto state217;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state316;
    }
    goto state0;

state325: // ;
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state3;
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '[': case '\\': case ']': case '^': case '_': case '`':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': case '{': case '|': case '}': case '~':
        goto state325;
    case 13: 
        goto state326;
    }
    goto state0;

state326: // ;\r
    ch = *ptr++;
    switch (ch) {
    case 10: 
        goto state3;
    }
    goto state0;

state327: // A
    m_notify->onAlternationStart(ptr - 1);
    m_notify->onConcatenationStart(ptr - 1);
    m_notify->onRepetitionStart(ptr - 1);
    m_notify->onRulenameStart(ptr - 1);
    m_notify->onRulenameChar(ch);
    ch = *ptr++;
    switch (ch) {
    case ')': case ']': 
        goto state64;
    case '/': 
        goto state66;
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state214;
    case 13: 
        goto state215;
    case ';': 
        goto state217;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
    case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q':
    case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state316;
    }
    goto state0;
}

