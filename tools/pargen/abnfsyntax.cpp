// abnfsyntax.cpp - pargen
// clang-format off
#include "pch.h"
#pragma hdrstop


/****************************************************************************
*
*   AbnfParser
*
*   Normalized ABNF of syntax being checked: 
*   <ROOT> = rulelist
*   ALPHA = ( %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a )
*   BIT = ( %x30 / %x31 )
*   CR = %xd
*   CRLF = ( CR LF )
*   DIGIT = ( %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 )
*   DQUOTE = %x22
*   HEXDIG = ( DIGIT / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 )
*   HTAB = %x9
*   LF = %xa
*   SP = %x20
*   VCHAR = ( %x21 / %x22 / %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e )
*   WSP = ( SP / HTAB )
*   alternation* = ( concatenation *( *c-wsp %x2f *c-wsp concatenation ) )
*   bin-val = ( ( %x62 / %x42 ) 1*BIT *1( 1*( %x2e 1*BIT ) / ( %x2d 1*BIT ) ) )
*   c-nl = ( comment / CRLF )
*   c-wsp = ( WSP / ( c-nl WSP ) )
*   char-val = ( DQUOTE *( %x20 / %x21 / %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e ) DQUOTE )
*   comment = ( %x3b *( WSP / VCHAR ) CRLF )
*   concatenation = ( repetition *( 1*c-wsp repetition ) )
*   dec-val = ( ( %x64 / %x44 ) 1*DIGIT *1( 1*( %x2e 1*DIGIT ) / ( %x2d 1*DIGIT ) ) )
*   defined-as = ( *c-wsp ( %x3d / ( %x2f %x3d ) ) *c-wsp )
*   element = ( rulename / group / option / char-val / num-val )
*   elements = ( alternation *c-wsp )
*   group = ( %x28 *c-wsp alternation *c-wsp %x29 )
*   hex-val = ( ( %x78 / %x58 ) 1*HEXDIG *1( 1*( %x2e 1*HEXDIG ) / ( %x2d 1*HEXDIG ) ) )
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
        goto state28;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state30;
    }
    goto state0;

state3: // \t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 13: 
        goto state4;
    case ';': 
        goto state28;
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
        goto state26;
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
        goto state24;
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
        goto state24;
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
        goto state1;
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

state24: // \t\r\n\t\r\n\r\nA\t;
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
        goto state24;
    case 13: 
        goto state25;
    }
    goto state0;

state25: // \t\r\n\t\r\n\r\nA\t;\r
    switch (*ptr++) {
    case 10: 
        goto state17;
    }
    goto state0;

state26: // \t\r\n\t;
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
        goto state26;
    case 13: 
        goto state27;
    }
    goto state0;

state27: // \t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state8;
    }
    goto state0;

state28: // \t;
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
        goto state28;
    case 13: 
        goto state29;
    }
    goto state0;

state29: // \t;\r
    switch (*ptr++) {
    case 10: 
        goto state5;
    }
    goto state0;

state30: // A
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
        goto state30;
    case 9: case ' ': 
        goto state31;
    case 13: 
        goto state32;
    case '/': 
        goto state34;
    case '=': 
        goto state35;
    case ';': 
        goto state40;
    }
    goto state0;

state31: // A\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state31;
    case 13: 
        goto state32;
    case '/': 
        goto state34;
    case '=': 
        goto state35;
    case ';': 
        goto state40;
    }
    goto state0;

state32: // A\t\r
    switch (*ptr++) {
    case 10: 
        goto state33;
    }
    goto state0;

state33: // A\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state31;
    }
    goto state0;

state34: // A\t/
    switch (*ptr++) {
    case '=': 
        goto state35;
    }
    goto state0;

state35: // A\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state35;
    case 13: 
        goto state36;
    case ';': 
        goto state38;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state36: // A\t/=\r
    switch (*ptr++) {
    case 10: 
        goto state37;
    }
    goto state0;

state37: // A\t/=\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state35;
    }
    goto state0;

state38: // A\t/=;
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
        goto state38;
    case 13: 
        goto state39;
    }
    goto state0;

state39: // A\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state37;
    }
    goto state0;

state40: // A\t;
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
        goto state40;
    case 13: 
        goto state41;
    }
    goto state0;

state41: // A\t;\r
    switch (*ptr++) {
    case 10: 
        goto state33;
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
        goto state146;
    case '(': 
        goto state171;
    case '*': 
        goto state176;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state177;
    case '[': 
        goto state178;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state183;
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
        goto state109;
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
        goto state131;
    case '*': 
        goto state136;
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
    case '[': 
        goto state138;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state143;
    case ';': 
        goto state144;
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
        goto state115;
    case 'X': case 'x': 
        goto state123;
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
        goto state109;
    case '.': 
        goto state111;
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
        goto state109;
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
        goto state69;
    case '(': 
        goto state94;
    case '*': 
        goto state99;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state100;
    case '[': 
        goto state101;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state106;
    case ';': 
        goto state107;
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
        goto state59;
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
    case '[': 
        goto state61;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state66;
    case ';': 
        goto state67;
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
        goto state1;
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

state59: // ""\t%B0-0/""\t*
    switch (*ptr++) {
    case '"': 
        goto state24;
    case '%': 
        goto state25;
    case '(': 
        goto state54;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state59;
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
    case '[': 
        goto state61;
    }
    goto state0;

state60: // ""\t%B0-0/""\t*A
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
        goto state60;
    }
    goto state0;

state61: // ""\t%B0-0/""\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state61;
    case 13: 
        goto state62;
    case ';': 
        goto state64;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state62: // ""\t%B0-0/""\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state63;
    }
    goto state0;

state63: // ""\t%B0-0/""\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state61;
    }
    goto state0;

state64: // ""\t%B0-0/""\t*[;
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

state65: // ""\t%B0-0/""\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state63;
    }
    goto state0;

state66: // ""\t%B0-0/""\t0
    switch (*ptr++) {
    case '"': 
        goto state24;
    case '%': 
        goto state25;
    case '(': 
        goto state54;
    case '*': 
        goto state59;
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
    case '[': 
        goto state61;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state66;
    }
    goto state0;

state67: // ""\t%B0-0/""\t;
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

state68: // ""\t%B0-0/""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state23;
    }
    goto state0;

state69: // ""\t%B0-0/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state70;
    case 'D': case 'd': 
        goto state78;
    case 'X': case 'x': 
        goto state86;
    }
    goto state0;

state70: // ""\t%B0-0/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state71;
    }
    goto state0;

state71: // ""\t%B0-0/%B0
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
        goto state71;
    case '-': 
        goto state72;
    case '.': 
        goto state74;
    }
    goto state0;

state72: // ""\t%B0-0/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state73;
    }
    goto state0;

state73: // ""\t%B0-0/%B0-0
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
        goto state73;
    }
    goto state0;

state74: // ""\t%B0-0/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state75;
    }
    goto state0;

state75: // ""\t%B0-0/%B0.0
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
        goto state75;
    case '.': 
        goto state76;
    }
    goto state0;

state76: // ""\t%B0-0/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state77;
    }
    goto state0;

state77: // ""\t%B0-0/%B0.0.0
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
        goto state76;
    case '0': case '1': 
        goto state77;
    }
    goto state0;

state78: // ""\t%B0-0/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state79;
    }
    goto state0;

state79: // ""\t%B0-0/%D0
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
        goto state79;
    case '-': 
        goto state80;
    case '.': 
        goto state82;
    }
    goto state0;

state80: // ""\t%B0-0/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state81;
    }
    goto state0;

state81: // ""\t%B0-0/%D0-0
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
        goto state81;
    }
    goto state0;

state82: // ""\t%B0-0/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state83;
    }
    goto state0;

state83: // ""\t%B0-0/%D0.0
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
        goto state83;
    case '.': 
        goto state84;
    }
    goto state0;

state84: // ""\t%B0-0/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state85;
    }
    goto state0;

state85: // ""\t%B0-0/%D0.0.0
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
        goto state84;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state85;
    }
    goto state0;

state86: // ""\t%B0-0/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state87;
    }
    goto state0;

state87: // ""\t%B0-0/%X0
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
        goto state87;
    case '-': 
        goto state88;
    case '.': 
        goto state90;
    }
    goto state0;

state88: // ""\t%B0-0/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state89;
    }
    goto state0;

state89: // ""\t%B0-0/%X0-0
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
        goto state89;
    }
    goto state0;

state90: // ""\t%B0-0/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state91;
    }
    goto state0;

state91: // ""\t%B0-0/%X0.0
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
        goto state91;
    case '.': 
        goto state92;
    }
    goto state0;

state92: // ""\t%B0-0/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state93;
    }
    goto state0;

state93: // ""\t%B0-0/%X0.0.0
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
        goto state92;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state93;
    }
    goto state0;

state94: // ""\t%B0-0/(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state94;
    case 13: 
        goto state95;
    case ';': 
        goto state97;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state95: // ""\t%B0-0/(\r
    switch (*ptr++) {
    case 10: 
        goto state96;
    }
    goto state0;

state96: // ""\t%B0-0/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state94;
    }
    goto state0;

state97: // ""\t%B0-0/(;
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
        goto state97;
    case 13: 
        goto state98;
    }
    goto state0;

state98: // ""\t%B0-0/(;\r
    switch (*ptr++) {
    case 10: 
        goto state96;
    }
    goto state0;

state99: // ""\t%B0-0/*
    switch (*ptr++) {
    case '"': 
        goto state19;
    case '%': 
        goto state69;
    case '(': 
        goto state94;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state99;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state100;
    case '[': 
        goto state101;
    }
    goto state0;

state100: // ""\t%B0-0/*A
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
        goto state100;
    }
    goto state0;

state101: // ""\t%B0-0/*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state101;
    case 13: 
        goto state102;
    case ';': 
        goto state104;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state102: // ""\t%B0-0/*[\r
    switch (*ptr++) {
    case 10: 
        goto state103;
    }
    goto state0;

state103: // ""\t%B0-0/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state101;
    }
    goto state0;

state104: // ""\t%B0-0/*[;
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
        goto state104;
    case 13: 
        goto state105;
    }
    goto state0;

state105: // ""\t%B0-0/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state103;
    }
    goto state0;

state106: // ""\t%B0-0/0
    switch (*ptr++) {
    case '"': 
        goto state19;
    case '%': 
        goto state69;
    case '(': 
        goto state94;
    case '*': 
        goto state99;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state100;
    case '[': 
        goto state101;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state106;
    }
    goto state0;

state107: // ""\t%B0-0/;
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
        goto state107;
    case 13: 
        goto state108;
    }
    goto state0;

state108: // ""\t%B0-0/;\r
    switch (*ptr++) {
    case 10: 
        goto state18;
    }
    goto state0;

state109: // ""\t%B0-0;
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
        goto state109;
    case 13: 
        goto state110;
    }
    goto state0;

state110: // ""\t%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state13;
    }
    goto state0;

state111: // ""\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state112;
    }
    goto state0;

state112: // ""\t%B0.0
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
        goto state109;
    case '0': case '1': 
        goto state112;
    case '.': 
        goto state113;
    }
    goto state0;

state113: // ""\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state114;
    }
    goto state0;

state114: // ""\t%B0.0.0
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
        goto state109;
    case '.': 
        goto state113;
    case '0': case '1': 
        goto state114;
    }
    goto state0;

state115: // ""\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state116;
    }
    goto state0;

state116: // ""\t%D0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state116;
    case '-': 
        goto state117;
    case '.': 
        goto state119;
    }
    goto state0;

state117: // ""\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state118;
    }
    goto state0;

state118: // ""\t%D0-0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state118;
    }
    goto state0;

state119: // ""\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state120;
    }
    goto state0;

state120: // ""\t%D0.0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state120;
    case '.': 
        goto state121;
    }
    goto state0;

state121: // ""\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state122;
    }
    goto state0;

state122: // ""\t%D0.0.0
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
        goto state109;
    case '.': 
        goto state121;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state122;
    }
    goto state0;

state123: // ""\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state124;
    }
    goto state0;

state124: // ""\t%X0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state124;
    case '-': 
        goto state125;
    case '.': 
        goto state127;
    }
    goto state0;

state125: // ""\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state126;
    }
    goto state0;

state126: // ""\t%X0-0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state126;
    }
    goto state0;

state127: // ""\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state128;
    }
    goto state0;

state128: // ""\t%X0.0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state128;
    case '.': 
        goto state129;
    }
    goto state0;

state129: // ""\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state130;
    }
    goto state0;

state130: // ""\t%X0.0.0
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
        goto state109;
    case '.': 
        goto state129;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state130;
    }
    goto state0;

state131: // ""\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state131;
    case 13: 
        goto state132;
    case ';': 
        goto state134;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state132: // ""\t(\r
    switch (*ptr++) {
    case 10: 
        goto state133;
    }
    goto state0;

state133: // ""\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state131;
    }
    goto state0;

state134: // ""\t(;
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
        goto state134;
    case 13: 
        goto state135;
    }
    goto state0;

state135: // ""\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state133;
    }
    goto state0;

state136: // ""\t*
    switch (*ptr++) {
    case '"': 
        goto state8;
    case '%': 
        goto state9;
    case '(': 
        goto state131;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state136;
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
    case '[': 
        goto state138;
    }
    goto state0;

state137: // ""\t*A
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
        goto state109;
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
        goto state137;
    }
    goto state0;

state138: // ""\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state138;
    case 13: 
        goto state139;
    case ';': 
        goto state141;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state139: // ""\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state140;
    }
    goto state0;

state140: // ""\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state138;
    }
    goto state0;

state141: // ""\t*[;
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
        goto state141;
    case 13: 
        goto state142;
    }
    goto state0;

state142: // ""\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state140;
    }
    goto state0;

state143: // ""\t0
    switch (*ptr++) {
    case '"': 
        goto state8;
    case '%': 
        goto state9;
    case '(': 
        goto state131;
    case '*': 
        goto state136;
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
    case '[': 
        goto state138;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state143;
    }
    goto state0;

state144: // ""\t;
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

state145: // ""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state7;
    }
    goto state0;

state146: // %
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state147;
    case 'D': case 'd': 
        goto state155;
    case 'X': case 'x': 
        goto state163;
    }
    goto state0;

state147: // %B
    switch (*ptr++) {
    case '0': case '1': 
        goto state148;
    }
    goto state0;

state148: // %B0
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
        goto state109;
    case '0': case '1': 
        goto state148;
    case '-': 
        goto state149;
    case '.': 
        goto state151;
    }
    goto state0;

state149: // %B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state150;
    }
    goto state0;

state150: // %B0-0
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
        goto state109;
    case '0': case '1': 
        goto state150;
    }
    goto state0;

state151: // %B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state152;
    }
    goto state0;

state152: // %B0.0
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
        goto state109;
    case '0': case '1': 
        goto state152;
    case '.': 
        goto state153;
    }
    goto state0;

state153: // %B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state154;
    }
    goto state0;

state154: // %B0.0.0
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
        goto state109;
    case '.': 
        goto state153;
    case '0': case '1': 
        goto state154;
    }
    goto state0;

state155: // %D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state156;
    }
    goto state0;

state156: // %D0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state156;
    case '-': 
        goto state157;
    case '.': 
        goto state159;
    }
    goto state0;

state157: // %D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state158;
    }
    goto state0;

state158: // %D0-0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state158;
    }
    goto state0;

state159: // %D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    }
    goto state0;

state160: // %D0.0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    case '.': 
        goto state161;
    }
    goto state0;

state161: // %D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    }
    goto state0;

state162: // %D0.0.0
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
        goto state109;
    case '.': 
        goto state161;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    }
    goto state0;

state163: // %X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state164;
    }
    goto state0;

state164: // %X0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state164;
    case '-': 
        goto state165;
    case '.': 
        goto state167;
    }
    goto state0;

state165: // %X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state166;
    }
    goto state0;

state166: // %X0-0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state166;
    }
    goto state0;

state167: // %X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    }
    goto state0;

state168: // %X0.0
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
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    case '.': 
        goto state169;
    }
    goto state0;

state169: // %X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    }
    goto state0;

state170: // %X0.0.0
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
        goto state109;
    case '.': 
        goto state169;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    }
    goto state0;

state171: // (
    switch (*ptr++) {
    case 9: case ' ': 
        goto state171;
    case 13: 
        goto state172;
    case ';': 
        goto state174;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state172: // (\r
    switch (*ptr++) {
    case 10: 
        goto state173;
    }
    goto state0;

state173: // (\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state171;
    }
    goto state0;

state174: // (;
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
        goto state174;
    case 13: 
        goto state175;
    }
    goto state0;

state175: // (;\r
    switch (*ptr++) {
    case 10: 
        goto state173;
    }
    goto state0;

state176: // *
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state146;
    case '(': 
        goto state171;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state176;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state177;
    case '[': 
        goto state178;
    }
    goto state0;

state177: // *A
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
        goto state109;
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
        goto state177;
    }
    goto state0;

state178: // *[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state178;
    case 13: 
        goto state179;
    case ';': 
        goto state181;
    }
    if (stateAlternation(--ptr)) {
        goto state1;
    }
    goto state0;

state179: // *[\r
    switch (*ptr++) {
    case 10: 
        goto state180;
    }
    goto state0;

state180: // *[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state178;
    }
    goto state0;

state181: // *[;
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
        goto state181;
    case 13: 
        goto state182;
    }
    goto state0;

state182: // *[;\r
    switch (*ptr++) {
    case 10: 
        goto state180;
    }
    goto state0;

state183: // 0
    switch (*ptr++) {
    case '"': 
        goto state3;
    case '%': 
        goto state146;
    case '(': 
        goto state171;
    case '*': 
        goto state176;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state177;
    case '[': 
        goto state178;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state183;
    }
    goto state0;
}

