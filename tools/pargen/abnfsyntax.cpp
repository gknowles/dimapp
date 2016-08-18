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
*   NEWLINE = ( CR / LF / CRLF ) 
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
*   bin-val = ( ( %x62 / %x42 ) ( bin-val-simple / bin-val-concatenation / 
*       bin-val-alternation ) ) 
*   bin-val-alternation = ( bin-val-simple %x2d bin-val-simple ) 
*   bin-val-concatenation = ( bin-val-simple 1*( %x2e bin-val-simple ) ) 
*   bin-val-simple = 1*BIT 
*   c-nl = ( comment / NEWLINE ) 
*   c-wsp = ( WSP / ( c-nl WSP ) ) 
*   char-val = ( DQUOTE char-val-sequence DQUOTE ) 
*   char-val-sequence = *( %x20 / %x21 / %x23 / %x24 / %x25 / %x26 / %x27 / 
*       %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / 
*       %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / 
*       %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / 
*       %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / 
*       %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / 
*       %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / 
*       %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / 
*       %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / 
*       %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e ) 
*   comment = ( %x3b *( WSP / VCHAR ) NEWLINE ) 
*   concatenation = ( repetition *( 1*c-wsp repetition ) ) 
*   dec-val = ( ( %x64 / %x44 ) ( dec-val-simple / dec-val-concatenation / 
*       dec-val-alternation ) ) 
*   dec-val-alternation = ( dec-val-simple %x2d dec-val-simple ) 
*   dec-val-concatenation = ( dec-val-simple 1*( %x2e dec-val-simple ) ) 
*   dec-val-simple = 1*DIGIT 
*   defined-as = ( *c-wsp ( %x3d / ( %x2f %x3d ) ) *c-wsp ) 
*   element = ( rulename / group / option / char-val / num-val ) 
*   elements = ( alternation *c-wsp ) 
*   group = ( %x28 *c-wsp alternation *c-wsp %x29 ) 
*   hex-val = ( ( %x78 / %x58 ) ( hex-val-simple / hex-val-concatenation / 
*       hex-val-alternation ) ) 
*   hex-val-alternation = ( hex-val-simple %x2d hex-val-simple ) 
*   hex-val-concatenation = ( hex-val-simple 1*( %x2e hex-val-simple ) ) 
*   hex-val-simple = 1*HEXDIG 
*   num-val = ( %x25 ( bin-val / dec-val / hex-val ) ) 
*   option = ( %x5b *c-wsp alternation *c-wsp %x5d ) 
*   repeat = ( *1repeat-min / ( *1repeat-min %x2a *1repeat-max ) ) 
*   repeat-max = 1*DIGIT 
*   repeat-min = 1*DIGIT 
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
    case 10: 
        goto state4;
    case 13: 
        goto state45;
    case ';': 
        goto state46;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state48;
    }
    goto state0;

state3: // \t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 10: 
        goto state4;
    case 13: 
        goto state45;
    case ';': 
        goto state46;
    }
    goto state0;

state4: // \t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state5: // \t\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state42;
    case ';': 
        goto state43;
    }
    goto state0;

state6: // \t\n\t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state7: // \t\n\t\n\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state8: // \t\n\t\n\n\t
    switch (*ptr++) {
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    }
    goto state0;

state9: // \t\n\t\n\n\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state10: // \t\n\t\n\n\t\r;
    switch (*ptr++) {
    case 10: 
        goto state7;
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
        goto state10;
    case 13: 
        goto state11;
    }
    goto state0;

state11: // \t\n\t\n\n\t\r;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 9: case ' ': 
        goto state8;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state12: // \t\n\t\n\n\t\r;\rA
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
        goto state12;
    case 9: case ' ': 
        goto state13;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state16;
    case '=': 
        goto state17;
    case ';': 
        goto state40;
    }
    goto state0;

state13: // \t\n\t\n\n\t\r;\rA\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state13;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state16;
    case '=': 
        goto state17;
    case ';': 
        goto state40;
    }
    goto state0;

state14: // \t\n\t\n\n\t\r;\rA\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state13;
    }
    goto state0;

state15: // \t\n\t\n\n\t\r;\rA\t\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state13;
    case 10: 
        goto state14;
    }
    goto state0;

state16: // \t\n\t\n\n\t\r;\rA\t/
    switch (*ptr++) {
    case '=': 
        goto state17;
    }
    goto state0;

state17: // \t\n\t\n\n\t\r;\rA\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state17;
    case 10: 
        goto state18;
    case 13: 
        goto state19;
    case ';': 
        goto state20;
    }
    if (stateAlternation(--ptr)) {
        goto state22;
    }
    goto state0;

state18: // \t\n\t\n\n\t\r;\rA\t/=\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state17;
    }
    goto state0;

state19: // \t\n\t\n\n\t\r;\rA\t/=\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state17;
    case 10: 
        goto state18;
    }
    goto state0;

state20: // \t\n\t\n\n\t\r;\rA\t/=;
    switch (*ptr++) {
    case 10: 
        goto state18;
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
        goto state20;
    case 13: 
        goto state21;
    }
    goto state0;

state21: // \t\n\t\n\n\t\r;\rA\t/=;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state17;
    case 10: 
        goto state18;
    }
    goto state0;

state22: // \t\n\t\n\n\t\r;\rA\t/=\*
    switch (*ptr++) {
    case 9: 
        goto state23;
    case 10: 
        goto state34;
    case 13: 
        goto state36;
    case ' ': 
        goto state38;
    case ';': 
        goto state39;
    }
    goto state0;

state23: // \t\n\t\n\n\t\r;\rA\t/=\*\t
    switch (*ptr++) {
    case 9: 
        goto state23;
    case 10: 
        goto state24;
    case 13: 
        goto state30;
    case ' ': 
        goto state31;
    case ';': 
        goto state32;
    }
    goto state0;

state24: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    }
    goto state0;

state25: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    case 13: 
        goto state27;
    case ';': 
        goto state28;
    }
    goto state0;

state26: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n\t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    }
    goto state0;

state27: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    }
    goto state0;

state28: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state26;
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
        goto state28;
    case 13: 
        goto state29;
    }
    goto state0;

state29: // \t\n\t\n\n\t\r;\rA\t/=\*\t\n\t;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    }
    goto state0;

state30: // \t\n\t\n\n\t\r;\rA\t/=\*\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    }
    goto state0;

state31: // \t\n\t\n\n\t\r;\rA\t/=\*\t\s
    switch (*ptr++) {
    case 10: 
        goto state24;
    case 13: 
        goto state30;
    case 9: case ' ': 
        goto state31;
    case ';': 
        goto state32;
    }
    goto state0;

