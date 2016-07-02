// abnfsyntax.cpp - pargen
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   AbnfParser
*
*   Normalized ABNF of syntax being checked (recursive rules are marked 
*       with asterisks):
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
*   alternation* = ( concatenation *( *c-wsp %x2f *c-wsp concatenation ) ) 
*   bin-val = ( ( %x62 / %x42 ) 1*BIT *1( 1*( %x2e 1*BIT ) / ( %x2d 1*BIT ) ) 
*       ) 
*   c-nl = ( comment / CRLF ) 
*   c-wsp = ( WSP / ( c-nl WSP ) ) 
*   char-val = ( DQUOTE *( %x20 / %x21 / %x23 / %x24 / %x25 / %x26 / %x27 / 
*       %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / 
*       %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / 
*       %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / 
*       %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / 
*       %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / 
*       %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / 
*       %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / 
*       %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / 
*       %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e ) DQUOTE ) 
*   comment = ( %x3b *( WSP / VCHAR ) CRLF ) 
*   concatenation = ( repetition *( 1*c-wsp repetition ) ) 
*   dec-val = ( ( %x64 / %x44 ) 1*DIGIT *1( 1*( %x2e 1*DIGIT ) / ( %x2d 
*       1*DIGIT ) ) ) 
*   defined-as = ( *c-wsp ( %x3d / ( %x2f %x3d ) ) *c-wsp ) 
*   element = ( rulename / group / option / char-val / num-val ) 
*   elements = ( alternation *c-wsp ) 
*   group = ( %x28 *c-wsp alternation *c-wsp %x29 ) 
*   hex-val = ( ( %x78 / %x58 ) 1*HEXDIG *1( 1*( %x2e 1*HEXDIG ) / ( %x2d 
*       1*HEXDIG ) ) ) 
*   num-val = ( %x25 ( bin-val / dec-val / hex-val ) ) 
*   option = ( %x5b *c-wsp alternation *c-wsp %x5d ) 
*   repeat = ( 1*DIGIT / ( *DIGIT %x2a *DIGIT ) ) 
*   repetition = ( *1repeat element ) 
*   rule = ( rulename defined-as elements c-nl ) 
*   rulelist = 1*( rule / ( *c-wsp c-nl ) ) 
*   rulename = ( ALPHA *( ALPHA / DIGIT / %x2d ) ) 
*
***/

//===========================================================================
bool AbnfParser::checkSyntax (const char src[]) {
    const char * ptr = src;
    goto state2;

state0: // <FAILED>
    return false;

state1: // <DONE>
    return true;

state2: // 
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 13: 
        goto state4;
    case ';': 
        goto state45;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state47;
    }
    goto state0;

state3: // \t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 13: 
        goto state4;
    case ';': 
        goto state45;
    }
    goto state0;

state4: // \t\r
    switch (*ptr++) {
    case 10: 
        goto state5;
    }
    goto state0;

state5: // \t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    }
    goto state0;

state6: // \t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 13: 
        goto state7;
    case ';': 
        goto state43;
    }
    goto state0;

state7: // \t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state8;
    }
    goto state0;

state8: // \t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    }
    goto state0;

state9: // \t\r\n\t\r\n\r
    switch (*ptr++) {
    case 10: 
        goto state10;
    }
    goto state0;

state10: // \t\r\n\t\r\n\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case 9: case ' ': 
        goto state11;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    }
    goto state0;

state11: // \t\r\n\t\r\n\r\n\t
    switch (*ptr++) {
    case 13: 
        goto state9;
    case 9: case ' ': 
        goto state11;
    case ';': 
        goto state12;
    }
    goto state0;

state12: // \t\r\n\t\r\n\r\n\t;
    switch (*ptr++) {
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
        goto state12;
    case 13: 
        goto state13;
    }
    goto state0;

state13: // \t\r\n\t\r\n\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state10;
    }
    goto state0;

state14: // \t\r\n\t\r\n\r\nA
    switch (*ptr++) {
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
        goto state14;
    case 9: case ' ': 
        goto state15;
    case 13: 
        goto state16;
    case '/': 
        goto state18;
    case '=': 
        goto state19;
    case ';': 
        goto state41;
    }
    goto state0;

state15: // \t\r\n\t\r\n\r\nA\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state15;
    case 13: 
        goto state16;
    case '/': 
        goto state18;
    case '=': 
        goto state19;
    case ';': 
        goto state41;
    }
    goto state0;

state16: // \t\r\n\t\r\n\r\nA\t\r
    switch (*ptr++) {
    case 10: 
        goto state17;
    }
    goto state0;

state17: // \t\r\n\t\r\n\r\nA\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state15;
    }
    goto state0;

state18: // \t\r\n\t\r\n\r\nA\t/
    switch (*ptr++) {
    case '=': 
        goto state19;
    }
    goto state0;

state19: // \t\r\n\t\r\n\r\nA\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state19;
    case 13: 
        goto state20;
    case ';': 
        goto state22;
    }
    if (stateAlternation(--ptr)) {
        goto state24;
    }
    goto state0;

state20: // \t\r\n\t\r\n\r\nA\t/=\r
    switch (*ptr++) {
    case 10: 
        goto state21;
    }
    goto state0;

state21: // \t\r\n\t\r\n\r\nA\t/=\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state19;
    }
    goto state0;

state22: // \t\r\n\t\r\n\r\nA\t/=;
    switch (*ptr++) {
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
        goto state22;
    case 13: 
        goto state23;
    }
    goto state0;

state23: // \t\r\n\t\r\n\r\nA\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state21;
    }
    goto state0;

state24: // \t\r\n\t\r\n\r\nA\t/=\*
    switch (*ptr++) {
    case 9: 
        goto state25;
    case 13: 
        goto state36;
    case ' ': 
        goto state39;
    case ';': 
        goto state40;
    }
    goto state0;

