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

state3: // \t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 13: 
        goto state4;
    case ';': 
        goto state325;
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
        goto state323;
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
        goto state321;
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
        goto state321;
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
    case '"': 
        goto state22;
    case '%': 
        goto state259;
    case '(': 
        goto state284;
    case '*': 
        goto state300;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state301;
    case '[': 
        goto state302;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state318;
    case ';': 
        goto state319;
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

state22: // \t\r\n\t\r\n\r\nA\t/="
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

state23: // \t\r\n\t\r\n\r\nA\t/=""
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    }
    goto state0;

state24: // \t\r\n\t\r\n\r\nA\t/=""\t
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state25;
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case ' ': 
        goto state49;
    case '(': 
        goto state50;
    case '/': 
        goto state65;
    case '*': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state232;
    case ';': 
        goto state233;
    }
    goto state0;

state25: // \t\r\n\t\r\n\r\nA\t/=""\t\r
    switch (*ptr++) {
    case 10: 
        goto state26;
    }
    goto state0;

state26: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n
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
        goto state27;
    }
    goto state0;

state27: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state27;
    case 13: 
        goto state28;
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '(': 
        goto state50;
    case '/': 
        goto state65;
    case '*': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state232;
    case ';': 
        goto state257;
    }
    goto state0;

state28: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state29;
    }
    goto state0;

state29: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t\r\n
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
        goto state27;
    }
    goto state0;

state30: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t"
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
        goto state30;
    }
    goto state0;

state31: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state32;
    case 'D': case 'd': 
        goto state241;
    case 'X': case 'x': 
        goto state249;
    }
    goto state0;

state32: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state33;
    }
    goto state0;

state33: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case '0': case '1': 
        goto state33;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '-': 
        goto state235;
    case '.': 
        goto state237;
    }
    goto state0;

state34: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state35;
    case 13: 
        goto state36;
    }
    goto state0;

state35: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\n
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
        goto state27;
    }
    goto state0;

state36: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r
    switch (*ptr++) {
    case 13: 
        goto state36;
    case 10: 
        goto state37;
    }
    goto state0;

state37: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state38;
    }
    goto state0;

state38: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state38;
    case 13: 
        goto state39;
    case ';': 
        goto state46;
    }
    goto state0;

state39: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state40;
    }
    goto state0;

state40: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n
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
        goto state41;
    }
    goto state0;

state41: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state41;
    case 13: 
        goto state42;
    case ';': 
        goto state44;
    }
    goto state0;

state42: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state43;
    }
    goto state0;

state43: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t\r\n
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
        goto state41;
    }
    goto state0;

state44: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t;
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
        goto state44;
    case 13: 
        goto state45;
    }
    goto state0;

state45: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state43;
    }
    goto state0;

state46: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t;
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
        goto state46;
    case 13: 
        goto state47;
    }
    goto state0;

state47: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state40;
    }
    goto state0;

state48: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s
    switch (*ptr++) {
    case 13: 
        goto state25;
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case ' ': 
        goto state48;
    case 9: 
        goto state49;
    case '(': 
        goto state50;
    case '/': 
        goto state65;
    case '*': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state232;
    case ';': 
        goto state233;
    }
    goto state0;

state49: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t
    switch (*ptr++) {
    case 13: 
        goto state25;
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case 9: case ' ': 
        goto state49;
    case '(': 
        goto state50;
    case '/': 
        goto state65;
    case '*': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state232;
    case ';': 
        goto state233;
    }
    goto state0;

state50: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state50;
    case 13: 
        goto state51;
    case ';': 
        goto state53;
    }
    if (stateAlternation(--ptr)) {
        goto state55;
    }
    goto state0;

state51: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\r
    switch (*ptr++) {
    case 10: 
        goto state52;
    }
    goto state0;

state52: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state50;
    }
    goto state0;

state53: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(;
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
        goto state53;
    case 13: 
        goto state54;
    }
    goto state0;

state54: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state52;
    }
    goto state0;

state55: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*
    switch (*ptr++) {
    case 9: 
        goto state56;
    case 13: 
        goto state62;
    case ' ': 
        goto state63;
    case ')': 
        goto state64;
    case ';': 
        goto state213;
    }
    goto state0;

state56: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 9: 
        goto state56;
    case 13: 
        goto state57;
    case ' ': 
        goto state59;
    case ';': 
        goto state60;
    }
    goto state0;

state57: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state58;
    }
    goto state0;

state58: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state59;
    }
    goto state0;

state59: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 13: 
        goto state57;
    case 9: case ' ': 
        goto state59;
    case ';': 
        goto state60;
    }
    goto state0;

state60: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t;
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
        goto state60;
    case 13: 
        goto state61;
    }
    goto state0;

state61: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state58;
    }
    goto state0;

state62: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state58;
    case 13: 
        goto state62;
    }
    goto state0;

state63: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 13: 
        goto state57;
    case 9: 
        goto state59;
    case ';': 
        goto state60;
    case ' ': 
        goto state63;
    }
    goto state0;

state64: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case ')': 
        goto state64;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    }
    goto state0;

state65: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state65;
    case 13: 
        goto state66;
    case '"': 
        goto state68;
    case '%': 
        goto state148;
    case '(': 
        goto state173;
    case '*': 
        goto state189;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state190;
    case '[': 
        goto state191;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state207;
    case ';': 
        goto state208;
    }
    goto state0;

state66: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/\r
    switch (*ptr++) {
    case 10: 
        goto state67;
    }
    goto state0;

state67: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state65;
    }
    goto state0;

state68: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/"
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
        goto state68;
    case '"': 
        goto state69;
    }
    goto state0;

state69: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    }
    goto state0;

state70: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state71;
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case ' ': 
        goto state83;
    case '(': 
        goto state84;
    case '*': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state121;
    case ';': 
        goto state122;
    }
    goto state0;

state71: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r
    switch (*ptr++) {
    case 10: 
        goto state72;
    }
    goto state0;

state72: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n
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
        goto state73;
    }
    goto state0;

state73: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: case ' ': 
        goto state73;
    case 13: 
        goto state74;
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case '(': 
        goto state84;
    case '*': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state121;
    case ';': 
        goto state146;
    }
    goto state0;

state74: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state75;
    }
    goto state0;

state75: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t\r\n
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
        goto state73;
    }
    goto state0;

state76: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state69;
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
        goto state76;
    }
    goto state0;

state77: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state78;
    case 'D': case 'd': 
        goto state130;
    case 'X': case 'x': 
        goto state138;
    }
    goto state0;

state78: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state79;
    }
    goto state0;

state79: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case '0': case '1': 
        goto state79;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '-': 
        goto state124;
    case '.': 
        goto state126;
    }
    goto state0;

state80: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 13: 
        goto state36;
    case 10: 
        goto state81;
    }
    goto state0;

state81: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\r\n
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
        goto state73;
    }
    goto state0;

state82: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 13: 
        goto state71;
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case ' ': 
        goto state82;
    case 9: 
        goto state83;
    case '(': 
        goto state84;
    case '*': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state121;
    case ';': 
        goto state122;
    }
    goto state0;

state83: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 13: 
        goto state71;
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case 9: case ' ': 
        goto state83;
    case '(': 
        goto state84;
    case '*': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state121;
    case ';': 
        goto state122;
    }
    goto state0;

state84: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state84;
    case 13: 
        goto state85;
    case ';': 
        goto state87;
    }
    if (stateAlternation(--ptr)) {
        goto state89;
    }
    goto state0;

state85: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\r
    switch (*ptr++) {
    case 10: 
        goto state86;
    }
    goto state0;

state86: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state84;
    }
    goto state0;

state87: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(;
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
        goto state87;
    case 13: 
        goto state88;
    }
    goto state0;

state88: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state86;
    }
    goto state0;

state89: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*
    switch (*ptr++) {
    case 9: 
        goto state90;
    case 13: 
        goto state96;
    case ' ': 
        goto state97;
    case ')': 
        goto state98;
    case ';': 
        goto state102;
    }
    goto state0;

state90: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 9: 
        goto state90;
    case 13: 
        goto state91;
    case ' ': 
        goto state93;
    case ';': 
        goto state94;
    }
    goto state0;

state91: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state92;
    }
    goto state0;

state92: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state93;
    }
    goto state0;

state93: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 13: 
        goto state91;
    case 9: case ' ': 
        goto state93;
    case ';': 
        goto state94;
    }
    goto state0;

state94: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t;
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
        goto state94;
    case 13: 
        goto state95;
    }
    goto state0;

state95: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state92;
    }
    goto state0;

state96: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state92;
    case 13: 
        goto state96;
    }
    goto state0;

state97: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 13: 
        goto state91;
    case 9: 
        goto state93;
    case ';': 
        goto state94;
    case ' ': 
        goto state97;
    }
    goto state0;

state98: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*)
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ')': 
        goto state98;
    case ';': 
        goto state99;
    }
    goto state0;