state32: // \t\n\t\n\n\t\r;\rA\t/=\*\t\s;
    switch (*ptr++) {
    case 10: 
        goto state24;
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

state33: // \t\n\t\n\n\t\r;\rA\t/=\*\t\s;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    }
    goto state0;

state34: // \t\n\t\n\n\t\r;\rA\t/=\*\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state35;
    }
    goto state0;

state35: // \t\n\t\n\n\t\r;\rA\t/=\*\n\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state35;
    }
    goto state0;

state36: // \t\n\t\n\n\t\r;\rA\t/=\*\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    case 13: 
        goto state37;
    }
    goto state0;

state37: // \t\n\t\n\n\t\r;\rA\t/=\*\r\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state25;
    case 10: 
        goto state26;
    case 13: 
        goto state37;
    }
    goto state0;

state38: // \t\n\t\n\n\t\r;\rA\t/=\*\s
    switch (*ptr++) {
    case 10: 
        goto state24;
    case 13: 
        goto state30;
    case 9: 
        goto state31;
    case ';': 
        goto state32;
    case ' ': 
        goto state38;
    }
    goto state0;

state39: // \t\n\t\n\n\t\r;\rA\t/=\*;
    switch (*ptr++) {
    case 10: 
        goto state24;
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
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state39;
    }
    goto state0;

state40: // \t\n\t\n\n\t\r;\rA\t;
    switch (*ptr++) {
    case 10: 
        goto state14;
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
        goto state40;
    case 13: 
        goto state41;
    }
    goto state0;

state41: // \t\n\t\n\n\t\r;\rA\t;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state13;
    case 10: 
        goto state14;
    }
    goto state0;

state42: // \t\n\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state43: // \t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state6;
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

state44: // \t\n\t;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state45: // \t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state46: // \t;
    switch (*ptr++) {
    case 10: 
        goto state4;
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
        goto state46;
    case 13: 
        goto state47;
    }
    goto state0;

state47: // \t;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state5;
    case 10: 
        goto state6;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    }
    goto state0;

state48: // A
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
        goto state48;
    case 9: case ' ': 
        goto state49;
    case 10: 
        goto state50;
    case 13: 
        goto state51;
    case '/': 
        goto state52;
    case '=': 
        goto state53;
    case ';': 
        goto state76;
    }
    goto state0;

state49: // A\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state49;
    case 10: 
        goto state50;
    case 13: 
        goto state51;
    case '/': 
        goto state52;
    case '=': 
        goto state53;
    case ';': 
        goto state76;
    }
    goto state0;

state50: // A\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state49;
    }
    goto state0;

state51: // A\t\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state49;
    case 10: 
        goto state50;
    }
    goto state0;

state52: // A\t/
    switch (*ptr++) {
    case '=': 
        goto state53;
    }
    goto state0;

state53: // A\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state53;
    case 10: 
        goto state54;
    case 13: 
        goto state55;
    case ';': 
        goto state56;
    }
    if (stateAlternation(--ptr)) {
        goto state58;
    }
    goto state0;

state54: // A\t/=\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state53;
    }
    goto state0;

state55: // A\t/=\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state53;
    case 10: 
        goto state54;
    }
    goto state0;

state56: // A\t/=;
    switch (*ptr++) {
    case 10: 
        goto state54;
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
        goto state56;
    case 13: 
        goto state57;
    }
    goto state0;

state57: // A\t/=;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state53;
    case 10: 
        goto state54;
    }
    goto state0;

state58: // A\t/=\*
    switch (*ptr++) {
    case 9: 
        goto state59;
    case 10: 
        goto state70;
    case 13: 
        goto state72;
    case ' ': 
        goto state74;
    case ';': 
        goto state75;
    }
    goto state0;

state59: // A\t/=\*\t
    switch (*ptr++) {
    case 9: 
        goto state59;
    case 10: 
        goto state60;
    case 13: 
        goto state66;
    case ' ': 
        goto state67;
    case ';': 
        goto state68;
    }
    goto state0;

state60: // A\t/=\*\t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    }
    goto state0;

state61: // A\t/=\*\t\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state63;
    case ';': 
        goto state64;
    }
    goto state0;

state62: // A\t/=\*\t\n\t\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 10: 
        goto state7;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    }
    goto state0;

state63: // A\t/=\*\t\n\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    }
    goto state0;

state64: // A\t/=\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state62;
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

state65: // A\t/=\*\t\n\t;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    }
    goto state0;

state66: // A\t/=\*\t\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    }
    goto state0;

state67: // A\t/=\*\t\s
    switch (*ptr++) {
    case 10: 
        goto state60;
    case 13: 
        goto state66;
    case 9: case ' ': 
        goto state67;
    case ';': 
        goto state68;
    }
    goto state0;

state68: // A\t/=\*\t\s;
    switch (*ptr++) {
    case 10: 
        goto state60;
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
        goto state68;
    case 13: 
        goto state69;
    }
    goto state0;

state69: // A\t/=\*\t\s;\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    }
    goto state0;

state70: // A\t/=\*\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state71;
    }
    goto state0;

state71: // A\t/=\*\n\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state71;
    }
    goto state0;

state72: // A\t/=\*\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state73;
    }
    goto state0;

state73: // A\t/=\*\r\r
    switch (*ptr++) {
    case 0: 
        goto state1;
    case ';': 
        goto state10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state12;
    case 9: case ' ': 
        goto state61;
    case 10: 
        goto state62;
    case 13: 
        goto state73;
    }
    goto state0;

state74: // A\t/=\*\s
    switch (*ptr++) {
    case 10: 
        goto state60;
    case 13: 
        goto state66;
    case 9: 
        goto state67;
    case ';': 
        goto state68;
    case ' ': 
        goto state74;
    }
    goto state0;

state75: // A\t/=\*;
    switch (*ptr++) {
    case 10: 
        goto state60;
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
        goto state68;
    case 13: 
        goto state69;
    case ';': 
        goto state75;
    }
    goto state0;

state76: // A\t;
    switch (*ptr++) {
    case 10: 
        goto state50;
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
        goto state76;
    case 13: 
        goto state77;
    }
    goto state0;

state77: // A\t;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state49;
    case 10: 
        goto state50;
    }
    goto state0;
}

//===========================================================================
bool AbnfParser::stateAlternation (const char *& ptr) {
    const char * last{nullptr};
    goto state2;

state0: // <FAILED>
    if (last) {
        ptr = last;
        return true;
    }
    return false;

state1: // <DONE>
    return true;

state2: // 
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state225;
    case '(': 
        goto state250;
    case '*': 
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
        goto state269;
    case '[': 
        goto state270;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state287;
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
        goto state4;
    }
    goto state0;

state4: // "\s
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
        goto state4;
    case '"': 
        goto state5;
    }
    goto state0;

state5: // "\s"
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    }
    goto state0;