state25: // \t\r\n\t\r\n\r\nA\t/=\*\t
    switch (*ptr++) {
    case 9: 
        goto state25;
    case 13: 
        goto state26;
    case ' ': 
        goto state33;
    case ';': 
        goto state34;
    }
    goto state0;

state26: // \t\r\n\t\r\n\r\nA\t/=\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state27;
    }
    goto state0;

state27: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state28;
    }
    goto state0;

state28: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state28;
    case 13: 
        goto state29;
    case ';': 
        goto state31;
    }
    goto state0;

state29: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state30;
    }
    goto state0;

state30: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state28;
    }
    goto state0;

state31: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state31;
    case 13: 
        goto state32;
    }
    goto state0;

state32: // \t\r\n\t\r\n\r\nA\t/=\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state30;
    }
    goto state0;

state33: // \t\r\n\t\r\n\r\nA\t/=\*\t\s
    switch (*ptr++) {
    case 13: 
        goto state26;
    case 9: case ' ': 
        goto state33;
    case ';': 
        goto state34;
    }
    goto state0;

state34: // \t\r\n\t\r\n\r\nA\t/=\*\t\s;
    switch (*ptr++) {
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
        goto state34;
    case 13: 
        goto state35;
    }
    goto state0;

state35: // \t\r\n\t\r\n\r\nA\t/=\*\t\s;\r
    switch (*ptr++) {
    case 10: 
        goto state27;
    }
    goto state0;

state36: // \t\r\n\t\r\n\r\nA\t/=\*\r
    switch (*ptr++) {
    case 10: 
        goto state27;
    case 13: 
        goto state37;
    }
    goto state0;

state37: // \t\r\n\t\r\n\r\nA\t/=\*\r\r
    switch (*ptr++) {
    case 13: 
        goto state37;
    case 10: 
        goto state38;
    }
    goto state0;

state38: // \t\r\n\t\r\n\r\nA\t/=\*\r\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state33;
    }
    goto state0;

state39: // \t\r\n\t\r\n\r\nA\t/=\*\s
    switch (*ptr++) {
    case 13: 
        goto state26;
    case 9: 
        goto state33;
    case ';': 
        goto state34;
    case ' ': 
        goto state39;
    }
    goto state0;

state40: // \t\r\n\t\r\n\r\nA\t/=\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state34;
    case 13: 
        goto state35;
    case ';': 
        goto state40;
    }
    goto state0;

state41: // \t\r\n\t\r\n\r\nA\t;
    switch (*ptr++) {
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
        goto state41;
    case 13: 
        goto state42;
    }
    goto state0;

state42: // \t\r\n\t\r\n\r\nA\t;\r
    switch (*ptr++) {
    case 10: 
        goto state17;
    }
    goto state0;

state43: // \t\r\n\t;
    switch (*ptr++) {
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
        goto state43;
    case 13: 
        goto state44;
    }
    goto state0;

state44: // \t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state8;
    }
    goto state0;

state45: // \t;
    switch (*ptr++) {
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
        goto state45;
    case 13: 
        goto state46;
    }
    goto state0;

state46: // \t;\r
    switch (*ptr++) {
    case 10: 
        goto state5;
    }
    goto state0;

state47: // A
    switch (*ptr++) {
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
        goto state47;
    case 9: case ' ': 
        goto state48;
    case 13: 
        goto state49;
    case '/': 
        goto state51;
    case '=': 
        goto state52;
    case ';': 
        goto state74;
    }
    goto state0;

state48: // A\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state48;
    case 13: 
        goto state49;
    case '/': 
        goto state51;
    case '=': 
        goto state52;
    case ';': 
        goto state74;
    }
    goto state0;

state49: // A\t\r
    switch (*ptr++) {
    case 10: 
        goto state50;
    }
    goto state0;

state50: // A\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state48;
    }
    goto state0;

state51: // A\t/
    switch (*ptr++) {
    case '=': 
        goto state52;
    }
    goto state0;

state52: // A\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state52;
    case 13: 
        goto state53;
    case ';': 
        goto state55;
    }
    if (stateAlternation(--ptr)) {
        goto state57;
    }
    goto state0;

state53: // A\t/=\r
    switch (*ptr++) {
    case 10: 
        goto state54;
    }
    goto state0;

state54: // A\t/=\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state52;
    }
    goto state0;

state55: // A\t/=;
    switch (*ptr++) {
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
        goto state55;
    case 13: 
        goto state56;
    }
    goto state0;

state56: // A\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state54;
    }
    goto state0;

state57: // A\t/=\*
    switch (*ptr++) {
    case 9: 
        goto state58;
    case 13: 
        goto state69;
    case ' ': 
        goto state72;
    case ';': 
        goto state73;
    }
    goto state0;

state58: // A\t/=\*\t
    switch (*ptr++) {
    case 9: 
        goto state58;
    case 13: 
        goto state59;
    case ' ': 
        goto state66;
    case ';': 
        goto state67;
    }
    goto state0;

state59: // A\t/=\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state60;
    }
    goto state0;

state60: // A\t/=\*\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state61;
    }
    goto state0;

state61: // A\t/=\*\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state61;
    case 13: 
        goto state62;
    case ';': 
        goto state64;
    }
    goto state0;

state62: // A\t/=\*\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state63;
    }
    goto state0;

state63: // A\t/=\*\t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state61;
    }
    goto state0;

state64: // A\t/=\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state64;
    case 13: 
        goto state65;
    }
    goto state0;

state65: // A\t/=\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state63;
    }
    goto state0;

state66: // A\t/=\*\t\s
    switch (*ptr++) {
    case 13: 
        goto state59;
    case 9: case ' ': 
        goto state66;
    case ';': 
        goto state67;
    }
    goto state0;

state67: // A\t/=\*\t\s;
    switch (*ptr++) {
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
        goto state67;
    case 13: 
        goto state68;
    }
    goto state0;