state99: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);
    switch (*ptr++) {
    case ';': 
        goto state99;
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
        goto state100;
    case 13: 
        goto state101;
    }
    goto state0;

state100: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);\t
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
        goto state100;
    case 13: 
        goto state101;
    }
    goto state0;

state101: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);\t\r
    switch (*ptr++) {
    case 10: 
        goto state81;
    }
    goto state0;

state102: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*;
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
        goto state94;
    case 13: 
        goto state95;
    case ';': 
        goto state102;
    }
    goto state0;

state103: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*
    switch (*ptr++) {
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case '(': 
        goto state84;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    }
    goto state0;

state104: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*A
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
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
        goto state104;
    }
    goto state0;

state105: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state105;
    case 13: 
        goto state106;
    case ';': 
        goto state108;
    }
    if (stateAlternation(--ptr)) {
        goto state110;
    }
    goto state0;

state106: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state107;
    }
    goto state0;

state107: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state105;
    }
    goto state0;

state108: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[;
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
        goto state108;
    case 13: 
        goto state109;
    }
    goto state0;

state109: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state107;
    }
    goto state0;

state110: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state111;
    case 13: 
        goto state117;
    case ' ': 
        goto state118;
    case ';': 
        goto state119;
    case ']': 
        goto state120;
    }
    goto state0;

state111: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 9: 
        goto state111;
    case 13: 
        goto state112;
    case ' ': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state112: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state113;
    }
    goto state0;

state113: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state114;
    }
    goto state0;

state114: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 13: 
        goto state112;
    case 9: case ' ': 
        goto state114;
    case ';': 
        goto state115;
    }
    goto state0;

state115: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t;
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
        goto state115;
    case 13: 
        goto state116;
    }
    goto state0;

state116: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state113;
    }
    goto state0;

state117: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state113;
    case 13: 
        goto state117;
    }
    goto state0;

state118: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 13: 
        goto state112;
    case 9: 
        goto state114;
    case ';': 
        goto state115;
    case ' ': 
        goto state118;
    }
    goto state0;

state119: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*;
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
        goto state115;
    case 13: 
        goto state116;
    case ';': 
        goto state119;
    }
    goto state0;

state120: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*]
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case ']': 
        goto state120;
    }
    goto state0;

state121: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t0
    switch (*ptr++) {
    case '"': 
        goto state76;
    case '%': 
        goto state77;
    case '(': 
        goto state84;
    case '*': 
        goto state103;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state104;
    case '[': 
        goto state105;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state121;
    }
    goto state0;

state122: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t;
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
        goto state122;
    case 13: 
        goto state123;
    }
    goto state0;

state123: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t;\r
    switch (*ptr++) {
    case 10: 
        goto state72;
    }
    goto state0;

state124: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state125;
    }
    goto state0;

state125: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': 
        goto state125;
    }
    goto state0;

state126: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state127;
    }
    goto state0;

state127: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': 
        goto state127;
    case '.': 
        goto state128;
    }
    goto state0;

state128: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state129;
    }
    goto state0;

state129: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state128;
    case '0': case '1': 
        goto state129;
    }
    goto state0;

state130: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state131;
    }
    goto state0;

state131: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state131;
    case '-': 
        goto state132;
    case '.': 
        goto state134;
    }
    goto state0;

state132: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state133;
    }
    goto state0;

state133: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state133;
    }
    goto state0;

state134: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state135;
    }
    goto state0;

state135: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state135;
    case '.': 
        goto state136;
    }
    goto state0;

state136: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state137;
    }
    goto state0;

state137: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state136;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state137;
    }
    goto state0;

state138: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state139;
    }
    goto state0;

state139: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state139;
    case '-': 
        goto state140;
    case '.': 
        goto state142;
    }
    goto state0;

state140: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state141;
    }
    goto state0;

state141: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state141;
    }
    goto state0;

state142: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state143;
    }
    goto state0;

state143: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state143;
    case '.': 
        goto state144;
    }
    goto state0;

state144: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state145;
    }
    goto state0;

state145: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state144;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state145;
    }
    goto state0;

state146: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t;
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
        goto state146;
    case 13: 
        goto state147;
    }
    goto state0;

state147: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state75;
    }
    goto state0;

state148: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state149;
    case 'D': case 'd': 
        goto state157;
    case 'X': case 'x': 
        goto state165;
    }
    goto state0;

state149: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state150;
    }
    goto state0;

state150: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': 
        goto state150;
    case '-': 
        goto state151;
    case '.': 
        goto state153;
    }
    goto state0;

state151: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state152;
    }
    goto state0;

state152: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': 
        goto state152;
    }
    goto state0;

state153: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state154;
    }
    goto state0;

state154: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': 
        goto state154;
    case '.': 
        goto state155;
    }
    goto state0;

state155: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state156;
    }
    goto state0;

state156: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state155;
    case '0': case '1': 
        goto state156;
    }
    goto state0;

state157: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state158;
    }
    goto state0;

state158: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state158;
    case '-': 
        goto state159;
    case '.': 
        goto state161;
    }
    goto state0;

state159: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    }
    goto state0;

state160: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state160;
    }
    goto state0;

state161: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    }
    goto state0;

state162: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state162;
    case '.': 
        goto state163;
    }
    goto state0;

state163: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state164;
    }
    goto state0;

state164: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state163;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state164;
    }
    goto state0;

state165: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state166;
    }
    goto state0;

state166: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state166;
    case '-': 
        goto state167;
    case '.': 
        goto state169;
    }
    goto state0;

state167: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    }
    goto state0;

state168: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0-0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state168;
    }
    goto state0;

state169: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    }
    goto state0;

state170: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state170;
    case '.': 
        goto state171;
    }
    goto state0;

state171: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state172;
    }
    goto state0;

state172: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case '.': 
        goto state171;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state172;
    }
    goto state0;

state173: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state173;
    case 13: 
        goto state174;
    case ';': 
        goto state176;
    }
    if (stateAlternation(--ptr)) {
        goto state178;
    }
    goto state0;

state174: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\r
    switch (*ptr++) {
    case 10: 
        goto state175;
    }
    goto state0;

state175: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state173;
    }
    goto state0;

state176: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(;
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
        goto state176;
    case 13: 
        goto state177;
    }
    goto state0;

state177: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(;\r
    switch (*ptr++) {
    case 10: 
        goto state175;
    }
    goto state0;

state178: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*
    switch (*ptr++) {
    case 9: 
        goto state179;
    case 13: 
        goto state185;
    case ' ': 
        goto state186;
    case ')': 
        goto state187;
    case ';': 
        goto state188;
    }
    goto state0;

state179: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 9: 
        goto state179;
    case 13: 
        goto state180;
    case ' ': 
        goto state182;
    case ';': 
        goto state183;
    }
    goto state0;

state180: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state181;
    }
    goto state0;

state181: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state182;
    }
    goto state0;

state182: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 13: 
        goto state180;
    case 9: case ' ': 
        goto state182;
    case ';': 
        goto state183;
    }
    goto state0;

state183: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t;
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
        goto state183;
    case 13: 
        goto state184;
    }
    goto state0;

state184: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state181;
    }
    goto state0;

state185: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\r
    switch (*ptr++) {
    case 10: 
        goto state181;
    case 13: 
        goto state185;
    }
    goto state0;

state186: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*\s
    switch (*ptr++) {
    case ')': 
        goto state69;
    case 13: 
        goto state180;
    case 9: 
        goto state182;
    case ';': 
        goto state183;
    case ' ': 
        goto state186;
    }
    goto state0;

state187: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*)
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case ')': 
        goto state187;
    }
    goto state0;

state188: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/(\*;
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
        goto state183;
    case 13: 
        goto state184;
    case ';': 
        goto state188;
    }
    goto state0;

state189: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*
    switch (*ptr++) {
    case '"': 
        goto state68;
    case '%': 
        goto state148;
    case '(': 
        goto state173;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state189;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state190;
    case '[': 
        goto state191;
    }
    goto state0;

state190: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*A
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
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
        goto state190;
    }
    goto state0;

state191: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state191;
    case 13: 
        goto state192;
    case ';': 
        goto state194;
    }
    if (stateAlternation(--ptr)) {
        goto state196;
    }
    goto state0;

state192: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\r
    switch (*ptr++) {
    case 10: 
        goto state193;
    }
    goto state0;

state193: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state191;
    }
    goto state0;

state194: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[;
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
        goto state194;
    case 13: 
        goto state195;
    }
    goto state0;

state195: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state193;
    }
    goto state0;

state196: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*
    switch (*ptr++) {
    case 9: 
        goto state197;
    case 13: 
        goto state203;
    case ' ': 
        goto state204;
    case ';': 
        goto state205;
    case ']': 
        goto state206;
    }
    goto state0;

state197: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 9: 
        goto state197;
    case 13: 
        goto state198;
    case ' ': 
        goto state200;
    case ';': 
        goto state201;
    }
    goto state0;

state198: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state199;
    }
    goto state0;