state6: // "\s"\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state7;
    case 13: 
        goto state8;
    case '"': 
        goto state9;
    case '%': 
        goto state11;
    case '/': 
        goto state18;
    case '(': 
        goto state185;
    case '*': 
        goto state202;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state204;
    case '[': 
        goto state205;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state222;
    case ';': 
        goto state223;
    }
    goto state0;

state7: // "\s"\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    }
    goto state0;

state8: // "\s"\t\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state7;
    }
    goto state0;

state9: // "\s"\t"
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
        goto state10;
    }
    goto state0;

state10: // "\s"\t"\s
    switch (*ptr++) {
    case '"': 
        goto state5;
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
        goto state10;
    }
    goto state0;

state11: // "\s"\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state12;
    case 'D': case 'd': 
        goto state169;
    case 'X': case 'x': 
        goto state177;
    }
    goto state0;

state12: // "\s"\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state13;
    }
    goto state0;

state13: // "\s"\t%B0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case '0': case '1': 
        goto state13;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '-': 
        goto state16;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state165;
    }
    goto state0;

state14: // "\s"\t%B0\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    }
    goto state0;

state15: // "\s"\t%B0\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    }
    goto state0;

state16: // "\s"\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state17;
    }
    goto state0;

state17: // "\s"\t%B0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '0': case '1': 
        goto state17;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    }
    goto state0;

state18: // "\s"\t%B0-0/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state18;
    case 10: 
        goto state19;
    case 13: 
        goto state20;
    case '"': 
        goto state21;
    case '%': 
        goto state98;
    case '(': 
        goto state123;
    case '*': 
        goto state140;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state142;
    case '[': 
        goto state143;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    case ';': 
        goto state161;
    }
    goto state0;

state19: // "\s"\t%B0-0/\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state18;
    }
    goto state0;

state20: // "\s"\t%B0-0/\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state18;
    case 10: 
        goto state19;
    }
    goto state0;

state21: // "\s"\t%B0-0/"
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
        goto state22;
    }
    goto state0;

state22: // "\s"\t%B0-0/"\s
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
        goto state22;
    case '"': 
        goto state23;
    }
    goto state0;

state23: // "\s"\t%B0-0/"\s"
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    }
    goto state0;

state24: // "\s"\t%B0-0/"\s"\t
    switch (*ptr++) {
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    case 13: 
        goto state26;
    case '"': 
        goto state27;
    case '%': 
        goto state29;
    case '(': 
        goto state58;
    case '*': 
        goto state75;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state77;
    case '[': 
        goto state78;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state95;
    case ';': 
        goto state96;
    }
    goto state0;

state25: // "\s"\t%B0-0/"\s"\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    }
    goto state0;

state26: // "\s"\t%B0-0/"\s"\t\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    }
    goto state0;

state27: // "\s"\t%B0-0/"\s"\t"
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
        goto state28;
    }
    goto state0;

state28: // "\s"\t%B0-0/"\s"\t"\s
    switch (*ptr++) {
    case '"': 
        goto state23;
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
        goto state28;
    }
    goto state0;

state29: // "\s"\t%B0-0/"\s"\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state30;
    case 'D': case 'd': 
        goto state42;
    case 'X': case 'x': 
        goto state50;
    }
    goto state0;

state30: // "\s"\t%B0-0/"\s"\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state31;
    }
    goto state0;

state31: // "\s"\t%B0-0/"\s"\t%B0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case '0': case '1': 
        goto state31;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case '-': 
        goto state34;
    case ';': 
        goto state36;
    case '.': 
        goto state38;
    }
    goto state0;

state32: // "\s"\t%B0-0/"\s"\t%B0\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    }
    goto state0;

state33: // "\s"\t%B0-0/"\s"\t%B0\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    }
    goto state0;

state34: // "\s"\t%B0-0/"\s"\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state35;
    }
    goto state0;

state35: // "\s"\t%B0-0/"\s"\t%B0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case '0': case '1': 
        goto state35;
    case ';': 
        goto state36;
    }
    goto state0;

state36: // "\s"\t%B0-0/"\s"\t%B0-0;
    switch (*ptr++) {
    case 10: 
        goto state32;
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
        goto state36;
    case 13: 
        goto state37;
    }
    goto state0;

state37: // "\s"\t%B0-0/"\s"\t%B0-0;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    }
    goto state0;

state38: // "\s"\t%B0-0/"\s"\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state39;
    }
    goto state0;

state39: // "\s"\t%B0-0/"\s"\t%B0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': 
        goto state39;
    case '.': 
        goto state40;
    }
    goto state0;

state40: // "\s"\t%B0-0/"\s"\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state41;
    }
    goto state0;

state41: // "\s"\t%B0-0/"\s"\t%B0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state40;
    case '0': case '1': 
        goto state41;
    }
    goto state0;

state42: // "\s"\t%B0-0/"\s"\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state43;
    }
    goto state0;

state43: // "\s"\t%B0-0/"\s"\t%D0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state43;
    case '-': 
        goto state44;
    case '.': 
        goto state46;
    }
    goto state0;

state44: // "\s"\t%B0-0/"\s"\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state45;
    }
    goto state0;

state45: // "\s"\t%B0-0/"\s"\t%D0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state45;
    }
    goto state0;

state46: // "\s"\t%B0-0/"\s"\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state47;
    }
    goto state0;

state47: // "\s"\t%B0-0/"\s"\t%D0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state47;
    case '.': 
        goto state48;
    }
    goto state0;

state48: // "\s"\t%B0-0/"\s"\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state49;
    }
    goto state0;

state49: // "\s"\t%B0-0/"\s"\t%D0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state48;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state49;
    }
    goto state0;

state50: // "\s"\t%B0-0/"\s"\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state51;
    }
    goto state0;

state51: // "\s"\t%B0-0/"\s"\t%X0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state51;
    case '-': 
        goto state52;
    case '.': 
        goto state54;
    }
    goto state0;

state52: // "\s"\t%B0-0/"\s"\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state53;
    }
    goto state0;

state53: // "\s"\t%B0-0/"\s"\t%X0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state53;
    }
    goto state0;

state54: // "\s"\t%B0-0/"\s"\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state55;
    }
    goto state0;

state55: // "\s"\t%B0-0/"\s"\t%X0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state55;
    case '.': 
        goto state56;
    }
    goto state0;

state56: // "\s"\t%B0-0/"\s"\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state57;
    }
    goto state0;

state57: // "\s"\t%B0-0/"\s"\t%X0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state56;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state57;
    }
    goto state0;

state58: // "\s"\t%B0-0/"\s"\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state58;
    case 10: 
        goto state59;
    case 13: 
        goto state60;
    case ';': 
        goto state61;
    }
    if (stateAlternation(--ptr)) {
        goto state63;
    }
    goto state0;