state68: // A\t/=\*\t\s;\r
    switch (*ptr++) {
    case 10: 
        goto state60;
    }
    goto state0;

state69: // A\t/=\*\r
    switch (*ptr++) {
    case 10: 
        goto state60;
    case 13: 
        goto state70;
    }
    goto state0;

state70: // A\t/=\*\r\r
    switch (*ptr++) {
    case 13: 
        goto state70;
    case 10: 
        goto state71;
    }
    goto state0;

state71: // A\t/=\*\r\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state66;
    }
    goto state0;

state72: // A\t/=\*\s
    switch (*ptr++) {
    case 13: 
        goto state59;
    case 9: 
        goto state66;
    case ';': 
        goto state67;
    case ' ': 
        goto state72;
    }
    goto state0;

state73: // A\t/=\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state67;
    case 13: 
        goto state68;
    case ';': 
        goto state73;
    }
    goto state0;

state74: // A\t;
    switch (*ptr++) {
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
        goto state74;
    case 13: 
        goto state75;
    }
    goto state0;

state75: // A\t;\r
    switch (*ptr++) {
    case 10: 
        goto state50;
    }
    goto state0;
}

//===========================================================================
bool AbnfParser::stateAlternation(const char *& ptr) {
    goto state2;

state0: // <FAILED>
    return false;

state1: // <DONE>
    return true;

state2: // 
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state212;
    case '(': 
        goto state237;
    case '*': 
        goto state253;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state254;
    case '[': 
        goto state255;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state271;
    }
    goto state0;

state3: // "
    switch (*ptr++) {
    case ' ': case '!': case '#': case '$': case '%': case '&':
    case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case ':': case ';': case '<': case '=': case '>':
    case '?': case '@': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\':
    case ']': case '^': case '_': case '`': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case '{': case '|': case '}': case '~': 
        goto state3;
    case '"': 
        goto state4;
    }
    goto state0;

state4: // ""
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    }
    goto state0;

state5: // ""\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state6;
    case '"': 
        goto state8;
    case '%': 
        goto state9;
    case '/': 
        goto state16;
    case '(': 
        goto state175;
    case '*': 
        goto state191;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state192;
    case '[': 
        goto state193;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state209;
    case ';': 
        goto state210;
    }
    goto state0;

state6: // ""\t\r
    switch (*ptr++) {
    case 10: 
        goto state7;
    }
    goto state0;

state7: // ""\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state5;
    }
    goto state0;

state8: // ""\t"
    switch (*ptr++) {
    case '"': 
        goto state4;
    case ' ': case '!': case '#': case '$': case '%': case '&':
    case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case ':': case ';': case '<': case '=': case '>':
    case '?': case '@': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\':
    case ']': case '^': case '_': case '`': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case '{': case '|': case '}': case '~': 
        goto state8;
    }
    goto state0;

state9: // ""\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state10;
    case 'D': case 'd': 
        goto state159;
    case 'X': case 'x': 
        goto state167;
    }
    goto state0;

state10: // ""\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state11;
    }
    goto state0;

state11: // ""\t%B0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case '0': case '1': 
        goto state11;
    case 13: 
        goto state12;
    case '-': 
        goto state14;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state155;
    }
    goto state0;

state12: // ""\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state13;
    }
    goto state0;

state13: // ""\t%B0\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state5;
    }
    goto state0;

state14: // ""\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state15;
    }
    goto state0;

state15: // ""\t%B0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '0': case '1': 
        goto state15;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    }
    goto state0;

state16: // ""\t%B0-0/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state16;
    case 13: 
        goto state17;
    case '"': 
        goto state19;
    case '%': 
        goto state91;
    case '(': 
        goto state116;
    case '*': 
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
        goto state133;
    case '[': 
        goto state134;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state150;
    case ';': 
        goto state151;
    }
    goto state0;

state17: // ""\t%B0-0/\r
    switch (*ptr++) {
    case 10: 
        goto state18;
    }
    goto state0;

state18: // ""\t%B0-0/\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state16;
    }
    goto state0;

state19: // ""\t%B0-0/"
    switch (*ptr++) {
    case ' ': case '!': case '#': case '$': case '%': case '&':
    case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case ':': case ';': case '<': case '=': case '>':
    case '?': case '@': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\':
    case ']': case '^': case '_': case '`': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case '{': case '|': case '}': case '~': 
        goto state19;
    case '"': 
        goto state20;
    }
    goto state0;

state20: // ""\t%B0-0/""
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    }
    goto state0;

state21: // ""\t%B0-0/""\t
    switch (*ptr++) {
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state22;
    case '"': 
        goto state24;
    case '%': 
        goto state25;
    case '(': 
        goto state54;
    case '*': 
        goto state70;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state71;
    case '[': 
        goto state72;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state88;
    case ';': 
        goto state89;
    }
    goto state0;

state22: // ""\t%B0-0/""\t\r
    switch (*ptr++) {
    case 10: 
        goto state23;
    }
    goto state0;

state23: // ""\t%B0-0/""\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state21;
    }
    goto state0;

state24: // ""\t%B0-0/""\t"
    switch (*ptr++) {
    case '"': 
        goto state20;
    case ' ': case '!': case '#': case '$': case '%': case '&':
    case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case ':': case ';': case '<': case '=': case '>':
    case '?': case '@': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\':
    case ']': case '^': case '_': case '`': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case '{': case '|': case '}': case '~': 
        goto state24;
    }
    goto state0;

state25: // ""\t%B0-0/""\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state26;
    case 'D': case 'd': 
        goto state38;
    case 'X': case 'x': 
        goto state46;
    }
    goto state0;

state26: // ""\t%B0-0/""\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state27;
    }
    goto state0;