state199: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state200;
    }
    goto state0;

state200: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 13: 
        goto state198;
    case 9: case ' ': 
        goto state200;
    case ';': 
        goto state201;
    }
    goto state0;

state201: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t;
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
        goto state201;
    case 13: 
        goto state202;
    }
    goto state0;

state202: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state199;
    }
    goto state0;

state203: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state199;
    case 13: 
        goto state203;
    }
    goto state0;

state204: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state69;
    case 13: 
        goto state198;
    case 9: 
        goto state200;
    case ';': 
        goto state201;
    case ' ': 
        goto state204;
    }
    goto state0;

state205: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*;
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
        goto state201;
    case 13: 
        goto state202;
    case ';': 
        goto state205;
    }
    goto state0;

state206: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/*[\*]
    switch (*ptr++) {
    case '/': 
        goto state65;
    case 9: 
        goto state70;
    case 13: 
        goto state80;
    case ' ': 
        goto state82;
    case ';': 
        goto state99;
    case ']': 
        goto state206;
    }
    goto state0;

state207: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/0
    switch (*ptr++) {
    case '"': 
        goto state68;
    case '%': 
        goto state148;
    case '(': 
        goto state173;
    case '*': 
        goto state189;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state190;
    case '[': 
        goto state191;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state207;
    }
    goto state0;

state208: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/;
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
        goto state208;
    case 13: 
        goto state209;
    }
    goto state0;

state209: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*)/;\r
    switch (*ptr++) {
    case 10: 
        goto state67;
    }
    goto state0;

state210: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*);
    switch (*ptr++) {
    case ';': 
        goto state210;
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
        goto state211;
    case 13: 
        goto state212;
    }
    goto state0;

state211: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*);\t
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
        goto state211;
    case 13: 
        goto state212;
    }
    goto state0;

state212: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*);\t\r
    switch (*ptr++) {
    case 10: 
        goto state35;
    }
    goto state0;

state213: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t(\*;
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
        goto state60;
    case 13: 
        goto state61;
    case ';': 
        goto state213;
    }
    goto state0;

state214: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*
    switch (*ptr++) {
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '(': 
        goto state50;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    }
    goto state0;

state215: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*A
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
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
        goto state215;
    }
    goto state0;

state216: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state216;
    case 13: 
        goto state217;
    case ';': 
        goto state219;
    }
    if (stateAlternation(--ptr)) {
        goto state221;
    }
    goto state0;

state217: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state218;
    }
    goto state0;

state218: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state216;
    }
    goto state0;

state219: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[;
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
        goto state219;
    case 13: 
        goto state220;
    }
    goto state0;

state220: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state218;
    }
    goto state0;

state221: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state222;
    case 13: 
        goto state228;
    case ' ': 
        goto state229;
    case ';': 
        goto state230;
    case ']': 
        goto state231;
    }
    goto state0;

state222: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 9: 
        goto state222;
    case 13: 
        goto state223;
    case ' ': 
        goto state225;
    case ';': 
        goto state226;
    }
    goto state0;

state223: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state224;
    }
    goto state0;

state224: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state225;
    }
    goto state0;

state225: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 13: 
        goto state223;
    case 9: case ' ': 
        goto state225;
    case ';': 
        goto state226;
    }
    goto state0;

state226: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t;
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
        goto state226;
    case 13: 
        goto state227;
    }
    goto state0;

state227: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state224;
    }
    goto state0;

state228: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state224;
    case 13: 
        goto state228;
    }
    goto state0;

state229: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 13: 
        goto state223;
    case 9: 
        goto state225;
    case ';': 
        goto state226;
    case ' ': 
        goto state229;
    }
    goto state0;

state230: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*;
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
        goto state226;
    case 13: 
        goto state227;
    case ';': 
        goto state230;
    }
    goto state0;

state231: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t*[\*]
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case ']': 
        goto state231;
    }
    goto state0;

state232: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t0
    switch (*ptr++) {
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '(': 
        goto state50;
    case '*': 
        goto state214;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state215;
    case '[': 
        goto state216;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state232;
    }
    goto state0;

state233: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t;
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
        goto state233;
    case 13: 
        goto state234;
    }
    goto state0;

state234: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\s\t;\r
    switch (*ptr++) {
    case 10: 
        goto state26;
    }
    goto state0;

state235: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state236;
    }
    goto state0;

state236: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': 
        goto state236;
    }
    goto state0;

state237: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state238;
    }
    goto state0;

state238: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': 
        goto state238;
    case '.': 
        goto state239;
    }
    goto state0;

state239: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state240;
    }
    goto state0;

state240: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state239;
    case '0': case '1': 
        goto state240;
    }
    goto state0;

state241: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state242;
    }
    goto state0;

state242: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state242;
    case '-': 
        goto state243;
    case '.': 
        goto state245;
    }
    goto state0;

state243: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state244;
    }
    goto state0;

state244: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state244;
    }
    goto state0;

state245: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state246;
    }
    goto state0;

state246: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state246;
    case '.': 
        goto state247;
    }
    goto state0;

state247: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state248;
    }
    goto state0;

state248: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state247;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state248;
    }
    goto state0;

state249: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state250;
    }
    goto state0;

state250: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state250;
    case '-': 
        goto state251;
    case '.': 
        goto state253;
    }
    goto state0;

state251: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state252;
    }
    goto state0;

state252: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state252;
    }
    goto state0;

state253: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state254;
    }
    goto state0;

state254: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state254;
    case '.': 
        goto state255;
    }
    goto state0;

state255: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state256;
    }
    goto state0;

state256: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state255;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state256;
    }
    goto state0;

state257: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t;
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
        goto state257;
    case 13: 
        goto state258;
    }
    goto state0;

state258: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state29;
    }
    goto state0;

state259: // \t\r\n\t\r\n\r\nA\t/=%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state260;
    case 'D': case 'd': 
        goto state268;
    case 'X': case 'x': 
        goto state276;
    }
    goto state0;

state260: // \t\r\n\t\r\n\r\nA\t/=%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state261;
    }
    goto state0;

state261: // \t\r\n\t\r\n\r\nA\t/=%B0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': 
        goto state261;
    case '-': 
        goto state262;
    case '.': 
        goto state264;
    }
    goto state0;

state262: // \t\r\n\t\r\n\r\nA\t/=%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state263;
    }
    goto state0;

state263: // \t\r\n\t\r\n\r\nA\t/=%B0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': 
        goto state263;
    }
    goto state0;

state264: // \t\r\n\t\r\n\r\nA\t/=%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state265;
    }
    goto state0;

state265: // \t\r\n\t\r\n\r\nA\t/=%B0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': 
        goto state265;
    case '.': 
        goto state266;
    }
    goto state0;

state266: // \t\r\n\t\r\n\r\nA\t/=%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state267;
    }
    goto state0;

state267: // \t\r\n\t\r\n\r\nA\t/=%B0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state266;
    case '0': case '1': 
        goto state267;
    }
    goto state0;

state268: // \t\r\n\t\r\n\r\nA\t/=%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state269;
    }
    goto state0;

state269: // \t\r\n\t\r\n\r\nA\t/=%D0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state269;
    case '-': 
        goto state270;
    case '.': 
        goto state272;
    }
    goto state0;

state270: // \t\r\n\t\r\n\r\nA\t/=%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state271;
    }
    goto state0;

state271: // \t\r\n\t\r\n\r\nA\t/=%D0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state271;
    }
    goto state0;

state272: // \t\r\n\t\r\n\r\nA\t/=%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state273;
    }
    goto state0;

state273: // \t\r\n\t\r\n\r\nA\t/=%D0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state273;
    case '.': 
        goto state274;
    }
    goto state0;

state274: // \t\r\n\t\r\n\r\nA\t/=%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state275;
    }
    goto state0;

state275: // \t\r\n\t\r\n\r\nA\t/=%D0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state274;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state275;
    }
    goto state0;

state276: // \t\r\n\t\r\n\r\nA\t/=%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state277;
    }
    goto state0;

state277: // \t\r\n\t\r\n\r\nA\t/=%X0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state277;
    case '-': 
        goto state278;
    case '.': 
        goto state280;
    }
    goto state0;

state278: // \t\r\n\t\r\n\r\nA\t/=%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state279;
    }
    goto state0;

state279: // \t\r\n\t\r\n\r\nA\t/=%X0-0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state279;
    }
    goto state0;

state280: // \t\r\n\t\r\n\r\nA\t/=%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state281;
    }
    goto state0;

state281: // \t\r\n\t\r\n\r\nA\t/=%X0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state281;
    case '.': 
        goto state282;
    }
    goto state0;

state282: // \t\r\n\t\r\n\r\nA\t/=%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state283;
    }
    goto state0;

state283: // \t\r\n\t\r\n\r\nA\t/=%X0.0.0
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case '.': 
        goto state282;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state283;
    }
    goto state0;

state284: // \t\r\n\t\r\n\r\nA\t/=(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state284;
    case 13: 
        goto state285;
    case ';': 
        goto state287;
    }
    if (stateAlternation(--ptr)) {
        goto state289;
    }
    goto state0;

state285: // \t\r\n\t\r\n\r\nA\t/=(\r
    switch (*ptr++) {
    case 10: 
        goto state286;
    }
    goto state0;

state286: // \t\r\n\t\r\n\r\nA\t/=(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state284;
    }
    goto state0;

state287: // \t\r\n\t\r\n\r\nA\t/=(;
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
        goto state287;
    case 13: 
        goto state288;
    }
    goto state0;

state288: // \t\r\n\t\r\n\r\nA\t/=(;\r
    switch (*ptr++) {
    case 10: 
        goto state286;
    }
    goto state0;

state289: // \t\r\n\t\r\n\r\nA\t/=(\*
    switch (*ptr++) {
    case 9: 
        goto state290;
    case 13: 
        goto state296;
    case ' ': 
        goto state297;
    case ')': 
        goto state298;
    case ';': 
        goto state299;
    }
    goto state0;

state290: // \t\r\n\t\r\n\r\nA\t/=(\*\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 9: 
        goto state290;
    case 13: 
        goto state291;
    case ' ': 
        goto state293;
    case ';': 
        goto state294;
    }
    goto state0;

state291: // \t\r\n\t\r\n\r\nA\t/=(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state292;
    }
    goto state0;

state292: // \t\r\n\t\r\n\r\nA\t/=(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state293;
    }
    goto state0;

state293: // \t\r\n\t\r\n\r\nA\t/=(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 13: 
        goto state291;
    case 9: case ' ': 
        goto state293;
    case ';': 
        goto state294;
    }
    goto state0;

state294: // \t\r\n\t\r\n\r\nA\t/=(\*\t\r\n\t;
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
        goto state294;
    case 13: 
        goto state295;
    }
    goto state0;

state295: // \t\r\n\t\r\n\r\nA\t/=(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state292;
    }
    goto state0;

state296: // \t\r\n\t\r\n\r\nA\t/=(\*\r
    switch (*ptr++) {
    case 10: 
        goto state292;
    case 13: 
        goto state296;
    }
    goto state0;

state297: // \t\r\n\t\r\n\r\nA\t/=(\*\s
    switch (*ptr++) {
    case ')': 
        goto state23;
    case 13: 
        goto state291;
    case 9: 
        goto state293;
    case ';': 
        goto state294;
    case ' ': 
        goto state297;
    }
    goto state0;

state298: // \t\r\n\t\r\n\r\nA\t/=(\*)
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case ')': 
        goto state298;
    }
    goto state0;

state299: // \t\r\n\t\r\n\r\nA\t/=(\*;
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
        goto state294;
    case 13: 
        goto state295;
    case ';': 
        goto state299;
    }
    goto state0;

state300: // \t\r\n\t\r\n\r\nA\t/=*
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state259;
    case '(': 
        goto state284;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state300;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state301;
    case '[': 
        goto state302;
    }
    goto state0;

state301: // \t\r\n\t\r\n\r\nA\t/=*A
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
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
        goto state301;
    }
    goto state0;

state302: // \t\r\n\t\r\n\r\nA\t/=*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state302;
    case 13: 
        goto state303;
    case ';': 
        goto state305;
    }
    if (stateAlternation(--ptr)) {
        goto state307;
    }
    goto state0;

state303: // \t\r\n\t\r\n\r\nA\t/=*[\r
    switch (*ptr++) {
    case 10: 
        goto state304;
    }
    goto state0;

state304: // \t\r\n\t\r\n\r\nA\t/=*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state302;
    }
    goto state0;

state305: // \t\r\n\t\r\n\r\nA\t/=*[;
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
        goto state305;
    case 13: 
        goto state306;
    }
    goto state0;

state306: // \t\r\n\t\r\n\r\nA\t/=*[;\r
    switch (*ptr++) {
    case 10: 
        goto state304;
    }
    goto state0;

state307: // \t\r\n\t\r\n\r\nA\t/=*[\*
    switch (*ptr++) {
    case 9: 
        goto state308;
    case 13: 
        goto state314;
    case ' ': 
        goto state315;
    case ';': 
        goto state316;
    case ']': 
        goto state317;
    }
    goto state0;

state308: // \t\r\n\t\r\n\r\nA\t/=*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 9: 
        goto state308;
    case 13: 
        goto state309;
    case ' ': 
        goto state311;
    case ';': 
        goto state312;
    }
    goto state0;

state309: // \t\r\n\t\r\n\r\nA\t/=*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state310;
    }
    goto state0;

state310: // \t\r\n\t\r\n\r\nA\t/=*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state311;
    }
    goto state0;

state311: // \t\r\n\t\r\n\r\nA\t/=*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 13: 
        goto state309;
    case 9: case ' ': 
        goto state311;
    case ';': 
        goto state312;
    }
    goto state0;

state312: // \t\r\n\t\r\n\r\nA\t/=*[\*\t\r\n\t;
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
        goto state312;
    case 13: 
        goto state313;
    }
    goto state0;

state313: // \t\r\n\t\r\n\r\nA\t/=*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state310;
    }
    goto state0;

state314: // \t\r\n\t\r\n\r\nA\t/=*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state310;
    case 13: 
        goto state314;
    }
    goto state0;

state315: // \t\r\n\t\r\n\r\nA\t/=*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state23;
    case 13: 
        goto state309;
    case 9: 
        goto state311;
    case ';': 
        goto state312;
    case ' ': 
        goto state315;
    }
    goto state0;

state316: // \t\r\n\t\r\n\r\nA\t/=*[\*;
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
        goto state312;
    case 13: 
        goto state313;
    case ';': 
        goto state316;
    }
    goto state0;

state317: // \t\r\n\t\r\n\r\nA\t/=*[\*]
    switch (*ptr++) {
    case 9: 
        goto state24;
    case 13: 
        goto state34;
    case ' ': 
        goto state48;
    case '/': 
        goto state65;
    case ';': 
        goto state210;
    case ']': 
        goto state317;
    }
    goto state0;

state318: // \t\r\n\t\r\n\r\nA\t/=0
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state259;
    case '(': 
        goto state284;
    case '*': 
        goto state300;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state301;
    case '[': 
        goto state302;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state318;
    }
    goto state0;

state319: // \t\r\n\t\r\n\r\nA\t/=;
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
        goto state319;
    case 13: 
        goto state320;
    }
    goto state0;

state320: // \t\r\n\t\r\n\r\nA\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state21;
    }
    goto state0;

state321: // \t\r\n\t\r\n\r\nA\t;
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
        goto state321;
    case 13: 
        goto state322;
    }
    goto state0;

state322: // \t\r\n\t\r\n\r\nA\t;\r
    switch (*ptr++) {
    case 10: 
        goto state17;
    }
    goto state0;

state323: // \t\r\n\t;
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
        goto state323;
    case 13: 
        goto state324;
    }
    goto state0;

state324: // \t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state8;
    }
    goto state0;

state325: // \t;
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
        goto state325;
    case 13: 
        goto state326;
    }
    goto state0;

state326: // \t;\r
    switch (*ptr++) {
    case 10: 
        goto state5;
    }
    goto state0;

state327: // A
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
        goto state327;
    case 9: case ' ': 
        goto state328;
    case 13: 
        goto state329;
    case '/': 
        goto state331;
    case '=': 
        goto state332;
    case ';': 
        goto state634;
    }
    goto state0;

state328: // A\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state328;
    case 13: 
        goto state329;
    case '/': 
        goto state331;
    case '=': 
        goto state332;
    case ';': 
        goto state634;
    }
    goto state0;

state329: // A\t\r
    switch (*ptr++) {
    case 10: 
        goto state330;
    }
    goto state0;

state330: // A\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state328;
    }
    goto state0;

state331: // A\t/
    switch (*ptr++) {
    case '=': 
        goto state332;
    }
    goto state0;

state332: // A\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state332;
    case 13: 
        goto state333;
    case '"': 
        goto state335;
    case '%': 
        goto state572;
    case '(': 
        goto state597;
    case '*': 
        goto state613;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state614;
    case '[': 
        goto state615;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state631;
    case ';': 
        goto state632;
    }
    goto state0;

state333: // A\t/=\r
    switch (*ptr++) {
    case 10: 
        goto state334;
    }
    goto state0;

state334: // A\t/=\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state332;
    }
    goto state0;

state335: // A\t/="
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
        goto state335;
    case '"': 
        goto state336;
    }
    goto state0;

state336: // A\t/=""
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    }
    goto state0;

state337: // A\t/=""\t
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state338;
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case ' ': 
        goto state362;
    case '(': 
        goto state363;
    case '/': 
        goto state378;
    case '*': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state545;
    case ';': 
        goto state546;
    }
    goto state0;

state338: // A\t/=""\t\r
    switch (*ptr++) {
    case 10: 
        goto state339;
    }
    goto state0;

state339: // A\t/=""\t\r\n
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
        goto state340;
    }
    goto state0;

state340: // A\t/=""\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state340;
    case 13: 
        goto state341;
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case '(': 
        goto state363;
    case '/': 
        goto state378;
    case '*': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state545;
    case ';': 
        goto state570;
    }
    goto state0;

state341: // A\t/=""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state342;
    }
    goto state0;

state342: // A\t/=""\t\r\n\t\r\n
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
        goto state340;
    }
    goto state0;

state343: // A\t/=""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state336;
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
        goto state343;
    }
    goto state0;

state344: // A\t/=""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state345;
    case 'D': case 'd': 
        goto state554;
    case 'X': case 'x': 
        goto state562;
    }
    goto state0;

state345: // A\t/=""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state346;
    }
    goto state0;

state346: // A\t/=""\t\r\n\t%B0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case '0': case '1': 
        goto state346;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '-': 
        goto state548;
    case '.': 
        goto state550;
    }
    goto state0;

state347: // A\t/=""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state348;
    case 13: 
        goto state349;
    }
    goto state0;

state348: // A\t/=""\t\r\n\t%B0\r\n
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
        goto state340;
    }
    goto state0;

state349: // A\t/=""\t\r\n\t%B0\r\r
    switch (*ptr++) {
    case 13: 
        goto state349;
    case 10: 
        goto state350;
    }
    goto state0;

state350: // A\t/=""\t\r\n\t%B0\r\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state351;
    }
    goto state0;

state351: // A\t/=""\t\r\n\t%B0\r\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state351;
    case 13: 
        goto state352;
    case ';': 
        goto state359;
    }
    goto state0;

state352: // A\t/=""\t\r\n\t%B0\r\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state353;
    }
    goto state0;

state353: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n
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
        goto state354;
    }
    goto state0;

state354: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state354;
    case 13: 
        goto state355;
    case ';': 
        goto state357;
    }
    goto state0;

state355: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state356;
    }
    goto state0;

state356: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t\r\n
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
        goto state354;
    }
    goto state0;

state357: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t;
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
        goto state357;
    case 13: 
        goto state358;
    }
    goto state0;

state358: // A\t/=""\t\r\n\t%B0\r\r\n\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state356;
    }
    goto state0;

state359: // A\t/=""\t\r\n\t%B0\r\r\n\t;
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
        goto state359;
    case 13: 
        goto state360;
    }
    goto state0;

state360: // A\t/=""\t\r\n\t%B0\r\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state353;
    }
    goto state0;

state361: // A\t/=""\t\r\n\t%B0\s
    switch (*ptr++) {
    case 13: 
        goto state338;
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case ' ': 
        goto state361;
    case 9: 
        goto state362;
    case '(': 
        goto state363;
    case '/': 
        goto state378;
    case '*': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state545;
    case ';': 
        goto state546;
    }
    goto state0;

state362: // A\t/=""\t\r\n\t%B0\s\t
    switch (*ptr++) {
    case 13: 
        goto state338;
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case 9: case ' ': 
        goto state362;
    case '(': 
        goto state363;
    case '/': 
        goto state378;
    case '*': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state545;
    case ';': 
        goto state546;
    }
    goto state0;

state363: // A\t/=""\t\r\n\t%B0\s\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state363;
    case 13: 
        goto state364;
    case ';': 
        goto state366;
    }
    if (stateAlternation(--ptr)) {
        goto state368;
    }
    goto state0;

state364: // A\t/=""\t\r\n\t%B0\s\t(\r
    switch (*ptr++) {
    case 10: 
        goto state365;
    }
    goto state0;

state365: // A\t/=""\t\r\n\t%B0\s\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state363;
    }
    goto state0;

state366: // A\t/=""\t\r\n\t%B0\s\t(;
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
        goto state366;
    case 13: 
        goto state367;
    }
    goto state0;

state367: // A\t/=""\t\r\n\t%B0\s\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state365;
    }
    goto state0;

state368: // A\t/=""\t\r\n\t%B0\s\t(\*
    switch (*ptr++) {
    case 9: 
        goto state369;
    case 13: 
        goto state375;
    case ' ': 
        goto state376;
    case ')': 
        goto state377;
    case ';': 
        goto state526;
    }
    goto state0;

state369: // A\t/=""\t\r\n\t%B0\s\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 9: 
        goto state369;
    case 13: 
        goto state370;
    case ' ': 
        goto state372;
    case ';': 
        goto state373;
    }
    goto state0;

state370: // A\t/=""\t\r\n\t%B0\s\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state371;
    }
    goto state0;

state371: // A\t/=""\t\r\n\t%B0\s\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state372;
    }
    goto state0;

state372: // A\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 13: 
        goto state370;
    case 9: case ' ': 
        goto state372;
    case ';': 
        goto state373;
    }
    goto state0;

state373: // A\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t;
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
        goto state373;
    case 13: 
        goto state374;
    }
    goto state0;

state374: // A\t/=""\t\r\n\t%B0\s\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state371;
    }
    goto state0;

state375: // A\t/=""\t\r\n\t%B0\s\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state371;
    case 13: 
        goto state375;
    }
    goto state0;

state376: // A\t/=""\t\r\n\t%B0\s\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 13: 
        goto state370;
    case 9: 
        goto state372;
    case ';': 
        goto state373;
    case ' ': 
        goto state376;
    }
    goto state0;

state377: // A\t/=""\t\r\n\t%B0\s\t(\*)
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case ')': 
        goto state377;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    }
    goto state0;

state378: // A\t/=""\t\r\n\t%B0\s\t(\*)/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state378;
    case 13: 
        goto state379;
    case '"': 
        goto state381;
    case '%': 
        goto state461;
    case '(': 
        goto state486;
    case '*': 
        goto state502;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state503;
    case '[': 
        goto state504;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state520;
    case ';': 
        goto state521;
    }
    goto state0;

state379: // A\t/=""\t\r\n\t%B0\s\t(\*)/\r
    switch (*ptr++) {
    case 10: 
        goto state380;
    }
    goto state0;

state380: // A\t/=""\t\r\n\t%B0\s\t(\*)/\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state378;
    }
    goto state0;

state381: // A\t/=""\t\r\n\t%B0\s\t(\*)/"
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
        goto state381;
    case '"': 
        goto state382;
    }
    goto state0;

state382: // A\t/=""\t\r\n\t%B0\s\t(\*)/""
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    }
    goto state0;

state383: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state384;
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case ' ': 
        goto state396;
    case '(': 
        goto state397;
    case '*': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state434;
    case ';': 
        goto state435;
    }
    goto state0;

state384: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r
    switch (*ptr++) {
    case 10: 
        goto state385;
    }
    goto state0;

state385: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n
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
        goto state386;
    }
    goto state0;

state386: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: case ' ': 
        goto state386;
    case 13: 
        goto state387;
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case '(': 
        goto state397;
    case '*': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state434;
    case ';': 
        goto state459;
    }
    goto state0;

state387: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state388;
    }
    goto state0;

state388: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t\r\n
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
        goto state386;
    }
    goto state0;

state389: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state382;
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
        goto state389;
    }
    goto state0;

state390: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state391;
    case 'D': case 'd': 
        goto state443;
    case 'X': case 'x': 
        goto state451;
    }
    goto state0;

state391: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state392;
    }
    goto state0;

state392: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case '0': case '1': 
        goto state392;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '-': 
        goto state437;
    case '.': 
        goto state439;
    }
    goto state0;

state393: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 13: 
        goto state349;
    case 10: 
        goto state394;
    }
    goto state0;

state394: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\r\n
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
        goto state386;
    }
    goto state0;

state395: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 13: 
        goto state384;
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case ' ': 
        goto state395;
    case 9: 
        goto state396;
    case '(': 
        goto state397;
    case '*': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state434;
    case ';': 
        goto state435;
    }
    goto state0;

state396: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 13: 
        goto state384;
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case 9: case ' ': 
        goto state396;
    case '(': 
        goto state397;
    case '*': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state434;
    case ';': 
        goto state435;
    }
    goto state0;

state397: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state397;
    case 13: 
        goto state398;
    case ';': 
        goto state400;
    }
    if (stateAlternation(--ptr)) {
        goto state402;
    }
    goto state0;

state398: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\r
    switch (*ptr++) {
    case 10: 
        goto state399;
    }
    goto state0;

state399: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state397;
    }
    goto state0;

state400: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(;
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
        goto state400;
    case 13: 
        goto state401;
    }
    goto state0;

state401: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state399;
    }
    goto state0;

state402: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*
    switch (*ptr++) {
    case 9: 
        goto state403;
    case 13: 
        goto state409;
    case ' ': 
        goto state410;
    case ')': 
        goto state411;
    case ';': 
        goto state415;
    }
    goto state0;

state403: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 9: 
        goto state403;
    case 13: 
        goto state404;
    case ' ': 
        goto state406;
    case ';': 
        goto state407;
    }
    goto state0;

state404: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state405;
    }
    goto state0;

state405: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state406;
    }
    goto state0;

state406: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 13: 
        goto state404;
    case 9: case ' ': 
        goto state406;
    case ';': 
        goto state407;
    }
    goto state0;

state407: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t;
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
        goto state407;
    case 13: 
        goto state408;
    }
    goto state0;

state408: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state405;
    }
    goto state0;

state409: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\r
    switch (*ptr++) {
    case 10: 
        goto state405;
    case 13: 
        goto state409;
    }
    goto state0;

state410: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*\s
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 13: 
        goto state404;
    case 9: 
        goto state406;
    case ';': 
        goto state407;
    case ' ': 
        goto state410;
    }
    goto state0;

state411: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*)
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ')': 
        goto state411;
    case ';': 
        goto state412;
    }
    goto state0;

state412: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);
    switch (*ptr++) {
    case ';': 
        goto state412;
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
        goto state413;
    case 13: 
        goto state414;
    }
    goto state0;

state413: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);\t
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
        goto state413;
    case 13: 
        goto state414;
    }
    goto state0;

state414: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*);\t\r
    switch (*ptr++) {
    case 10: 
        goto state394;
    }
    goto state0;

state415: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t(\*;
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
        goto state407;
    case 13: 
        goto state408;
    case ';': 
        goto state415;
    }
    goto state0;

state416: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*
    switch (*ptr++) {
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case '(': 
        goto state397;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    }
    goto state0;

state417: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*A
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
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

state418: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state418;
    case 13: 
        goto state419;
    case ';': 
        goto state421;
    }
    if (stateAlternation(--ptr)) {
        goto state423;
    }
    goto state0;

state419: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state420;
    }
    goto state0;

state420: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state418;
    }
    goto state0;

state421: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[;
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
        goto state421;
    case 13: 
        goto state422;
    }
    goto state0;

state422: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state420;
    }
    goto state0;

state423: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state424;
    case 13: 
        goto state430;
    case ' ': 
        goto state431;
    case ';': 
        goto state432;
    case ']': 
        goto state433;
    }
    goto state0;

state424: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 9: 
        goto state424;
    case 13: 
        goto state425;
    case ' ': 
        goto state427;
    case ';': 
        goto state428;
    }
    goto state0;

state425: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state426;
    }
    goto state0;

state426: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state427;
    }
    goto state0;

state427: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 13: 
        goto state425;
    case 9: case ' ': 
        goto state427;
    case ';': 
        goto state428;
    }
    goto state0;

state428: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t;
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
        goto state428;
    case 13: 
        goto state429;
    }
    goto state0;

state429: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state426;
    }
    goto state0;

state430: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state426;
    case 13: 
        goto state430;
    }
    goto state0;

state431: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 13: 
        goto state425;
    case 9: 
        goto state427;
    case ';': 
        goto state428;
    case ' ': 
        goto state431;
    }
    goto state0;

state432: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*;
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
        goto state428;
    case 13: 
        goto state429;
    case ';': 
        goto state432;
    }
    goto state0;

state433: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t*[\*]
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case ']': 
        goto state433;
    }
    goto state0;

state434: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t0
    switch (*ptr++) {
    case '"': 
        goto state389;
    case '%': 
        goto state390;
    case '(': 
        goto state397;
    case '*': 
        goto state416;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state417;
    case '[': 
        goto state418;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state434;
    }
    goto state0;

state435: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t;
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
        goto state435;
    case 13: 
        goto state436;
    }
    goto state0;

state436: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0\s\t;\r
    switch (*ptr++) {
    case 10: 
        goto state385;
    }
    goto state0;

state437: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state438;
    }
    goto state0;

state438: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': 
        goto state438;
    }
    goto state0;

state439: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state440;
    }
    goto state0;

state440: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': 
        goto state440;
    case '.': 
        goto state441;
    }
    goto state0;

state441: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state442;
    }
    goto state0;

state442: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state441;
    case '0': case '1': 
        goto state442;
    }
    goto state0;

state443: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state444;
    }
    goto state0;

state444: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state444;
    case '-': 
        goto state445;
    case '.': 
        goto state447;
    }
    goto state0;

state445: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state446;
    }
    goto state0;

state446: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state446;
    }
    goto state0;

state447: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state448;
    }
    goto state0;

state448: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state448;
    case '.': 
        goto state449;
    }
    goto state0;

state449: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state450;
    }
    goto state0;

state450: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state449;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state450;
    }
    goto state0;

state451: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state452;
    }
    goto state0;

state452: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state452;
    case '-': 
        goto state453;
    case '.': 
        goto state455;
    }
    goto state0;

state453: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state454;
    }
    goto state0;

state454: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state454;
    }
    goto state0;

state455: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state456;
    }
    goto state0;

state456: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state456;
    case '.': 
        goto state457;
    }
    goto state0;

state457: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state458;
    }
    goto state0;

state458: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state457;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state458;
    }
    goto state0;

state459: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t;
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
        goto state459;
    case 13: 
        goto state460;
    }
    goto state0;

state460: // A\t/=""\t\r\n\t%B0\s\t(\*)/""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state388;
    }
    goto state0;

state461: // A\t/=""\t\r\n\t%B0\s\t(\*)/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state462;
    case 'D': case 'd': 
        goto state470;
    case 'X': case 'x': 
        goto state478;
    }
    goto state0;

state462: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state463;
    }
    goto state0;

state463: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': 
        goto state463;
    case '-': 
        goto state464;
    case '.': 
        goto state466;
    }
    goto state0;

state464: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state465;
    }
    goto state0;

state465: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': 
        goto state465;
    }
    goto state0;

state466: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state467;
    }
    goto state0;

state467: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': 
        goto state467;
    case '.': 
        goto state468;
    }
    goto state0;

state468: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state469;
    }
    goto state0;

state469: // A\t/=""\t\r\n\t%B0\s\t(\*)/%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state468;
    case '0': case '1': 
        goto state469;
    }
    goto state0;

state470: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state471;
    }
    goto state0;

state471: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state471;
    case '-': 
        goto state472;
    case '.': 
        goto state474;
    }
    goto state0;

state472: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state473;
    }
    goto state0;

state473: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state473;
    }
    goto state0;

state474: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state475;
    }
    goto state0;

state475: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state475;
    case '.': 
        goto state476;
    }
    goto state0;

state476: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state477;
    }
    goto state0;

state477: // A\t/=""\t\r\n\t%B0\s\t(\*)/%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state476;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state477;
    }
    goto state0;

state478: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state479;
    }
    goto state0;

state479: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state479;
    case '-': 
        goto state480;
    case '.': 
        goto state482;
    }
    goto state0;

state480: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state481;
    }
    goto state0;

state481: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0-0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state481;
    }
    goto state0;

state482: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state483;
    }
    goto state0;

state483: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state483;
    case '.': 
        goto state484;
    }
    goto state0;

state484: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state485;
    }
    goto state0;

state485: // A\t/=""\t\r\n\t%B0\s\t(\*)/%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case '.': 
        goto state484;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state485;
    }
    goto state0;

state486: // A\t/=""\t\r\n\t%B0\s\t(\*)/(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state486;
    case 13: 
        goto state487;
    case ';': 
        goto state489;
    }
    if (stateAlternation(--ptr)) {
        goto state491;
    }
    goto state0;

state487: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\r
    switch (*ptr++) {
    case 10: 
        goto state488;
    }
    goto state0;

state488: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state486;
    }
    goto state0;

state489: // A\t/=""\t\r\n\t%B0\s\t(\*)/(;
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
        goto state489;
    case 13: 
        goto state490;
    }
    goto state0;

state490: // A\t/=""\t\r\n\t%B0\s\t(\*)/(;\r
    switch (*ptr++) {
    case 10: 
        goto state488;
    }
    goto state0;

state491: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*
    switch (*ptr++) {
    case 9: 
        goto state492;
    case 13: 
        goto state498;
    case ' ': 
        goto state499;
    case ')': 
        goto state500;
    case ';': 
        goto state501;
    }
    goto state0;

state492: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 9: 
        goto state492;
    case 13: 
        goto state493;
    case ' ': 
        goto state495;
    case ';': 
        goto state496;
    }
    goto state0;

state493: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state494;
    }
    goto state0;

state494: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state495;
    }
    goto state0;

state495: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 13: 
        goto state493;
    case 9: case ' ': 
        goto state495;
    case ';': 
        goto state496;
    }
    goto state0;

state496: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t;
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
        goto state496;
    case 13: 
        goto state497;
    }
    goto state0;

state497: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state494;
    }
    goto state0;

state498: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\r
    switch (*ptr++) {
    case 10: 
        goto state494;
    case 13: 
        goto state498;
    }
    goto state0;

state499: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*\s
    switch (*ptr++) {
    case ')': 
        goto state382;
    case 13: 
        goto state493;
    case 9: 
        goto state495;
    case ';': 
        goto state496;
    case ' ': 
        goto state499;
    }
    goto state0;

state500: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*)
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case ')': 
        goto state500;
    }
    goto state0;

state501: // A\t/=""\t\r\n\t%B0\s\t(\*)/(\*;
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
        goto state496;
    case 13: 
        goto state497;
    case ';': 
        goto state501;
    }
    goto state0;

state502: // A\t/=""\t\r\n\t%B0\s\t(\*)/*
    switch (*ptr++) {
    case '"': 
        goto state381;
    case '%': 
        goto state461;
    case '(': 
        goto state486;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state502;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state503;
    case '[': 
        goto state504;
    }
    goto state0;

state503: // A\t/=""\t\r\n\t%B0\s\t(\*)/*A
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
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
        goto state503;
    }
    goto state0;

state504: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state504;
    case 13: 
        goto state505;
    case ';': 
        goto state507;
    }
    if (stateAlternation(--ptr)) {
        goto state509;
    }
    goto state0;

state505: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\r
    switch (*ptr++) {
    case 10: 
        goto state506;
    }
    goto state0;

state506: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state504;
    }
    goto state0;

state507: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[;
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
        goto state507;
    case 13: 
        goto state508;
    }
    goto state0;

state508: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state506;
    }
    goto state0;

state509: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*
    switch (*ptr++) {
    case 9: 
        goto state510;
    case 13: 
        goto state516;
    case ' ': 
        goto state517;
    case ';': 
        goto state518;
    case ']': 
        goto state519;
    }
    goto state0;

state510: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 9: 
        goto state510;
    case 13: 
        goto state511;
    case ' ': 
        goto state513;
    case ';': 
        goto state514;
    }
    goto state0;

state511: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state512;
    }
    goto state0;

state512: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state513;
    }
    goto state0;

state513: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 13: 
        goto state511;
    case 9: case ' ': 
        goto state513;
    case ';': 
        goto state514;
    }
    goto state0;

state514: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t;
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
        goto state514;
    case 13: 
        goto state515;
    }
    goto state0;

state515: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state512;
    }
    goto state0;

state516: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state512;
    case 13: 
        goto state516;
    }
    goto state0;

state517: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state382;
    case 13: 
        goto state511;
    case 9: 
        goto state513;
    case ';': 
        goto state514;
    case ' ': 
        goto state517;
    }
    goto state0;

state518: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*;
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
        goto state514;
    case 13: 
        goto state515;
    case ';': 
        goto state518;
    }
    goto state0;

state519: // A\t/=""\t\r\n\t%B0\s\t(\*)/*[\*]
    switch (*ptr++) {
    case '/': 
        goto state378;
    case 9: 
        goto state383;
    case 13: 
        goto state393;
    case ' ': 
        goto state395;
    case ';': 
        goto state412;
    case ']': 
        goto state519;
    }
    goto state0;

state520: // A\t/=""\t\r\n\t%B0\s\t(\*)/0
    switch (*ptr++) {
    case '"': 
        goto state381;
    case '%': 
        goto state461;
    case '(': 
        goto state486;
    case '*': 
        goto state502;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state503;
    case '[': 
        goto state504;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state520;
    }
    goto state0;

state521: // A\t/=""\t\r\n\t%B0\s\t(\*)/;
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
        goto state521;
    case 13: 
        goto state522;
    }
    goto state0;

state522: // A\t/=""\t\r\n\t%B0\s\t(\*)/;\r
    switch (*ptr++) {
    case 10: 
        goto state380;
    }
    goto state0;

state523: // A\t/=""\t\r\n\t%B0\s\t(\*);
    switch (*ptr++) {
    case ';': 
        goto state523;
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
        goto state524;
    case 13: 
        goto state525;
    }
    goto state0;

state524: // A\t/=""\t\r\n\t%B0\s\t(\*);\t
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
        goto state524;
    case 13: 
        goto state525;
    }
    goto state0;

state525: // A\t/=""\t\r\n\t%B0\s\t(\*);\t\r
    switch (*ptr++) {
    case 10: 
        goto state348;
    }
    goto state0;

state526: // A\t/=""\t\r\n\t%B0\s\t(\*;
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
        goto state373;
    case 13: 
        goto state374;
    case ';': 
        goto state526;
    }
    goto state0;

state527: // A\t/=""\t\r\n\t%B0\s\t*
    switch (*ptr++) {
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case '(': 
        goto state363;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    }
    goto state0;

state528: // A\t/=""\t\r\n\t%B0\s\t*A
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
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
        goto state528;
    }
    goto state0;

state529: // A\t/=""\t\r\n\t%B0\s\t*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state529;
    case 13: 
        goto state530;
    case ';': 
        goto state532;
    }
    if (stateAlternation(--ptr)) {
        goto state534;
    }
    goto state0;

state530: // A\t/=""\t\r\n\t%B0\s\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state531;
    }
    goto state0;

state531: // A\t/=""\t\r\n\t%B0\s\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state529;
    }
    goto state0;

state532: // A\t/=""\t\r\n\t%B0\s\t*[;
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
        goto state532;
    case 13: 
        goto state533;
    }
    goto state0;

state533: // A\t/=""\t\r\n\t%B0\s\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state531;
    }
    goto state0;

state534: // A\t/=""\t\r\n\t%B0\s\t*[\*
    switch (*ptr++) {
    case 9: 
        goto state535;
    case 13: 
        goto state541;
    case ' ': 
        goto state542;
    case ';': 
        goto state543;
    case ']': 
        goto state544;
    }
    goto state0;

state535: // A\t/=""\t\r\n\t%B0\s\t*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 9: 
        goto state535;
    case 13: 
        goto state536;
    case ' ': 
        goto state538;
    case ';': 
        goto state539;
    }
    goto state0;

state536: // A\t/=""\t\r\n\t%B0\s\t*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state537;
    }
    goto state0;

state537: // A\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state538;
    }
    goto state0;

state538: // A\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 13: 
        goto state536;
    case 9: case ' ': 
        goto state538;
    case ';': 
        goto state539;
    }
    goto state0;

state539: // A\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t;
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
        goto state539;
    case 13: 
        goto state540;
    }
    goto state0;

state540: // A\t/=""\t\r\n\t%B0\s\t*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state537;
    }
    goto state0;

state541: // A\t/=""\t\r\n\t%B0\s\t*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state537;
    case 13: 
        goto state541;
    }
    goto state0;

state542: // A\t/=""\t\r\n\t%B0\s\t*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 13: 
        goto state536;
    case 9: 
        goto state538;
    case ';': 
        goto state539;
    case ' ': 
        goto state542;
    }
    goto state0;

state543: // A\t/=""\t\r\n\t%B0\s\t*[\*;
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
        goto state539;
    case 13: 
        goto state540;
    case ';': 
        goto state543;
    }
    goto state0;

state544: // A\t/=""\t\r\n\t%B0\s\t*[\*]
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case ']': 
        goto state544;
    }
    goto state0;

state545: // A\t/=""\t\r\n\t%B0\s\t0
    switch (*ptr++) {
    case '"': 
        goto state343;
    case '%': 
        goto state344;
    case '(': 
        goto state363;
    case '*': 
        goto state527;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state528;
    case '[': 
        goto state529;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state545;
    }
    goto state0;

state546: // A\t/=""\t\r\n\t%B0\s\t;
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
        goto state546;
    case 13: 
        goto state547;
    }
    goto state0;

state547: // A\t/=""\t\r\n\t%B0\s\t;\r
    switch (*ptr++) {
    case 10: 
        goto state339;
    }
    goto state0;

state548: // A\t/=""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state549;
    }
    goto state0;

state549: // A\t/=""\t\r\n\t%B0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': 
        goto state549;
    }
    goto state0;

state550: // A\t/=""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state551;
    }
    goto state0;

state551: // A\t/=""\t\r\n\t%B0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': 
        goto state551;
    case '.': 
        goto state552;
    }
    goto state0;

state552: // A\t/=""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state553;
    }
    goto state0;

state553: // A\t/=""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state552;
    case '0': case '1': 
        goto state553;
    }
    goto state0;

state554: // A\t/=""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state555;
    }
    goto state0;

state555: // A\t/=""\t\r\n\t%D0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state555;
    case '-': 
        goto state556;
    case '.': 
        goto state558;
    }
    goto state0;

state556: // A\t/=""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state557;
    }
    goto state0;

state557: // A\t/=""\t\r\n\t%D0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state557;
    }
    goto state0;

state558: // A\t/=""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state559;
    }
    goto state0;

state559: // A\t/=""\t\r\n\t%D0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state559;
    case '.': 
        goto state560;
    }
    goto state0;

state560: // A\t/=""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state561;
    }
    goto state0;

state561: // A\t/=""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state560;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state561;
    }
    goto state0;

state562: // A\t/=""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state563;
    }
    goto state0;

state563: // A\t/=""\t\r\n\t%X0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state563;
    case '-': 
        goto state564;
    case '.': 
        goto state566;
    }
    goto state0;

state564: // A\t/=""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state565;
    }
    goto state0;

state565: // A\t/=""\t\r\n\t%X0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state565;
    }
    goto state0;

state566: // A\t/=""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state567;
    }
    goto state0;

state567: // A\t/=""\t\r\n\t%X0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state567;
    case '.': 
        goto state568;
    }
    goto state0;

state568: // A\t/=""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state569;
    }
    goto state0;

state569: // A\t/=""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state568;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state569;
    }
    goto state0;

state570: // A\t/=""\t\r\n\t;
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
        goto state570;
    case 13: 
        goto state571;
    }
    goto state0;

state571: // A\t/=""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state342;
    }
    goto state0;

state572: // A\t/=%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state573;
    case 'D': case 'd': 
        goto state581;
    case 'X': case 'x': 
        goto state589;
    }
    goto state0;

state573: // A\t/=%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state574;
    }
    goto state0;

state574: // A\t/=%B0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': 
        goto state574;
    case '-': 
        goto state575;
    case '.': 
        goto state577;
    }
    goto state0;

state575: // A\t/=%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state576;
    }
    goto state0;

state576: // A\t/=%B0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': 
        goto state576;
    }
    goto state0;

state577: // A\t/=%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state578;
    }
    goto state0;

state578: // A\t/=%B0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': 
        goto state578;
    case '.': 
        goto state579;
    }
    goto state0;

state579: // A\t/=%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state580;
    }
    goto state0;

state580: // A\t/=%B0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state579;
    case '0': case '1': 
        goto state580;
    }
    goto state0;

state581: // A\t/=%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state582;
    }
    goto state0;

state582: // A\t/=%D0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state582;
    case '-': 
        goto state583;
    case '.': 
        goto state585;
    }
    goto state0;

state583: // A\t/=%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state584;
    }
    goto state0;

state584: // A\t/=%D0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state584;
    }
    goto state0;

state585: // A\t/=%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state586;
    }
    goto state0;

state586: // A\t/=%D0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state586;
    case '.': 
        goto state587;
    }
    goto state0;

state587: // A\t/=%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state588;
    }
    goto state0;

state588: // A\t/=%D0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state587;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state588;
    }
    goto state0;

state589: // A\t/=%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state590;
    }
    goto state0;

state590: // A\t/=%X0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state590;
    case '-': 
        goto state591;
    case '.': 
        goto state593;
    }
    goto state0;

state591: // A\t/=%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state592;
    }
    goto state0;

state592: // A\t/=%X0-0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state592;
    }
    goto state0;

state593: // A\t/=%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state594;
    }
    goto state0;

state594: // A\t/=%X0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state594;
    case '.': 
        goto state595;
    }
    goto state0;

state595: // A\t/=%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state596;
    }
    goto state0;

state596: // A\t/=%X0.0.0
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case '.': 
        goto state595;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': 
        goto state596;
    }
    goto state0;

state597: // A\t/=(
    switch (*ptr++) {
    case 9: case ' ': 
        goto state597;
    case 13: 
        goto state598;
    case ';': 
        goto state600;
    }
    if (stateAlternation(--ptr)) {
        goto state602;
    }
    goto state0;

state598: // A\t/=(\r
    switch (*ptr++) {
    case 10: 
        goto state599;
    }
    goto state0;

state599: // A\t/=(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state597;
    }
    goto state0;

state600: // A\t/=(;
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
        goto state600;
    case 13: 
        goto state601;
    }
    goto state0;

state601: // A\t/=(;\r
    switch (*ptr++) {
    case 10: 
        goto state599;
    }
    goto state0;

state602: // A\t/=(\*
    switch (*ptr++) {
    case 9: 
        goto state603;
    case 13: 
        goto state609;
    case ' ': 
        goto state610;
    case ')': 
        goto state611;
    case ';': 
        goto state612;
    }
    goto state0;

state603: // A\t/=(\*\t
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 9: 
        goto state603;
    case 13: 
        goto state604;
    case ' ': 
        goto state606;
    case ';': 
        goto state607;
    }
    goto state0;

state604: // A\t/=(\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state605;
    }
    goto state0;

state605: // A\t/=(\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state606;
    }
    goto state0;

state606: // A\t/=(\*\t\r\n\t
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 13: 
        goto state604;
    case 9: case ' ': 
        goto state606;
    case ';': 
        goto state607;
    }
    goto state0;

state607: // A\t/=(\*\t\r\n\t;
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
        goto state607;
    case 13: 
        goto state608;
    }
    goto state0;

state608: // A\t/=(\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state605;
    }
    goto state0;

state609: // A\t/=(\*\r
    switch (*ptr++) {
    case 10: 
        goto state605;
    case 13: 
        goto state609;
    }
    goto state0;

state610: // A\t/=(\*\s
    switch (*ptr++) {
    case ')': 
        goto state336;
    case 13: 
        goto state604;
    case 9: 
        goto state606;
    case ';': 
        goto state607;
    case ' ': 
        goto state610;
    }
    goto state0;

state611: // A\t/=(\*)
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case ')': 
        goto state611;
    }
    goto state0;

state612: // A\t/=(\*;
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
        goto state607;
    case 13: 
        goto state608;
    case ';': 
        goto state612;
    }
    goto state0;

state613: // A\t/=*
    switch (*ptr++) {
    case '"': 
        goto state335;
    case '%': 
        goto state572;
    case '(': 
        goto state597;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state613;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state614;
    case '[': 
        goto state615;
    }
    goto state0;

state614: // A\t/=*A
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
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
        goto state614;
    }
    goto state0;

state615: // A\t/=*[
    switch (*ptr++) {
    case 9: case ' ': 
        goto state615;
    case 13: 
        goto state616;
    case ';': 
        goto state618;
    }
    if (stateAlternation(--ptr)) {
        goto state620;
    }
    goto state0;

state616: // A\t/=*[\r
    switch (*ptr++) {
    case 10: 
        goto state617;
    }
    goto state0;

state617: // A\t/=*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state615;
    }
    goto state0;

state618: // A\t/=*[;
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
        goto state618;
    case 13: 
        goto state619;
    }
    goto state0;

state619: // A\t/=*[;\r
    switch (*ptr++) {
    case 10: 
        goto state617;
    }
    goto state0;

state620: // A\t/=*[\*
    switch (*ptr++) {
    case 9: 
        goto state621;
    case 13: 
        goto state627;
    case ' ': 
        goto state628;
    case ';': 
        goto state629;
    case ']': 
        goto state630;
    }
    goto state0;

state621: // A\t/=*[\*\t
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 9: 
        goto state621;
    case 13: 
        goto state622;
    case ' ': 
        goto state624;
    case ';': 
        goto state625;
    }
    goto state0;

state622: // A\t/=*[\*\t\r
    switch (*ptr++) {
    case 10: 
        goto state623;
    }
    goto state0;

state623: // A\t/=*[\*\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state624;
    }
    goto state0;

state624: // A\t/=*[\*\t\r\n\t
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 13: 
        goto state622;
    case 9: case ' ': 
        goto state624;
    case ';': 
        goto state625;
    }
    goto state0;

state625: // A\t/=*[\*\t\r\n\t;
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
        goto state625;
    case 13: 
        goto state626;
    }
    goto state0;

state626: // A\t/=*[\*\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state623;
    }
    goto state0;

state627: // A\t/=*[\*\r
    switch (*ptr++) {
    case 10: 
        goto state623;
    case 13: 
        goto state627;
    }
    goto state0;

state628: // A\t/=*[\*\s
    switch (*ptr++) {
    case ']': 
        goto state336;
    case 13: 
        goto state622;
    case 9: 
        goto state624;
    case ';': 
        goto state625;
    case ' ': 
        goto state628;
    }
    goto state0;

state629: // A\t/=*[\*;
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
        goto state625;
    case 13: 
        goto state626;
    case ';': 
        goto state629;
    }
    goto state0;

state630: // A\t/=*[\*]
    switch (*ptr++) {
    case 9: 
        goto state337;
    case 13: 
        goto state347;
    case ' ': 
        goto state361;
    case '/': 
        goto state378;
    case ';': 
        goto state523;
    case ']': 
        goto state630;
    }
    goto state0;

state631: // A\t/=0
    switch (*ptr++) {
    case '"': 
        goto state335;
    case '%': 
        goto state572;
    case '(': 
        goto state597;
    case '*': 
        goto state613;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v':
    case 'w': case 'x': case 'y': case 'z': 
        goto state614;
    case '[': 
        goto state615;
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
        goto state631;
    }
    goto state0;

state632: // A\t/=;
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
        goto state632;
    case 13: 
        goto state633;
    }
    goto state0;

state633: // A\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state334;
    }
    goto state0;

state634: // A\t;
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
        goto state634;
    case 13: 
        goto state635;
    }
    goto state0;

state635: // A\t;\r
    switch (*ptr++) {
    case 10: 
        goto state330;
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