state59: // "\s"\t%B0-0/"\s"\t(\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state58;
    }
    goto state0;

state60: // "\s"\t%B0-0/"\s"\t(\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state58;
    case 10: 
        goto state59;
    }
    goto state0;

state61: // "\s"\t%B0-0/"\s"\t(;
    switch (*ptr++) {
    case 10: 
        goto state59;
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
        goto state61;
    case 13: 
        goto state62;
    }
    goto state0;

state62: // "\s"\t%B0-0/"\s"\t(;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state58;
    case 10: 
        goto state59;
    }
    goto state0;

state63: // "\s"\t%B0-0/"\s"\t(\*
    switch (*ptr++) {
    case 9: 
        goto state64;
    case 10: 
        goto state70;
    case 13: 
        goto state71;
    case ' ': 
        goto state72;
    case ')': 
        goto state73;
    case ';': 
        goto state74;
    }
    goto state0;

state64: // "\s"\t%B0-0/"\s"\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 9: 
        goto state64;
    case 10: 
        goto state65;
    case ' ': 
        goto state66;
    case 13: 
        goto state67;
    case ';': 
        goto state68;
    }
    goto state0;

state65: // "\s"\t%B0-0/"\s"\t(\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state66;
    }
    goto state0;

state66: // "\s"\t%B0-0/"\s"\t(\*\t\n\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 10: 
        goto state65;
    case 9: case ' ': 
        goto state66;
    case 13: 
        goto state67;
    case ';': 
        goto state68;
    }
    goto state0;

state67: // "\s"\t%B0-0/"\s"\t(\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state65;
    case 9: case ' ': 
        goto state66;
    }
    goto state0;

state68: // "\s"\t%B0-0/"\s"\t(\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state65;
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
        goto state68;
    case 13: 
        goto state69;
    }
    goto state0;

state69: // "\s"\t%B0-0/"\s"\t(\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state65;
    case 9: case ' ': 
        goto state66;
    }
    goto state0;

state70: // "\s"\t%B0-0/"\s"\t(\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state66;
    case 10: 
        goto state70;
    }
    goto state0;

state71: // "\s"\t%B0-0/"\s"\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state65;
    case 9: case ' ': 
        goto state66;
    case 13: 
        goto state71;
    }
    goto state0;

state72: // "\s"\t%B0-0/"\s"\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 10: 
        goto state65;
    case 9: 
        goto state66;
    case 13: 
        goto state67;
    case ';': 
        goto state68;
    case ' ': 
        goto state72;
    }
    goto state0;

state73: // "\s"\t%B0-0/"\s"\t(\*)
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case ')': 
        goto state73;
    }
    goto state0;

state74: // "\s"\t%B0-0/"\s"\t(\*;
    switch (*ptr++) {
    case 10: 
        goto state65;
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
        goto state68;
    case 13: 
        goto state69;
    case ';': 
        goto state74;
    }
    goto state0;

state75: // "\s"\t%B0-0/"\s"\t*
    switch (*ptr++) {
    case '"': 
        goto state27;
    case '%': 
        goto state29;
    case '(': 
        goto state58;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state76;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state77;
    case '[': 
        goto state78;
    }
    goto state0;

state76: // "\s"\t%B0-0/"\s"\t*0
    switch (*ptr++) {
    case '"': 
        goto state27;
    case '%': 
        goto state29;
    case '(': 
        goto state58;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state76;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state77;
    case '[': 
        goto state78;
    }
    goto state0;

state77: // "\s"\t%B0-0/"\s"\t*0A
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
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
        goto state77;
    }
    goto state0;

state78: // "\s"\t%B0-0/"\s"\t*0[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state78;
    case 10: 
        goto state79;
    case 13: 
        goto state80;
    case ';': 
        goto state81;
    }
    if (stateAlternation(--ptr)) {
        goto state83;
    }
    goto state0;

state79: // "\s"\t%B0-0/"\s"\t*0[\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state78;
    }
    goto state0;

state80: // "\s"\t%B0-0/"\s"\t*0[\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state78;
    case 10: 
        goto state79;
    }
    goto state0;

state81: // "\s"\t%B0-0/"\s"\t*0[;
    switch (*ptr++) {
    case 10: 
        goto state79;
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
        goto state81;
    case 13: 
        goto state82;
    }
    goto state0;

state82: // "\s"\t%B0-0/"\s"\t*0[;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state78;
    case 10: 
        goto state79;
    }
    goto state0;

state83: // "\s"\t%B0-0/"\s"\t*0[\*
    switch (*ptr++) {
    case 9: 
        goto state84;
    case 10: 
        goto state90;
    case 13: 
        goto state91;
    case ' ': 
        goto state92;
    case ';': 
        goto state93;
    case ']': 
        goto state94;
    }
    goto state0;

state84: // "\s"\t%B0-0/"\s"\t*0[\*\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 9: 
        goto state84;
    case 10: 
        goto state85;
    case ' ': 
        goto state86;
    case 13: 
        goto state87;
    case ';': 
        goto state88;
    }
    goto state0;

state85: // "\s"\t%B0-0/"\s"\t*0[\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state86;
    }
    goto state0;

state86: // "\s"\t%B0-0/"\s"\t*0[\*\t\n\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 10: 
        goto state85;
    case 9: case ' ': 
        goto state86;
    case 13: 
        goto state87;
    case ';': 
        goto state88;
    }
    goto state0;

state87: // "\s"\t%B0-0/"\s"\t*0[\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state85;
    case 9: case ' ': 
        goto state86;
    }
    goto state0;

state88: // "\s"\t%B0-0/"\s"\t*0[\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state85;
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
        goto state88;
    case 13: 
        goto state89;
    }
    goto state0;

state89: // "\s"\t%B0-0/"\s"\t*0[\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state85;
    case 9: case ' ': 
        goto state86;
    }
    goto state0;

state90: // "\s"\t%B0-0/"\s"\t*0[\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state86;
    case 10: 
        goto state90;
    }
    goto state0;

state91: // "\s"\t%B0-0/"\s"\t*0[\*\r
    switch (*ptr++) {
    case 10: 
        goto state85;
    case 9: case ' ': 
        goto state86;
    case 13: 
        goto state91;
    }
    goto state0;

state92: // "\s"\t%B0-0/"\s"\t*0[\*\s
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 10: 
        goto state85;
    case 9: 
        goto state86;
    case 13: 
        goto state87;
    case ';': 
        goto state88;
    case ' ': 
        goto state92;
    }
    goto state0;

state93: // "\s"\t%B0-0/"\s"\t*0[\*;
    switch (*ptr++) {
    case 10: 
        goto state85;
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
        goto state88;
    case 13: 
        goto state89;
    case ';': 
        goto state93;
    }
    goto state0;