state27: // ""\t%B0-0/""\t%B0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case '0': case '1': 
        goto state27;
    case 13: 
        goto state28;
    case '-': 
        goto state30;
    case ';': 
        goto state32;
    case '.': 
        goto state34;
    }
    goto state0;

state28: // ""\t%B0-0/""\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state29;
    }
    goto state0;

state29: // ""\t%B0-0/""\t%B0\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state21;
    }
    goto state0;

state30: // ""\t%B0-0/""\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state31;
    }
    goto state0;

state31: // ""\t%B0-0/""\t%B0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case '0': case '1': 
        goto state31;
    case ';': 
        goto state32;
    }
    goto state0;

state32: // ""\t%B0-0/""\t%B0-0;
    switch (*ptr++) {
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
        goto state32;
    case 13: 
        goto state33;
    }
    goto state0;

state33: // ""\t%B0-0/""\t%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state29;
    }
    goto state0;

state34: // ""\t%B0-0/""\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state35;
    }
    goto state0;

state35: // ""\t%B0-0/""\t%B0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': 
        goto state35;
    case '.': 
        goto state36;
    }
    goto state0;

state36: // ""\t%B0-0/""\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state37;
    }
    goto state0;

state37: // ""\t%B0-0/""\t%B0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state36;
    case '0': case '1': 
        goto state37;
    }
    goto state0;

state38: // ""\t%B0-0/""\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state39;
    }
    goto state0;

state39: // ""\t%B0-0/""\t%D0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state39;
    case '-': 
        goto state40;
    case '.': 
        goto state42;
    }
    goto state0;

state40: // ""\t%B0-0/""\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state41;
    }
    goto state0;

state41: // ""\t%B0-0/""\t%D0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state41;
    }
    goto state0;

state42: // ""\t%B0-0/""\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state43;
    }
    goto state0;

state43: // ""\t%B0-0/""\t%D0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state43;
    case '.': 
        goto state44;
    }
    goto state0;

state44: // ""\t%B0-0/""\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state45;
    }
    goto state0;

state45: // ""\t%B0-0/""\t%D0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state44;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state45;
    }
    goto state0;

state46: // ""\t%B0-0/""\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state47;
    }
    goto state0;

state47: // ""\t%B0-0/""\t%X0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state47;
    case '-': 
        goto state48;
    case '.': 
        goto state50;
    }
    goto state0;

state48: // ""\t%B0-0/""\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state49;
    }
    goto state0;

state49: // ""\t%B0-0/""\t%X0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state49;
    }
    goto state0;

state50: // ""\t%B0-0/""\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state51;
    }
    goto state0;

state51: // ""\t%B0-0/""\t%X0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state51;
    case '.': 
        goto state52;
    }
    goto state0;

state52: // ""\t%B0-0/""\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state53;
    }
    goto state0;

state53: // ""\t%B0-0/""\t%X0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state52;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state53;
    }
    goto state0;

state54: // ""\t%B0-0/""\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state54;
    case 13: 
        goto state55;
    case ';': 
        goto state57;
    }
    if (stateAlternation(--ptr)) {
        goto state59;
    }
    goto state0;

state55: // ""\t%B0-0/""\t(\r
    switch (*ptr++) {
    case 10: 
        goto state56;
    }
    goto state0;

state56: // ""\t%B0-0/""\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state54;
    }
    goto state0;

state57: // ""\t%B0-0/""\t(;
    switch (*ptr++) {
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
        goto state57;
    case 13: 
        goto state58;
    }
    goto state0;

state58: // ""\t%B0-0/""\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state56;
    }
    goto state0;

state59: // ""\t%B0-0/""\t(\*
    switch (*ptr++) {
    case 9: 
        goto state60;
    case 13: 
        goto state66;
    case ' ': 
        goto state67;
    case ')': 
        goto state68;
    case ';': 
        goto state69;
    }
    goto state0;

state60: // ""\t%B0-0/""\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 9: 
        goto state60;
    case 13: 
        goto state61;
    case ' ': 
        goto state63;
    case ';': 
        goto state64;
    }
    goto state0;

state61: // ""\t%B0-0/""\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state62;
    }
    goto state0;

state62: // ""\t%B0-0/""\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state63;
    }
    goto state0;

state63: // ""\t%B0-0/""\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 13: 
        goto state61;
    case 9: case ' ': 
        goto state63;
    case ';': 
        goto state64;
    }
    goto state0;

state64: // ""\t%B0-0/""\t(\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state64;
    case 13: 
        goto state65;
    }
    goto state0;

state65: // ""\t%B0-0/""\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state62;
    }
    goto state0;

state66: // ""\t%B0-0/""\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state62;
    case 13: 
        goto state66;
    }
    goto state0;

state67: // ""\t%B0-0/""\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 13: 
        goto state61;
    case 9: 
        goto state63;
    case ';': 
        goto state64;
    case ' ': 
        goto state67;
    }
    goto state0;

state68: // ""\t%B0-0/""\t(\*)
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case ')': 
        goto state68;
    }
    goto state0;

state69: // ""\t%B0-0/""\t(\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state64;
    case 13: 
        goto state65;
    case ';': 
        goto state69;
    }
    goto state0;

state70: // ""\t%B0-0/""\t*
    switch (*ptr++) {
    case '"': 
        goto state24;
    case '%': 
        goto state25;
    case '(': 
        goto state54;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state70;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state71;
    case '[': 
        goto state72;
    }
    goto state0;

state71: // ""\t%B0-0/""\t*A
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
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
        goto state71;
    }
    goto state0;

state72: // ""\t%B0-0/""\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state72;
    case 13: 
        goto state73;
    case ';': 
        goto state75;
    }
    if (stateAlternation(--ptr)) {
        goto state77;
    }
    goto state0;

state73: // ""\t%B0-0/""\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state74;
    }
    goto state0;

state74: // ""\t%B0-0/""\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state72;
    }
    goto state0;