state94: // "\s"\t%B0-0/"\s"\t*0[\*]
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case ']': 
        goto state94;
    }
    goto state0;

state95: // "\s"\t%B0-0/"\s"\t0
    switch (*ptr++) {
    case '"': 
        goto state27;
    case '%': 
        goto state29;
    case '(': 
        goto state58;
    case '*': 
        goto state75;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state77;
    case '[': 
        goto state78;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state95;
    }
    goto state0;

state96: // "\s"\t%B0-0/"\s"\t;
    switch (*ptr++) {
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
        goto state96;
    case 13: 
        goto state97;
    }
    goto state0;

state97: // "\s"\t%B0-0/"\s"\t;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state25;
    }
    goto state0;

state98: // "\s"\t%B0-0/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state99;
    case 'D': case 'd': 
        goto state107;
    case 'X': case 'x': 
        goto state115;
    }
    goto state0;

state99: // "\s"\t%B0-0/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state100;
    }
    goto state0;

state100: // "\s"\t%B0-0/%B0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': 
        goto state100;
    case '-': 
        goto state101;
    case '.': 
        goto state103;
    }
    goto state0;

state101: // "\s"\t%B0-0/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state102;
    }
    goto state0;

state102: // "\s"\t%B0-0/%B0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': 
        goto state102;
    }
    goto state0;

state103: // "\s"\t%B0-0/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state104;
    }
    goto state0;

state104: // "\s"\t%B0-0/%B0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': 
        goto state104;
    case '.': 
        goto state105;
    }
    goto state0;

state105: // "\s"\t%B0-0/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state106;
    }
    goto state0;

state106: // "\s"\t%B0-0/%B0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state105;
    case '0': case '1': 
        goto state106;
    }
    goto state0;

state107: // "\s"\t%B0-0/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state108;
    }
    goto state0;

state108: // "\s"\t%B0-0/%D0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state108;
    case '-': 
        goto state109;
    case '.': 
        goto state111;
    }
    goto state0;

state109: // "\s"\t%B0-0/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state110;
    }
    goto state0;

state110: // "\s"\t%B0-0/%D0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state110;
    }
    goto state0;

state111: // "\s"\t%B0-0/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state112;
    }
    goto state0;

state112: // "\s"\t%B0-0/%D0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state112;
    case '.': 
        goto state113;
    }
    goto state0;

state113: // "\s"\t%B0-0/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state114;
    }
    goto state0;

state114: // "\s"\t%B0-0/%D0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state113;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state114;
    }
    goto state0;

state115: // "\s"\t%B0-0/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state116;
    }
    goto state0;

state116: // "\s"\t%B0-0/%X0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state116;
    case '-': 
        goto state117;
    case '.': 
        goto state119;
    }
    goto state0;

state117: // "\s"\t%B0-0/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state118;
    }
    goto state0;

state118: // "\s"\t%B0-0/%X0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state118;
    }
    goto state0;

state119: // "\s"\t%B0-0/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state120;
    }
    goto state0;

state120: // "\s"\t%B0-0/%X0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state120;
    case '.': 
        goto state121;
    }
    goto state0;

state121: // "\s"\t%B0-0/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state122;
    }
    goto state0;

state122: // "\s"\t%B0-0/%X0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case '.': 
        goto state121;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state122;
    }
    goto state0;

state123: // "\s"\t%B0-0/(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state123;
    case 10: 
        goto state124;
    case 13: 
        goto state125;
    case ';': 
        goto state126;
    }
    if (stateAlternation(--ptr)) {
        goto state128;
    }
    goto state0;

state124: // "\s"\t%B0-0/(\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state123;
    }
    goto state0;

state125: // "\s"\t%B0-0/(\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state123;
    case 10: 
        goto state124;
    }
    goto state0;

state126: // "\s"\t%B0-0/(;
    switch (*ptr++) {
    case 10: 
        goto state124;
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

state127: // "\s"\t%B0-0/(;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state123;
    case 10: 
        goto state124;
    }
    goto state0;

state128: // "\s"\t%B0-0/(\*
    switch (*ptr++) {
    case 9: 
        goto state129;
    case 10: 
        goto state135;
    case 13: 
        goto state136;
    case ' ': 
        goto state137;
    case ')': 
        goto state138;
    case ';': 
        goto state139;
    }
    goto state0;

state129: // "\s"\t%B0-0/(\*\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 9: 
        goto state129;
    case 10: 
        goto state130;
    case ' ': 
        goto state131;
    case 13: 
        goto state132;
    case ';': 
        goto state133;
    }
    goto state0;

state130: // "\s"\t%B0-0/(\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state131;
    }
    goto state0;

state131: // "\s"\t%B0-0/(\*\t\n\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 10: 
        goto state130;
    case 9: case ' ': 
        goto state131;
    case 13: 
        goto state132;
    case ';': 
        goto state133;
    }
    goto state0;

state132: // "\s"\t%B0-0/(\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state130;
    case 9: case ' ': 
        goto state131;
    }
    goto state0;

state133: // "\s"\t%B0-0/(\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state130;
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
        goto state133;
    case 13: 
        goto state134;
    }
    goto state0;

state134: // "\s"\t%B0-0/(\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state130;
    case 9: case ' ': 
        goto state131;
    }
    goto state0;

state135: // "\s"\t%B0-0/(\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state131;
    case 10: 
        goto state135;
    }
    goto state0;

state136: // "\s"\t%B0-0/(\*\r
    switch (*ptr++) {
    case 10: 
        goto state130;
    case 9: case ' ': 
        goto state131;
    case 13: 
        goto state136;
    }
    goto state0;

state137: // "\s"\t%B0-0/(\*\s
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 10: 
        goto state130;
    case 9: 
        goto state131;
    case 13: 
        goto state132;
    case ';': 
        goto state133;
    case ' ': 
        goto state137;
    }
    goto state0;

state138: // "\s"\t%B0-0/(\*)
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case ')': 
        goto state138;
    }
    goto state0;

state139: // "\s"\t%B0-0/(\*;
    switch (*ptr++) {
    case 10: 
        goto state130;
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
        goto state133;
    case 13: 
        goto state134;
    case ';': 
        goto state139;
    }
    goto state0;

state140: // "\s"\t%B0-0/*
    switch (*ptr++) {
    case '"': 
        goto state21;
    case '%': 
        goto state98;
    case '(': 
        goto state123;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state141;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state142;
    case '[': 
        goto state143;
    }
    goto state0;

state141: // "\s"\t%B0-0/*0
    switch (*ptr++) {
    case '"': 
        goto state21;
    case '%': 
        goto state98;
    case '(': 
        goto state123;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state141;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state142;
    case '[': 
        goto state143;
    }
    goto state0;

state142: // "\s"\t%B0-0/*0A
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
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
        goto state142;
    }
    goto state0;

state143: // "\s"\t%B0-0/*0[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    case 13: 
        goto state145;
    case ';': 
        goto state146;
    }
    if (stateAlternation(--ptr)) {
        goto state148;
    }
    goto state0;

state144: // "\s"\t%B0-0/*0[\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state143;
    }
    goto state0;

state145: // "\s"\t%B0-0/*0[\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    }
    goto state0;

state146: // "\s"\t%B0-0/*0[;
    switch (*ptr++) {
    case 10: 
        goto state144;
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
        goto state146;
    case 13: 
        goto state147;
    }
    goto state0;

state147: // "\s"\t%B0-0/*0[;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state143;
    case 10: 
        goto state144;
    }
    goto state0;

state148: // "\s"\t%B0-0/*0[\*
    switch (*ptr++) {
    case 9: 
        goto state149;
    case 10: 
        goto state155;
    case 13: 
        goto state156;
    case ' ': 
        goto state157;
    case ';': 
        goto state158;
    case ']': 
        goto state159;
    }
    goto state0;

state149: // "\s"\t%B0-0/*0[\*\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 9: 
        goto state149;
    case 10: 
        goto state150;
    case ' ': 
        goto state151;
    case 13: 
        goto state152;
    case ';': 
        goto state153;
    }
    goto state0;

state150: // "\s"\t%B0-0/*0[\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state151;
    }
    goto state0;

state151: // "\s"\t%B0-0/*0[\*\t\n\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 10: 
        goto state150;
    case 9: case ' ': 
        goto state151;
    case 13: 
        goto state152;
    case ';': 
        goto state153;
    }
    goto state0;

state152: // "\s"\t%B0-0/*0[\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state150;
    case 9: case ' ': 
        goto state151;
    }
    goto state0;

state153: // "\s"\t%B0-0/*0[\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state150;
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

state154: // "\s"\t%B0-0/*0[\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state150;
    case 9: case ' ': 
        goto state151;
    }
    goto state0;

state155: // "\s"\t%B0-0/*0[\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state151;
    case 10: 
        goto state155;
    }
    goto state0;

state156: // "\s"\t%B0-0/*0[\*\r
    switch (*ptr++) {
    case 10: 
        goto state150;
    case 9: case ' ': 
        goto state151;
    case 13: 
        goto state156;
    }
    goto state0;

state157: // "\s"\t%B0-0/*0[\*\s
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 10: 
        goto state150;
    case 9: 
        goto state151;
    case 13: 
        goto state152;
    case ';': 
        goto state153;
    case ' ': 
        goto state157;
    }
    goto state0;

state158: // "\s"\t%B0-0/*0[\*;
    switch (*ptr++) {
    case 10: 
        goto state150;
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
        goto state153;
    case 13: 
        goto state154;
    case ';': 
        goto state158;
    }
    goto state0;

state159: // "\s"\t%B0-0/*0[\*]
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case '/': 
        goto state18;
    case 9: case ' ': 
        goto state24;
    case 10: 
        goto state32;
    case 13: 
        goto state33;
    case ';': 
        goto state36;
    case ']': 
        goto state159;
    }
    goto state0;

state160: // "\s"\t%B0-0/0
    switch (*ptr++) {
    case '"': 
        goto state21;
    case '%': 
        goto state98;
    case '(': 
        goto state123;
    case '*': 
        goto state140;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state142;
    case '[': 
        goto state143;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    }
    goto state0;

state161: // "\s"\t%B0-0/;
    switch (*ptr++) {
    case 10: 
        goto state19;
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
        goto state161;
    case 13: 
        goto state162;
    }
    goto state0;

state162: // "\s"\t%B0-0/;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state18;
    case 10: 
        goto state19;
    }
    goto state0;

state163: // "\s"\t%B0-0;
    switch (*ptr++) {
    case 10: 
        goto state14;
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
        goto state163;
    case 13: 
        goto state164;
    }
    goto state0;

state164: // "\s"\t%B0-0;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    }
    goto state0;

state165: // "\s"\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state166;
    }
    goto state0;

state166: // "\s"\t%B0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': 
        goto state166;
    case '.': 
        goto state167;
    }
    goto state0;

state167: // "\s"\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state168;
    }
    goto state0;

state168: // "\s"\t%B0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state167;
    case '0': case '1': 
        goto state168;
    }
    goto state0;

state169: // "\s"\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state170;
    }
    goto state0;

state170: // "\s"\t%D0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state170;
    case '-': 
        goto state171;
    case '.': 
        goto state173;
    }
    goto state0;

state171: // "\s"\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state172;
    }
    goto state0;

state172: // "\s"\t%D0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state172;
    }
    goto state0;

state173: // "\s"\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state174;
    }
    goto state0;

state174: // "\s"\t%D0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state174;
    case '.': 
        goto state175;
    }
    goto state0;

state175: // "\s"\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state176;
    }
    goto state0;

state176: // "\s"\t%D0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state175;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state176;
    }
    goto state0;

state177: // "\s"\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state178;
    }
    goto state0;

state178: // "\s"\t%X0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state178;
    case '-': 
        goto state179;
    case '.': 
        goto state181;
    }
    goto state0;

state179: // "\s"\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state180;
    }
    goto state0;

state180: // "\s"\t%X0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state180;
    }
    goto state0;

state181: // "\s"\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state182;
    }
    goto state0;

state182: // "\s"\t%X0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state182;
    case '.': 
        goto state183;
    }
    goto state0;

state183: // "\s"\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state184;
    }
    goto state0;

state184: // "\s"\t%X0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state183;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state184;
    }
    goto state0;

state185: // "\s"\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state185;
    case 10: 
        goto state186;
    case 13: 
        goto state187;
    case ';': 
        goto state188;
    }
    if (stateAlternation(--ptr)) {
        goto state190;
    }
    goto state0;

state186: // "\s"\t(\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state185;
    }
    goto state0;

state187: // "\s"\t(\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state185;
    case 10: 
        goto state186;
    }
    goto state0;

state188: // "\s"\t(;
    switch (*ptr++) {
    case 10: 
        goto state186;
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
        goto state188;
    case 13: 
        goto state189;
    }
    goto state0;

state189: // "\s"\t(;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state185;
    case 10: 
        goto state186;
    }
    goto state0;

state190: // "\s"\t(\*
    switch (*ptr++) {
    case 9: 
        goto state191;
    case 10: 
        goto state197;
    case 13: 
        goto state198;
    case ' ': 
        goto state199;
    case ')': 
        goto state200;
    case ';': 
        goto state201;
    }
    goto state0;

state191: // "\s"\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 9: 
        goto state191;
    case 10: 
        goto state192;
    case ' ': 
        goto state193;
    case 13: 
        goto state194;
    case ';': 
        goto state195;
    }
    goto state0;