state75: // ""\t%B0-0/""\t*[;
    switch (*ptr++) {
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
        goto state75;
    case 13: 
        goto state76;
    }
    goto state0;

state76: // ""\t%B0-0/""\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state74;
    }
    goto state0;

state77: // ""\t%B0-0/""\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state78;
    case 13: 
        goto state84;
    case ' ': 
        goto state85;
    case ';': 
        goto state86;
    case ']': 
        goto state87;
    }
    goto state0;

state78: // ""\t%B0-0/""\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 9: 
        goto state78;
    case 13: 
        goto state79;
    case ' ': 
        goto state81;
    case ';': 
        goto state82;
    }
    goto state0;

state79: // ""\t%B0-0/""\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state80;
    }
    goto state0;

state80: // ""\t%B0-0/""\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state81;
    }
    goto state0;

state81: // ""\t%B0-0/""\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 13: 
        goto state79;
    case 9: case ' ': 
        goto state81;
    case ';': 
        goto state82;
    }
    goto state0;

state82: // ""\t%B0-0/""\t*[\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state82;
    case 13: 
        goto state83;
    }
    goto state0;

state83: // ""\t%B0-0/""\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state80;
    }
    goto state0;

state84: // ""\t%B0-0/""\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state80;
    case 13: 
        goto state84;
    }
    goto state0;

state85: // ""\t%B0-0/""\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 13: 
        goto state79;
    case 9: 
        goto state81;
    case ';': 
        goto state82;
    case ' ': 
        goto state85;
    }
    goto state0;

state86: // ""\t%B0-0/""\t*[\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state82;
    case 13: 
        goto state83;
    case ';': 
        goto state86;
    }
    goto state0;

state87: // ""\t%B0-0/""\t*[\*]
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case ']': 
        goto state87;
    }
    goto state0;

state88: // ""\t%B0-0/""\t0
    switch (*ptr++) {
    case '"': 
        goto state24;
    case '%': 
        goto state25;
    case '(': 
        goto state54;
    case '*': 
        goto state70;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state71;
    case '[': 
        goto state72;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state88;
    }
    goto state0;

state89: // ""\t%B0-0/""\t;
    switch (*ptr++) {
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
        goto state89;
    case 13: 
        goto state90;
    }
    goto state0;

state90: // ""\t%B0-0/""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state23;
    }
    goto state0;

state91: // ""\t%B0-0/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state92;
    case 'D': case 'd': 
        goto state100;
    case 'X': case 'x': 
        goto state108;
    }
    goto state0;

state92: // ""\t%B0-0/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state93;
    }
    goto state0;

state93: // ""\t%B0-0/%B0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': 
        goto state93;
    case '-': 
        goto state94;
    case '.': 
        goto state96;
    }
    goto state0;

state94: // ""\t%B0-0/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state95;
    }
    goto state0;

state95: // ""\t%B0-0/%B0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': 
        goto state95;
    }
    goto state0;

state96: // ""\t%B0-0/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state97;
    }
    goto state0;

state97: // ""\t%B0-0/%B0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': 
        goto state97;
    case '.': 
        goto state98;
    }
    goto state0;

state98: // ""\t%B0-0/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state99;
    }
    goto state0;

state99: // ""\t%B0-0/%B0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state98;
    case '0': case '1': 
        goto state99;
    }
    goto state0;

state100: // ""\t%B0-0/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state101;
    }
    goto state0;

state101: // ""\t%B0-0/%D0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state101;
    case '-': 
        goto state102;
    case '.': 
        goto state104;
    }
    goto state0;

state102: // ""\t%B0-0/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state103;
    }
    goto state0;

state103: // ""\t%B0-0/%D0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state103;
    }
    goto state0;

state104: // ""\t%B0-0/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state105;
    }
    goto state0;

state105: // ""\t%B0-0/%D0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state105;
    case '.': 
        goto state106;
    }
    goto state0;

state106: // ""\t%B0-0/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state107;
    }
    goto state0;

state107: // ""\t%B0-0/%D0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state106;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state107;
    }
    goto state0;

state108: // ""\t%B0-0/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state109;
    }
    goto state0;

state109: // ""\t%B0-0/%X0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state109;
    case '-': 
        goto state110;
    case '.': 
        goto state112;
    }
    goto state0;

state110: // ""\t%B0-0/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state111;
    }
    goto state0;

state111: // ""\t%B0-0/%X0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state111;
    }
    goto state0;

state112: // ""\t%B0-0/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state113;
    }
    goto state0;

state113: // ""\t%B0-0/%X0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state113;
    case '.': 
        goto state114;
    }
    goto state0;

state114: // ""\t%B0-0/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state115;
    }
    goto state0;

state115: // ""\t%B0-0/%X0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case '.': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state115;
    }
    goto state0;

state116: // ""\t%B0-0/(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state116;
    case 13: 
        goto state117;
    case ';': 
        goto state119;
    }
    if (stateAlternation(--ptr)) {
        goto state121;
    }
    goto state0;

state117: // ""\t%B0-0/(\r
    switch (*ptr++) {
    case 10: 
        goto state118;
    }
    goto state0;

state118: // ""\t%B0-0/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state116;
    }
    goto state0;

state119: // ""\t%B0-0/(;
    switch (*ptr++) {
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
        goto state119;
    case 13: 
        goto state120;
    }
    goto state0;

state120: // ""\t%B0-0/(;\r
    switch (*ptr++) {
    case 10: 
        goto state118;
    }
    goto state0;

state121: // ""\t%B0-0/(\*
    switch (*ptr++) {
    case 9: 
        goto state122;
    case 13: 
        goto state128;
    case ' ': 
        goto state129;
    case ')': 
        goto state130;
    case ';': 
        goto state131;
    }
    goto state0;

state122: // ""\t%B0-0/(\*\t
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 9: 
        goto state122;
    case 13: 
        goto state123;
    case ' ': 
        goto state125;
    case ';': 
        goto state126;
    }
    goto state0;