state192: // "\s"\t(\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state193;
    }
    goto state0;

state193: // "\s"\t(\*\t\n\t
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 10: 
        goto state192;
    case 9: case ' ': 
        goto state193;
    case 13: 
        goto state194;
    case ';': 
        goto state195;
    }
    goto state0;

state194: // "\s"\t(\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state192;
    case 9: case ' ': 
        goto state193;
    }
    goto state0;

state195: // "\s"\t(\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state192;
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
        goto state195;
    case 13: 
        goto state196;
    }
    goto state0;

state196: // "\s"\t(\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state192;
    case 9: case ' ': 
        goto state193;
    }
    goto state0;

state197: // "\s"\t(\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state193;
    case 10: 
        goto state197;
    }
    goto state0;

state198: // "\s"\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state192;
    case 9: case ' ': 
        goto state193;
    case 13: 
        goto state198;
    }
    goto state0;

state199: // "\s"\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 10: 
        goto state192;
    case 9: 
        goto state193;
    case 13: 
        goto state194;
    case ';': 
        goto state195;
    case ' ': 
        goto state199;
    }
    goto state0;

state200: // "\s"\t(\*)
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case ')': 
        goto state200;
    }
    goto state0;

state201: // "\s"\t(\*;
    switch (*ptr++) {
    case 10: 
        goto state192;
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
        goto state195;
    case 13: 
        goto state196;
    case ';': 
        goto state201;
    }
    goto state0;

state202: // "\s"\t*
    switch (*ptr++) {
    case '"': 
        goto state9;
    case '%': 
        goto state11;
    case '(': 
        goto state185;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state203;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state204;
    case '[': 
        goto state205;
    }
    goto state0;

state203: // "\s"\t*0
    switch (*ptr++) {
    case '"': 
        goto state9;
    case '%': 
        goto state11;
    case '(': 
        goto state185;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state203;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state204;
    case '[': 
        goto state205;
    }
    goto state0;

state204: // "\s"\t*0A
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
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
        goto state204;
    }
    goto state0;

state205: // "\s"\t*0[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state205;
    case 10: 
        goto state206;
    case 13: 
        goto state207;
    case ';': 
        goto state208;
    }
    if (stateAlternation(--ptr)) {
        goto state210;
    }
    goto state0;

state206: // "\s"\t*0[\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state205;
    }
    goto state0;

state207: // "\s"\t*0[\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state205;
    case 10: 
        goto state206;
    }
    goto state0;

state208: // "\s"\t*0[;
    switch (*ptr++) {
    case 10: 
        goto state206;
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
        goto state208;
    case 13: 
        goto state209;
    }
    goto state0;

state209: // "\s"\t*0[;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state205;
    case 10: 
        goto state206;
    }
    goto state0;

state210: // "\s"\t*0[\*
    switch (*ptr++) {
    case 9: 
        goto state211;
    case 10: 
        goto state217;
    case 13: 
        goto state218;
    case ' ': 
        goto state219;
    case ';': 
        goto state220;
    case ']': 
        goto state221;
    }
    goto state0;

state211: // "\s"\t*0[\*\t
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 9: 
        goto state211;
    case 10: 
        goto state212;
    case ' ': 
        goto state213;
    case 13: 
        goto state214;
    case ';': 
        goto state215;
    }
    goto state0;

state212: // "\s"\t*0[\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state213;
    }
    goto state0;

state213: // "\s"\t*0[\*\t\n\t
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 10: 
        goto state212;
    case 9: case ' ': 
        goto state213;
    case 13: 
        goto state214;
    case ';': 
        goto state215;
    }
    goto state0;

state214: // "\s"\t*0[\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state212;
    case 9: case ' ': 
        goto state213;
    }
    goto state0;

state215: // "\s"\t*0[\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state212;
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
        goto state215;
    case 13: 
        goto state216;
    }
    goto state0;

state216: // "\s"\t*0[\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state212;
    case 9: case ' ': 
        goto state213;
    }
    goto state0;

state217: // "\s"\t*0[\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state213;
    case 10: 
        goto state217;
    }
    goto state0;

state218: // "\s"\t*0[\*\r
    switch (*ptr++) {
    case 10: 
        goto state212;
    case 9: case ' ': 
        goto state213;
    case 13: 
        goto state218;
    }
    goto state0;

state219: // "\s"\t*0[\*\s
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 10: 
        goto state212;
    case 9: 
        goto state213;
    case 13: 
        goto state214;
    case ';': 
        goto state215;
    case ' ': 
        goto state219;
    }
    goto state0;

state220: // "\s"\t*0[\*;
    switch (*ptr++) {
    case 10: 
        goto state212;
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
        goto state215;
    case 13: 
        goto state216;
    case ';': 
        goto state220;
    }
    goto state0;

state221: // "\s"\t*0[\*]
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case ']': 
        goto state221;
    }
    goto state0;

state222: // "\s"\t0
    switch (*ptr++) {
    case '"': 
        goto state9;
    case '%': 
        goto state11;
    case '(': 
        goto state185;
    case '*': 
        goto state202;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state204;
    case '[': 
        goto state205;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state222;
    }
    goto state0;

state223: // "\s"\t;
    switch (*ptr++) {
    case 10: 
        goto state7;
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
        goto state223;
    case 13: 
        goto state224;
    }
    goto state0;

state224: // "\s"\t;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state7;
    }
    goto state0;

state225: // %
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state226;
    case 'D': case 'd': 
        goto state234;
    case 'X': case 'x': 
        goto state242;
    }
    goto state0;

state226: // %B
    switch (*ptr++) {
    case '0': case '1': 
        goto state227;
    }
    goto state0;

state227: // %B0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': 
        goto state227;
    case '-': 
        goto state228;
    case '.': 
        goto state230;
    }
    goto state0;

state228: // %B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state229;
    }
    goto state0;

state229: // %B0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': 
        goto state229;
    }
    goto state0;

state230: // %B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state231;
    }
    goto state0;

state231: // %B0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': 
        goto state231;
    case '.': 
        goto state232;
    }
    goto state0;

state232: // %B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state233;
    }
    goto state0;

state233: // %B0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state232;
    case '0': case '1': 
        goto state233;
    }
    goto state0;

state234: // %D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state235;
    }
    goto state0;

state235: // %D0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state235;
    case '-': 
        goto state236;
    case '.': 
        goto state238;
    }
    goto state0;

state236: // %D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state237;
    }
    goto state0;

state237: // %D0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state237;
    }
    goto state0;

state238: // %D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state239;
    }
    goto state0;

state239: // %D0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state239;
    case '.': 
        goto state240;
    }
    goto state0;

state240: // %D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state241;
    }
    goto state0;

state241: // %D0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state240;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state241;
    }
    goto state0;

state242: // %X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state243;
    }
    goto state0;

state243: // %X0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state243;
    case '-': 
        goto state244;
    case '.': 
        goto state246;
    }
    goto state0;

state244: // %X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state245;
    }
    goto state0;

state245: // %X0-0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state245;
    }
    goto state0;

state246: // %X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state247;
    }
    goto state0;

state247: // %X0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state247;
    case '.': 
        goto state248;
    }
    goto state0;

state248: // %X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state249;
    }
    goto state0;

state249: // %X0.0.0
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case '.': 
        goto state248;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state249;
    }
    goto state0;

state250: // (
    switch (*ptr++) {
    case 9: case ' ': 
        goto state250;
    case 10: 
        goto state251;
    case 13: 
        goto state252;
    case ';': 
        goto state253;
    }
    if (stateAlternation(--ptr)) {
        goto state255;
    }
    goto state0;

state251: // (\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state250;
    }
    goto state0;

state252: // (\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state250;
    case 10: 
        goto state251;
    }
    goto state0;

state253: // (;
    switch (*ptr++) {
    case 10: 
        goto state251;
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
        goto state253;
    case 13: 
        goto state254;
    }
    goto state0;

state254: // (;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state250;
    case 10: 
        goto state251;
    }
    goto state0;

state255: // (\*
    switch (*ptr++) {
    case 9: 
        goto state256;
    case 10: 
        goto state262;
    case 13: 
        goto state263;
    case ' ': 
        goto state264;
    case ')': 
        goto state265;
    case ';': 
        goto state266;
    }
    goto state0;

state256: // (\*\t
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 9: 
        goto state256;
    case 10: 
        goto state257;
    case ' ': 
        goto state258;
    case 13: 
        goto state259;
    case ';': 
        goto state260;
    }
    goto state0;

state257: // (\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state258;
    }
    goto state0;

state258: // (\*\t\n\t
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 10: 
        goto state257;
    case 9: case ' ': 
        goto state258;
    case 13: 
        goto state259;
    case ';': 
        goto state260;
    }
    goto state0;

state259: // (\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state257;
    case 9: case ' ': 
        goto state258;
    }
    goto state0;

state260: // (\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state257;
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
        goto state260;
    case 13: 
        goto state261;
    }
    goto state0;

state261: // (\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state257;
    case 9: case ' ': 
        goto state258;
    }
    goto state0;

state262: // (\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state258;
    case 10: 
        goto state262;
    }
    goto state0;

state263: // (\*\r
    switch (*ptr++) {
    case 10: 
        goto state257;
    case 9: case ' ': 
        goto state258;
    case 13: 
        goto state263;
    }
    goto state0;

state264: // (\*\s
    switch (*ptr++) {
    case ')': 
        goto state5;
    case 10: 
        goto state257;
    case 9: 
        goto state258;
    case 13: 
        goto state259;
    case ';': 
        goto state260;
    case ' ': 
        goto state264;
    }
    goto state0;

state265: // (\*)
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case ')': 
        goto state265;
    }
    goto state0;

state266: // (\*;
    switch (*ptr++) {
    case 10: 
        goto state257;
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
        goto state260;
    case 13: 
        goto state261;
    case ';': 
        goto state266;
    }
    goto state0;

state267: // *
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state225;
    case '(': 
        goto state250;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state268;
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
    case '[': 
        goto state270;
    }
    goto state0;

state268: // *0
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state225;
    case '(': 
        goto state250;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state268;
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
    case '[': 
        goto state270;
    }
    goto state0;

state269: // *0A
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
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
        goto state269;
    }
    goto state0;

state270: // *0[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state270;
    case 10: 
        goto state271;
    case 13: 
        goto state272;
    case ';': 
        goto state273;
    }
    if (stateAlternation(--ptr)) {
        goto state275;
    }
    goto state0;

state271: // *0[\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state270;
    }
    goto state0;

state272: // *0[\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state270;
    case 10: 
        goto state271;
    }
    goto state0;

state273: // *0[;
    switch (*ptr++) {
    case 10: 
        goto state271;
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
        goto state273;
    case 13: 
        goto state274;
    }
    goto state0;

state274: // *0[;\r
    switch (*ptr++) {
    case 9: case ' ': 
        goto state270;
    case 10: 
        goto state271;
    }
    goto state0;

state275: // *0[\*
    switch (*ptr++) {
    case 9: 
        goto state276;
    case 10: 
        goto state282;
    case 13: 
        goto state283;
    case ' ': 
        goto state284;
    case ';': 
        goto state285;
    case ']': 
        goto state286;
    }
    goto state0;

state276: // *0[\*\t
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 9: 
        goto state276;
    case 10: 
        goto state277;
    case ' ': 
        goto state278;
    case 13: 
        goto state279;
    case ';': 
        goto state280;
    }
    goto state0;

state277: // *0[\*\t\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state278;
    }
    goto state0;

state278: // *0[\*\t\n\t
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 10: 
        goto state277;
    case 9: case ' ': 
        goto state278;
    case 13: 
        goto state279;
    case ';': 
        goto state280;
    }
    goto state0;

state279: // *0[\*\t\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state277;
    case 9: case ' ': 
        goto state278;
    }
    goto state0;

state280: // *0[\*\t\n\t;
    switch (*ptr++) {
    case 10: 
        goto state277;
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
        goto state280;
    case 13: 
        goto state281;
    }
    goto state0;

state281: // *0[\*\t\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state277;
    case 9: case ' ': 
        goto state278;
    }
    goto state0;

state282: // *0[\*\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state278;
    case 10: 
        goto state282;
    }
    goto state0;

state283: // *0[\*\r
    switch (*ptr++) {
    case 10: 
        goto state277;
    case 9: case ' ': 
        goto state278;
    case 13: 
        goto state283;
    }
    goto state0;

state284: // *0[\*\s
    switch (*ptr++) {
    case ']': 
        goto state5;
    case 10: 
        goto state277;
    case 9: 
        goto state278;
    case 13: 
        goto state279;
    case ';': 
        goto state280;
    case ' ': 
        goto state284;
    }
    goto state0;

state285: // *0[\*;
    switch (*ptr++) {
    case 10: 
        goto state277;
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
        goto state280;
    case 13: 
        goto state281;
    case ';': 
        goto state285;
    }
    goto state0;

state286: // *0[\*]
    last = ptr;
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 9: case ' ': 
        goto state6;
    case 10: 
        goto state14;
    case 13: 
        goto state15;
    case '/': 
        goto state18;
    case ';': 
        goto state163;
    case ']': 
        goto state286;
    }
    goto state0;

state287: // 0
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state225;
    case '(': 
        goto state250;
    case '*': 
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
        goto state269;
    case '[': 
        goto state270;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state287;
    }
    goto state0;
}