state123: // ""\t%B0-0/(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state124;
    }
    goto state0;

state124: // ""\t%B0-0/(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state125;
    }
    goto state0;

state125: // ""\t%B0-0/(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 13: 
        goto state123;
    case 9: case ' ': 
        goto state125;
    case ';': 
        goto state126;
    }
    goto state0;

state126: // ""\t%B0-0/(\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state126;
    case 13: 
        goto state127;
    }
    goto state0;

state127: // ""\t%B0-0/(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state124;
    }
    goto state0;

state128: // ""\t%B0-0/(\*\r
    switch (*ptr++) {
    case 10: 
        goto state124;
    case 13: 
        goto state128;
    }
    goto state0;

state129: // ""\t%B0-0/(\*\s
    switch (*ptr++) {
    case ')': 
        goto state20;
    case 13: 
        goto state123;
    case 9: 
        goto state125;
    case ';': 
        goto state126;
    case ' ': 
        goto state129;
    }
    goto state0;

state130: // ""\t%B0-0/(\*)
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case ')': 
        goto state130;
    }
    goto state0;

state131: // ""\t%B0-0/(\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state126;
    case 13: 
        goto state127;
    case ';': 
        goto state131;
    }
    goto state0;

state132: // ""\t%B0-0/*
    switch (*ptr++) {
    case '"': 
        goto state19;
    case '%': 
        goto state91;
    case '(': 
        goto state116;
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
        goto state133;
    case '[': 
        goto state134;
    }
    goto state0;

state133: // ""\t%B0-0/*A
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
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
        goto state133;
    }
    goto state0;

state134: // ""\t%B0-0/*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state134;
    case 13: 
        goto state135;
    case ';': 
        goto state137;
    }
    if (stateAlternation(--ptr)) {
        goto state139;
    }
    goto state0;

state135: // ""\t%B0-0/*[\r
    switch (*ptr++) {
    case 10: 
        goto state136;
    }
    goto state0;

state136: // ""\t%B0-0/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state134;
    }
    goto state0;

state137: // ""\t%B0-0/*[;
    switch (*ptr++) {
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
        goto state137;
    case 13: 
        goto state138;
    }
    goto state0;

state138: // ""\t%B0-0/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state136;
    }
    goto state0;

state139: // ""\t%B0-0/*[\*
    switch (*ptr++) {
    case 9: 
        goto state140;
    case 13: 
        goto state146;
    case ' ': 
        goto state147;
    case ';': 
        goto state148;
    case ']': 
        goto state149;
    }
    goto state0;

state140: // ""\t%B0-0/*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 9: 
        goto state140;
    case 13: 
        goto state141;
    case ' ': 
        goto state143;
    case ';': 
        goto state144;
    }
    goto state0;

state141: // ""\t%B0-0/*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state142;
    }
    goto state0;

state142: // ""\t%B0-0/*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state143;
    }
    goto state0;

state143: // ""\t%B0-0/*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 13: 
        goto state141;
    case 9: case ' ': 
        goto state143;
    case ';': 
        goto state144;
    }
    goto state0;

state144: // ""\t%B0-0/*[\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state144;
    case 13: 
        goto state145;
    }
    goto state0;

state145: // ""\t%B0-0/*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state142;
    }
    goto state0;

state146: // ""\t%B0-0/*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state142;
    case 13: 
        goto state146;
    }
    goto state0;

state147: // ""\t%B0-0/*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state20;
    case 13: 
        goto state141;
    case 9: 
        goto state143;
    case ';': 
        goto state144;
    case ' ': 
        goto state147;
    }
    goto state0;

state148: // ""\t%B0-0/*[\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state144;
    case 13: 
        goto state145;
    case ';': 
        goto state148;
    }
    goto state0;

state149: // ""\t%B0-0/*[\*]
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state16;
    case 9: case ' ': 
        goto state21;
    case 13: 
        goto state28;
    case ';': 
        goto state32;
    case ']': 
        goto state149;
    }
    goto state0;

state150: // ""\t%B0-0/0
    switch (*ptr++) {
    case '"': 
        goto state19;
    case '%': 
        goto state91;
    case '(': 
        goto state116;
    case '*': 
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
        goto state133;
    case '[': 
        goto state134;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state150;
    }
    goto state0;

state151: // ""\t%B0-0/;
    switch (*ptr++) {
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
        goto state151;
    case 13: 
        goto state152;
    }
    goto state0;

state152: // ""\t%B0-0/;\r
    switch (*ptr++) {
    case 10: 
        goto state18;
    }
    goto state0;

state153: // ""\t%B0-0;
    switch (*ptr++) {
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
        goto state153;
    case 13: 
        goto state154;
    }
    goto state0;

state154: // ""\t%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state13;
    }
    goto state0;

state155: // ""\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state156;
    }
    goto state0;

state156: // ""\t%B0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': 
        goto state156;
    case '.': 
        goto state157;
    }
    goto state0;

state157: // ""\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state158;
    }
    goto state0;

state158: // ""\t%B0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state157;
    case '0': case '1': 
        goto state158;
    }
    goto state0;

state159: // ""\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    }
    goto state0;

state160: // ""\t%D0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    case '-': 
        goto state161;
    case '.': 
        goto state163;
    }
    goto state0;

state161: // ""\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    }
    goto state0;

state162: // ""\t%D0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    }
    goto state0;

state163: // ""\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state164;
    }
    goto state0;

state164: // ""\t%D0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state164;
    case '.': 
        goto state165;
    }
    goto state0;

state165: // ""\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state166;
    }
    goto state0;

state166: // ""\t%D0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state165;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state166;
    }
    goto state0;

state167: // ""\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    }
    goto state0;

state168: // ""\t%X0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    case '-': 
        goto state169;
    case '.': 
        goto state171;
    }
    goto state0;

state169: // ""\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    }
    goto state0;

state170: // ""\t%X0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    }
    goto state0;

state171: // ""\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state172;
    }
    goto state0;

state172: // ""\t%X0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state172;
    case '.': 
        goto state173;
    }
    goto state0;

state173: // ""\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state174;
    }
    goto state0;

state174: // ""\t%X0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state173;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state174;
    }
    goto state0;

state175: // ""\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state175;
    case 13: 
        goto state176;
    case ';': 
        goto state178;
    }
    if (stateAlternation(--ptr)) {
        goto state180;
    }
    goto state0;

state176: // ""\t(\r
    switch (*ptr++) {
    case 10: 
        goto state177;
    }
    goto state0;

state177: // ""\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state175;
    }
    goto state0;

state178: // ""\t(;
    switch (*ptr++) {
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
        goto state178;
    case 13: 
        goto state179;
    }
    goto state0;

state179: // ""\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state177;
    }
    goto state0;

state180: // ""\t(\*
    switch (*ptr++) {
    case 9: 
        goto state181;
    case 13: 
        goto state187;
    case ' ': 
        goto state188;
    case ')': 
        goto state189;
    case ';': 
        goto state190;
    }
    goto state0;

state181: // ""\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 9: 
        goto state181;
    case 13: 
        goto state182;
    case ' ': 
        goto state184;
    case ';': 
        goto state185;
    }
    goto state0;

state182: // ""\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state183;
    }
    goto state0;

state183: // ""\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state184;
    }
    goto state0;

state184: // ""\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 13: 
        goto state182;
    case 9: case ' ': 
        goto state184;
    case ';': 
        goto state185;
    }
    goto state0;

state185: // ""\t(\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state185;
    case 13: 
        goto state186;
    }
    goto state0;

state186: // ""\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state183;
    }
    goto state0;

state187: // ""\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state183;
    case 13: 
        goto state187;
    }
    goto state0;

state188: // ""\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 13: 
        goto state182;
    case 9: 
        goto state184;
    case ';': 
        goto state185;
    case ' ': 
        goto state188;
    }
    goto state0;

state189: // ""\t(\*)
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case ')': 
        goto state189;
    }
    goto state0;

state190: // ""\t(\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state185;
    case 13: 
        goto state186;
    case ';': 
        goto state190;
    }
    goto state0;

state191: // ""\t*
    switch (*ptr++) {
    case '"': 
        goto state8;
    case '%': 
        goto state9;
    case '(': 
        goto state175;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state191;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state192;
    case '[': 
        goto state193;
    }
    goto state0;

state192: // ""\t*A
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
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
        goto state192;
    }
    goto state0;

state193: // ""\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state193;
    case 13: 
        goto state194;
    case ';': 
        goto state196;
    }
    if (stateAlternation(--ptr)) {
        goto state198;
    }
    goto state0;

state194: // ""\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state195;
    }
    goto state0;

state195: // ""\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state193;
    }
    goto state0;

state196: // ""\t*[;
    switch (*ptr++) {
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

state197: // ""\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state195;
    }
    goto state0;

state198: // ""\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state199;
    case 13: 
        goto state205;
    case ' ': 
        goto state206;
    case ';': 
        goto state207;
    case ']': 
        goto state208;
    }
    goto state0;

state199: // ""\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 9: 
        goto state199;
    case 13: 
        goto state200;
    case ' ': 
        goto state202;
    case ';': 
        goto state203;
    }
    goto state0;

state200: // ""\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state201;
    }
    goto state0;

state201: // ""\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state202;
    }
    goto state0;

state202: // ""\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 13: 
        goto state200;
    case 9: case ' ': 
        goto state202;
    case ';': 
        goto state203;
    }
    goto state0;

state203: // ""\t*[\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state203;
    case 13: 
        goto state204;
    }
    goto state0;

state204: // ""\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state201;
    }
    goto state0;

state205: // ""\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state201;
    case 13: 
        goto state205;
    }
    goto state0;

state206: // ""\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 13: 
        goto state200;
    case 9: 
        goto state202;
    case ';': 
        goto state203;
    case ' ': 
        goto state206;
    }
    goto state0;

state207: // ""\t*[\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state203;
    case 13: 
        goto state204;
    case ';': 
        goto state207;
    }
    goto state0;

state208: // ""\t*[\*]
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case ']': 
        goto state208;
    }
    goto state0;

state209: // ""\t0
    switch (*ptr++) {
    case '"': 
        goto state8;
    case '%': 
        goto state9;
    case '(': 
        goto state175;
    case '*': 
        goto state191;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state192;
    case '[': 
        goto state193;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state209;
    }
    goto state0;

state210: // ""\t;
    switch (*ptr++) {
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

state211: // ""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state7;
    }
    goto state0;

state212: // %
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state213;
    case 'D': case 'd': 
        goto state221;
    case 'X': case 'x': 
        goto state229;
    }
    goto state0;

state213: // %B
    switch (*ptr++) {
    case '0': case '1': 
        goto state214;
    }
    goto state0;

state214: // %B0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': 
        goto state214;
    case '-': 
        goto state215;
    case '.': 
        goto state217;
    }
    goto state0;

state215: // %B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state216;
    }
    goto state0;

state216: // %B0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': 
        goto state216;
    }
    goto state0;

state217: // %B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state218;
    }
    goto state0;

state218: // %B0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': 
        goto state218;
    case '.': 
        goto state219;
    }
    goto state0;

state219: // %B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state220;
    }
    goto state0;

state220: // %B0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state219;
    case '0': case '1': 
        goto state220;
    }
    goto state0;

state221: // %D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state222;
    }
    goto state0;

state222: // %D0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state222;
    case '-': 
        goto state223;
    case '.': 
        goto state225;
    }
    goto state0;

state223: // %D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state224;
    }
    goto state0;

state224: // %D0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state224;
    }
    goto state0;

state225: // %D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state226;
    }
    goto state0;

state226: // %D0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state226;
    case '.': 
        goto state227;
    }
    goto state0;

state227: // %D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state228;
    }
    goto state0;

state228: // %D0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state227;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state228;
    }
    goto state0;

state229: // %X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state230;
    }
    goto state0;

state230: // %X0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state230;
    case '-': 
        goto state231;
    case '.': 
        goto state233;
    }
    goto state0;

state231: // %X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state232;
    }
    goto state0;

state232: // %X0-0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state232;
    }
    goto state0;

state233: // %X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state234;
    }
    goto state0;

state234: // %X0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state234;
    case '.': 
        goto state235;
    }
    goto state0;

state235: // %X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state236;
    }
    goto state0;

state236: // %X0.0.0
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case '.': 
        goto state235;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state236;
    }
    goto state0;

state237: // (
    switch (*ptr++) {
    case 9: case ' ': 
        goto state237;
    case 13: 
        goto state238;
    case ';': 
        goto state240;
    }
    if (stateAlternation(--ptr)) {
        goto state242;
    }
    goto state0;

state238: // (\r
    switch (*ptr++) {
    case 10: 
        goto state239;
    }
    goto state0;

state239: // (\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state237;
    }
    goto state0;

state240: // (;
    switch (*ptr++) {
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
        goto state240;
    case 13: 
        goto state241;
    }
    goto state0;

state241: // (;\r
    switch (*ptr++) {
    case 10: 
        goto state239;
    }
    goto state0;

state242: // (\*
    switch (*ptr++) {
    case 9: 
        goto state243;
    case 13: 
        goto state249;
    case ' ': 
        goto state250;
    case ')': 
        goto state251;
    case ';': 
        goto state252;
    }
    goto state0;

state243: // (\*\t
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 9: 
        goto state243;
    case 13: 
        goto state244;
    case ' ': 
        goto state246;
    case ';': 
        goto state247;
    }
    goto state0;

state244: // (\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state245;
    }
    goto state0;

state245: // (\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state246;
    }
    goto state0;

state246: // (\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 13: 
        goto state244;
    case 9: case ' ': 
        goto state246;
    case ';': 
        goto state247;
    }
    goto state0;

state247: // (\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state247;
    case 13: 
        goto state248;
    }
    goto state0;

state248: // (\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state245;
    }
    goto state0;

state249: // (\*\r
    switch (*ptr++) {
    case 10: 
        goto state245;
    case 13: 
        goto state249;
    }
    goto state0;

state250: // (\*\s
    switch (*ptr++) {
    case ')': 
        goto state4;
    case 13: 
        goto state244;
    case 9: 
        goto state246;
    case ';': 
        goto state247;
    case ' ': 
        goto state250;
    }
    goto state0;

state251: // (\*)
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case ')': 
        goto state251;
    }
    goto state0;

state252: // (\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state247;
    case 13: 
        goto state248;
    case ';': 
        goto state252;
    }
    goto state0;

state253: // *
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state212;
    case '(': 
        goto state237;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state253;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state254;
    case '[': 
        goto state255;
    }
    goto state0;

state254: // *A
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
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
        goto state254;
    }
    goto state0;

state255: // *[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state255;
    case 13: 
        goto state256;
    case ';': 
        goto state258;
    }
    if (stateAlternation(--ptr)) {
        goto state260;
    }
    goto state0;

state256: // *[\r
    switch (*ptr++) {
    case 10: 
        goto state257;
    }
    goto state0;

state257: // *[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state255;
    }
    goto state0;

state258: // *[;
    switch (*ptr++) {
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
        goto state258;
    case 13: 
        goto state259;
    }
    goto state0;

state259: // *[;\r
    switch (*ptr++) {
    case 10: 
        goto state257;
    }
    goto state0;

state260: // *[\*
    switch (*ptr++) {
    case 9: 
        goto state261;
    case 13: 
        goto state267;
    case ' ': 
        goto state268;
    case ';': 
        goto state269;
    case ']': 
        goto state270;
    }
    goto state0;

state261: // *[\*\t
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 9: 
        goto state261;
    case 13: 
        goto state262;
    case ' ': 
        goto state264;
    case ';': 
        goto state265;
    }
    goto state0;

state262: // *[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state263;
    }
    goto state0;

state263: // *[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state264;
    }
    goto state0;

state264: // *[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 13: 
        goto state262;
    case 9: case ' ': 
        goto state264;
    case ';': 
        goto state265;
    }
    goto state0;

state265: // *[\*\t\r\n\t;
    switch (*ptr++) {
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
        goto state265;
    case 13: 
        goto state266;
    }
    goto state0;

state266: // *[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state263;
    }
    goto state0;

state267: // *[\*\r
    switch (*ptr++) {
    case 10: 
        goto state263;
    case 13: 
        goto state267;
    }
    goto state0;

state268: // *[\*\s
    switch (*ptr++) {
    case ']': 
        goto state4;
    case 13: 
        goto state262;
    case 9: 
        goto state264;
    case ';': 
        goto state265;
    case ' ': 
        goto state268;
    }
    goto state0;

state269: // *[\*;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$':
    case '%': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/': case '0':
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case ':': case '<': case '=':
    case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case '[':
    case '\\': case ']': case '^': case '_': case '`': case 'a':
    case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state265;
    case 13: 
        goto state266;
    case ';': 
        goto state269;
    }
    goto state0;

state270: // *[\*]
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 13: 
        goto state12;
    case '/': 
        goto state16;
    case ';': 
        goto state153;
    case ']': 
        goto state270;
    }
    goto state0;

state271: // 0
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state212;
    case '(': 
        goto state237;
    case '*': 
        goto state253;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state254;
    case '[': 
        goto state255;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state271;
    }
    goto state0;
}

