// abnfsyntax.cpp - pargen
#include "pch.h"
#pragma hdrstop

//===========================================================================
// Normalized ABNF of syntax being checked, with 'rulelist' as the
// top level rule:
//
// ALPHA = ( %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a )
// BIT = ( %x30 / %x31 )
// CR = %xd
// CRLF = ( CR LF )
// DIGIT = ( %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 )
// DQUOTE = %x22
// HEXDIG = ( DIGIT / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 )
// HTAB = %x9
// LF = %xa
// SP = %x20
// VCHAR = ( %x21 / %x22 / %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e )
// WSP = ( SP / HTAB )
// alternation = ( concatenation *( *c-wsp %x2f *c-wsp concatenation ) )
// bin-val = ( ( %x62 / %x42 ) 1*BIT *1( 1*( %x2e 1*BIT ) / ( %x2d 1*BIT ) ) )
// c-nl = ( comment / CRLF )
// c-wsp = ( WSP / ( c-nl WSP ) )
// char-val = ( DQUOTE *( %x20 / %x21 / %x23 / %x24 / %x25 / %x26 / %x27 / %x28 / %x29 / %x2a / %x2b / %x2c / %x2d / %x2e / %x2f / %x30 / %x31 / %x32 / %x33 / %x34 / %x35 / %x36 / %x37 / %x38 / %x39 / %x3a / %x3b / %x3c / %x3d / %x3e / %x3f / %x40 / %x41 / %x42 / %x43 / %x44 / %x45 / %x46 / %x47 / %x48 / %x49 / %x4a / %x4b / %x4c / %x4d / %x4e / %x4f / %x50 / %x51 / %x52 / %x53 / %x54 / %x55 / %x56 / %x57 / %x58 / %x59 / %x5a / %x5b / %x5c / %x5d / %x5e / %x5f / %x60 / %x61 / %x62 / %x63 / %x64 / %x65 / %x66 / %x67 / %x68 / %x69 / %x6a / %x6b / %x6c / %x6d / %x6e / %x6f / %x70 / %x71 / %x72 / %x73 / %x74 / %x75 / %x76 / %x77 / %x78 / %x79 / %x7a / %x7b / %x7c / %x7d / %x7e ) DQUOTE )
// comment = ( %x3b *( WSP / VCHAR ) CRLF )
// concatenation = ( repetition *( 1*c-wsp repetition ) )
// dec-val = ( ( %x64 / %x44 ) 1*DIGIT *1( 1*( %x2e 1*DIGIT ) / ( %x2d 1*DIGIT ) ) )
// defined-as = ( *c-wsp ( %x3d / ( %x2f %x3d ) ) *c-wsp )
// element = ( rulename / group / option / char-val / num-val )
// elements = ( alternation *c-wsp )
// group = ( %x28 *c-wsp alternation *c-wsp %x29 )
// hex-val = ( ( %x78 / %x58 ) 1*HEXDIG *1( 1*( %x2e 1*HEXDIG ) / ( %x2d 1*HEXDIG ) ) )
// num-val = ( %x25 ( bin-val / dec-val / hex-val ) )
// option = ( %x5b *c-wsp alternation *c-wsp %x5d )
// repeat = ( 1*DIGIT / ( *DIGIT %x2a *DIGIT ) )
// repetition = ( *1repeat element )
// rule = ( rulename defined-as elements c-nl )
// rulelist = 1*( rule / ( *c-wsp c-nl ) )
// rulename = ( ALPHA *( ALPHA / DIGIT / %x2d ) )
bool abnfCheckSyntax (const char src[]) {
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
        goto state219;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state221;
    }
    goto state0;
    
state3: // \t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state3;
    case 13: 
        goto state4;
    case ';': 
        goto state219;
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
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
        goto state217;
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
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
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
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
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
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
        goto state215;
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
        goto state215;
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
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case ';': 
        goto state213;
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
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state22;
    case '"': 
        goto state23;
    }
    goto state0;
    
state23: // \t\r\n\t\r\n\r\nA\t/=""
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    }
    goto state0;
    
state24: // \t\r\n\t\r\n\r\nA\t/=""\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state25;
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '/': 
        goto state38;
    case '(': 
        goto state196;
    case '*': 
        goto state201;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state202;
    case '[': 
        goto state203;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state208;
    case ';': 
        goto state211;
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
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
    case '/': 
        goto state38;
    case '(': 
        goto state196;
    case '*': 
        goto state201;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state202;
    case '[': 
        goto state203;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state208;
    case ';': 
        goto state209;
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state27;
    }
    goto state0;
    
state30: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state23;
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state30;
    }
    goto state0;
    
state31: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state32;
    case 'D': case 'd': 
        goto state180;
    case 'X': case 'x': 
        goto state188;
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
    case 9: case ' ': 
        goto state24;
    case '0': case '1': 
        goto state33;
    case 13: 
        goto state34;
    case '-': 
        goto state36;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state176;
    }
    goto state0;
    
state34: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state35;
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
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state27;
    }
    goto state0;
    
state36: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state37;
    }
    goto state0;
    
state37: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '0': case '1': 
        goto state37;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    }
    goto state0;
    
state38: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state38;
    case 13: 
        goto state39;
    case '"': 
        goto state41;
    case '%': 
        goto state136;
    case '(': 
        goto state161;
    case '*': 
        goto state166;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state167;
    case '[': 
        goto state168;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state173;
    case ';': 
        goto state174;
    }
    goto state0;
    
state39: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/\r
    switch (*ptr++) {
    case 10: 
        goto state40;
    }
    goto state0;
    
state40: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state38;
    }
    goto state0;
    
state41: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/"
    switch (*ptr++) {
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state41;
    case '"': 
        goto state42;
    }
    goto state0;
    
state42: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    }
    goto state0;
    
state43: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state44;
    case '"': 
        goto state49;
    case '%': 
        goto state50;
    case '(': 
        goto state79;
    case '*': 
        goto state124;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state125;
    case '[': 
        goto state126;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state131;
    case ';': 
        goto state134;
    }
    goto state0;
    
state44: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r
    switch (*ptr++) {
    case 10: 
        goto state45;
    }
    goto state0;
    
state45: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state46;
    }
    goto state0;
    
state46: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state46;
    case 13: 
        goto state47;
    case '"': 
        goto state49;
    case '%': 
        goto state50;
    case '(': 
        goto state79;
    case '*': 
        goto state124;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state125;
    case '[': 
        goto state126;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state131;
    case ';': 
        goto state132;
    }
    goto state0;
    
state47: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state48;
    }
    goto state0;
    
state48: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state46;
    }
    goto state0;
    
state49: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state42;
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state49;
    }
    goto state0;
    
state50: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state51;
    case 'D': case 'd': 
        goto state63;
    case 'X': case 'x': 
        goto state71;
    }
    goto state0;
    
state51: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state52;
    }
    goto state0;
    
state52: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case '0': case '1': 
        goto state52;
    case 13: 
        goto state53;
    case '-': 
        goto state55;
    case ';': 
        goto state57;
    case '.': 
        goto state59;
    }
    goto state0;
    
state53: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state54;
    }
    goto state0;
    
state54: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state46;
    }
    goto state0;
    
state55: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state56;
    }
    goto state0;
    
state56: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case '0': case '1': 
        goto state56;
    case ';': 
        goto state57;
    }
    goto state0;
    
state57: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state57;
    case 13: 
        goto state58;
    }
    goto state0;
    
state58: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state54;
    }
    goto state0;
    
state59: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state60;
    }
    goto state0;
    
state60: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': 
        goto state60;
    case '.': 
        goto state61;
    }
    goto state0;
    
state61: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state62;
    }
    goto state0;
    
state62: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state61;
    case '0': case '1': 
        goto state62;
    }
    goto state0;
    
state63: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state64;
    }
    goto state0;
    
state64: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state64;
    case '-': 
        goto state65;
    case '.': 
        goto state67;
    }
    goto state0;
    
state65: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state66;
    }
    goto state0;
    
state66: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state66;
    }
    goto state0;
    
state67: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state68;
    }
    goto state0;
    
state68: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state68;
    case '.': 
        goto state69;
    }
    goto state0;
    
state69: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state70;
    }
    goto state0;
    
state70: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state69;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state70;
    }
    goto state0;
    
state71: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state72;
    }
    goto state0;
    
state72: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state72;
    case '-': 
        goto state73;
    case '.': 
        goto state75;
    }
    goto state0;
    
state73: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state74;
    }
    goto state0;
    
state74: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state74;
    }
    goto state0;
    
state75: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state76;
    }
    goto state0;
    
state76: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state76;
    case '.': 
        goto state77;
    }
    goto state0;
    
state77: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state78;
    }
    goto state0;
    
state78: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state77;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state78;
    }
    goto state0;
    
state79: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(
    switch (*ptr++) {
    case '"': 
        goto state22;
    case 9: case ' ': 
        goto state79;
    case 13: 
        goto state80;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case ';': 
        goto state122;
    }
    goto state0;
    
state80: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(\r
    switch (*ptr++) {
    case 10: 
        goto state81;
    }
    goto state0;
    
state81: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state79;
    }
    goto state0;
    
state82: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state83;
    case 'D': case 'd': 
        goto state93;
    case 'X': case 'x': 
        goto state101;
    }
    goto state0;
    
state83: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state84;
    }
    goto state0;
    
state84: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case '0': case '1': 
        goto state84;
    case '-': 
        goto state85;
    case ';': 
        goto state87;
    case '.': 
        goto state89;
    }
    goto state0;
    
state85: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state86;
    }
    goto state0;
    
state86: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case '0': case '1': 
        goto state86;
    case ';': 
        goto state87;
    }
    goto state0;
    
state87: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state87;
    case 13: 
        goto state88;
    }
    goto state0;
    
state88: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state35;
    }
    goto state0;
    
state89: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state90;
    }
    goto state0;
    
state90: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': 
        goto state90;
    case '.': 
        goto state91;
    }
    goto state0;
    
state91: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state92;
    }
    goto state0;
    
state92: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state91;
    case '0': case '1': 
        goto state92;
    }
    goto state0;
    
state93: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state94;
    }
    goto state0;
    
state94: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state94;
    case '-': 
        goto state95;
    case '.': 
        goto state97;
    }
    goto state0;
    
state95: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state96;
    }
    goto state0;
    
state96: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state96;
    }
    goto state0;
    
state97: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state98;
    }
    goto state0;
    
state98: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state98;
    case '.': 
        goto state99;
    }
    goto state0;
    
state99: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state100;
    }
    goto state0;
    
state100: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state99;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state100;
    }
    goto state0;
    
state101: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state102;
    }
    goto state0;
    
state102: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state102;
    case '-': 
        goto state103;
    case '.': 
        goto state105;
    }
    goto state0;
    
state103: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state104;
    }
    goto state0;
    
state104: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state104;
    }
    goto state0;
    
state105: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state106;
    }
    goto state0;
    
state106: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state106;
    case '.': 
        goto state107;
    }
    goto state0;
    
state107: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state108;
    }
    goto state0;
    
state108: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state107;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state108;
    }
    goto state0;
    
state109: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case 9: case ' ': case '(': 
        goto state109;
    case 13: 
        goto state110;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case ';': 
        goto state120;
    }
    goto state0;
    
state110: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((\r
    switch (*ptr++) {
    case 10: 
        goto state111;
    }
    goto state0;
    
state111: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state109;
    }
    goto state0;
    
state112: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    }
    goto state0;
    
state113: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*A
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state113;
    }
    goto state0;
    
state114: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case 9: case ' ': case '[': 
        goto state114;
    case 13: 
        goto state115;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case ';': 
        goto state118;
    }
    goto state0;
    
state115: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[\r
    switch (*ptr++) {
    case 10: 
        goto state116;
    }
    goto state0;
    
state116: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state114;
    }
    goto state0;
    
state117: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[0
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    }
    goto state0;
    
state118: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state118;
    case 13: 
        goto state119;
    }
    goto state0;
    
state119: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[;\r
    switch (*ptr++) {
    case 10: 
        goto state116;
    }
    goto state0;
    
state120: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state120;
    case 13: 
        goto state121;
    }
    goto state0;
    
state121: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t((;\r
    switch (*ptr++) {
    case 10: 
        goto state111;
    }
    goto state0;
    
state122: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state122;
    case 13: 
        goto state123;
    }
    goto state0;
    
state123: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state81;
    }
    goto state0;
    
state124: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*
    switch (*ptr++) {
    case '"': 
        goto state49;
    case '%': 
        goto state50;
    case '(': 
        goto state79;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state124;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state125;
    case '[': 
        goto state126;
    }
    goto state0;
    
state125: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*A
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state125;
    }
    goto state0;
    
state126: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case 9: case ' ': 
        goto state126;
    case 13: 
        goto state127;
    case ';': 
        goto state129;
    }
    goto state0;
    
state127: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state128;
    }
    goto state0;
    
state128: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state126;
    }
    goto state0;
    
state129: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state129;
    case 13: 
        goto state130;
    }
    goto state0;
    
state130: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state128;
    }
    goto state0;
    
state131: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t0
    switch (*ptr++) {
    case '"': 
        goto state49;
    case '%': 
        goto state50;
    case '(': 
        goto state79;
    case '*': 
        goto state124;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state125;
    case '[': 
        goto state126;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state131;
    }
    goto state0;
    
state132: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state132;
    case 13: 
        goto state133;
    }
    goto state0;
    
state133: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state48;
    }
    goto state0;
    
state134: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state134;
    case 13: 
        goto state135;
    }
    goto state0;
    
state135: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state45;
    }
    goto state0;
    
state136: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state137;
    case 'D': case 'd': 
        goto state145;
    case 'X': case 'x': 
        goto state153;
    }
    goto state0;
    
state137: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state138;
    }
    goto state0;
    
state138: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': 
        goto state138;
    case '-': 
        goto state139;
    case '.': 
        goto state141;
    }
    goto state0;
    
state139: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state140;
    }
    goto state0;
    
state140: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': 
        goto state140;
    }
    goto state0;
    
state141: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state142;
    }
    goto state0;
    
state142: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': 
        goto state142;
    case '.': 
        goto state143;
    }
    goto state0;
    
state143: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state144;
    }
    goto state0;
    
state144: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state143;
    case '0': case '1': 
        goto state144;
    }
    goto state0;
    
state145: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state146;
    }
    goto state0;
    
state146: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state146;
    case '-': 
        goto state147;
    case '.': 
        goto state149;
    }
    goto state0;
    
state147: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state148;
    }
    goto state0;
    
state148: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state148;
    }
    goto state0;
    
state149: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state150;
    }
    goto state0;
    
state150: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state150;
    case '.': 
        goto state151;
    }
    goto state0;
    
state151: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state152;
    }
    goto state0;
    
state152: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state151;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state152;
    }
    goto state0;
    
state153: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state154;
    }
    goto state0;
    
state154: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state154;
    case '-': 
        goto state155;
    case '.': 
        goto state157;
    }
    goto state0;
    
state155: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state156;
    }
    goto state0;
    
state156: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0-0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state156;
    }
    goto state0;
    
state157: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state158;
    }
    goto state0;
    
state158: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state158;
    case '.': 
        goto state159;
    }
    goto state0;
    
state159: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state160;
    }
    goto state0;
    
state160: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '.': 
        goto state159;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state160;
    }
    goto state0;
    
state161: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/(
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case 9: case ' ': 
        goto state161;
    case 13: 
        goto state162;
    case ';': 
        goto state164;
    }
    goto state0;
    
state162: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/(\r
    switch (*ptr++) {
    case 10: 
        goto state163;
    }
    goto state0;
    
state163: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state161;
    }
    goto state0;
    
state164: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state164;
    case 13: 
        goto state165;
    }
    goto state0;
    
state165: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/(;\r
    switch (*ptr++) {
    case 10: 
        goto state163;
    }
    goto state0;
    
state166: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*
    switch (*ptr++) {
    case '"': 
        goto state41;
    case '%': 
        goto state136;
    case '(': 
        goto state161;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state166;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state167;
    case '[': 
        goto state168;
    }
    goto state0;
    
state167: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*A
    switch (*ptr++) {
    case '/': 
        goto state38;
    case 9: case ' ': 
        goto state43;
    case 13: 
        goto state53;
    case ';': 
        goto state57;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state167;
    }
    goto state0;
    
state168: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*[
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case 9: case ' ': 
        goto state168;
    case 13: 
        goto state169;
    case ';': 
        goto state171;
    }
    goto state0;
    
state169: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*[\r
    switch (*ptr++) {
    case 10: 
        goto state170;
    }
    goto state0;
    
state170: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state168;
    }
    goto state0;
    
state171: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state171;
    case 13: 
        goto state172;
    }
    goto state0;
    
state172: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state170;
    }
    goto state0;
    
state173: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/0
    switch (*ptr++) {
    case '"': 
        goto state41;
    case '%': 
        goto state136;
    case '(': 
        goto state161;
    case '*': 
        goto state166;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state167;
    case '[': 
        goto state168;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state173;
    }
    goto state0;
    
state174: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state174;
    case 13: 
        goto state175;
    }
    goto state0;
    
state175: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0-0/;\r
    switch (*ptr++) {
    case 10: 
        goto state40;
    }
    goto state0;
    
state176: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state177;
    }
    goto state0;
    
state177: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': 
        goto state177;
    case '.': 
        goto state178;
    }
    goto state0;
    
state178: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state179;
    }
    goto state0;
    
state179: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state178;
    case '0': case '1': 
        goto state179;
    }
    goto state0;
    
state180: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state181;
    }
    goto state0;
    
state181: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state181;
    case '-': 
        goto state182;
    case '.': 
        goto state184;
    }
    goto state0;
    
state182: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state183;
    }
    goto state0;
    
state183: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state183;
    }
    goto state0;
    
state184: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state185;
    }
    goto state0;
    
state185: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state185;
    case '.': 
        goto state186;
    }
    goto state0;
    
state186: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state187;
    }
    goto state0;
    
state187: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state186;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state187;
    }
    goto state0;
    
state188: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state189;
    }
    goto state0;
    
state189: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state189;
    case '-': 
        goto state190;
    case '.': 
        goto state192;
    }
    goto state0;
    
state190: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state191;
    }
    goto state0;
    
state191: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state191;
    }
    goto state0;
    
state192: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state193;
    }
    goto state0;
    
state193: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state193;
    case '.': 
        goto state194;
    }
    goto state0;
    
state194: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state195;
    }
    goto state0;
    
state195: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '.': 
        goto state194;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state195;
    }
    goto state0;
    
state196: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t(
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case 9: case ' ': 
        goto state196;
    case 13: 
        goto state197;
    case ';': 
        goto state199;
    }
    goto state0;
    
state197: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t(\r
    switch (*ptr++) {
    case 10: 
        goto state198;
    }
    goto state0;
    
state198: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state196;
    }
    goto state0;
    
state199: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state199;
    case 13: 
        goto state200;
    }
    goto state0;
    
state200: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state198;
    }
    goto state0;
    
state201: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*
    switch (*ptr++) {
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '(': 
        goto state196;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state201;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state202;
    case '[': 
        goto state203;
    }
    goto state0;
    
state202: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*A
    switch (*ptr++) {
    case 9: case ' ': 
        goto state24;
    case 13: 
        goto state34;
    case '/': 
        goto state38;
    case ';': 
        goto state87;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state202;
    }
    goto state0;
    
state203: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*[
    switch (*ptr++) {
    case '"': 
        goto state22;
    case '%': 
        goto state82;
    case '(': 
        goto state109;
    case '*': 
        goto state112;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state113;
    case '[': 
        goto state114;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state117;
    case 9: case ' ': 
        goto state203;
    case 13: 
        goto state204;
    case ';': 
        goto state206;
    }
    goto state0;
    
state204: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state205;
    }
    goto state0;
    
state205: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state203;
    }
    goto state0;
    
state206: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state206;
    case 13: 
        goto state207;
    }
    goto state0;
    
state207: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state205;
    }
    goto state0;
    
state208: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t0
    switch (*ptr++) {
    case '"': 
        goto state30;
    case '%': 
        goto state31;
    case '(': 
        goto state196;
    case '*': 
        goto state201;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state202;
    case '[': 
        goto state203;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state208;
    }
    goto state0;
    
state209: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state209;
    case 13: 
        goto state210;
    }
    goto state0;
    
state210: // \t\r\n\t\r\n\r\nA\t/=""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state29;
    }
    goto state0;
    
state211: // \t\r\n\t\r\n\r\nA\t/=""\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state211;
    case 13: 
        goto state212;
    }
    goto state0;
    
state212: // \t\r\n\t\r\n\r\nA\t/=""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state26;
    }
    goto state0;
    
state213: // \t\r\n\t\r\n\r\nA\t/=;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state213;
    case 13: 
        goto state214;
    }
    goto state0;
    
state214: // \t\r\n\t\r\n\r\nA\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state21;
    }
    goto state0;
    
state215: // \t\r\n\t\r\n\r\nA\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state215;
    case 13: 
        goto state216;
    }
    goto state0;
    
state216: // \t\r\n\t\r\n\r\nA\t;\r
    switch (*ptr++) {
    case 10: 
        goto state17;
    }
    goto state0;
    
state217: // \t\r\n\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state217;
    case 13: 
        goto state218;
    }
    goto state0;
    
state218: // \t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state8;
    }
    goto state0;
    
state219: // \t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state219;
    case 13: 
        goto state220;
    }
    goto state0;
    
state220: // \t;\r
    switch (*ptr++) {
    case 10: 
        goto state5;
    }
    goto state0;
    
state221: // A
    switch (*ptr++) {
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state221;
    case 9: case ' ': 
        goto state222;
    case 13: 
        goto state223;
    case '/': 
        goto state225;
    case '=': 
        goto state226;
    case ';': 
        goto state422;
    }
    goto state0;
    
state222: // A\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state222;
    case 13: 
        goto state223;
    case '/': 
        goto state225;
    case '=': 
        goto state226;
    case ';': 
        goto state422;
    }
    goto state0;
    
state223: // A\t\r
    switch (*ptr++) {
    case 10: 
        goto state224;
    }
    goto state0;
    
state224: // A\t\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state222;
    }
    goto state0;
    
state225: // A\t/
    switch (*ptr++) {
    case '=': 
        goto state226;
    }
    goto state0;
    
state226: // A\t/=
    switch (*ptr++) {
    case 9: case ' ': 
        goto state226;
    case 13: 
        goto state227;
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case ';': 
        goto state420;
    }
    goto state0;
    
state227: // A\t/=\r
    switch (*ptr++) {
    case 10: 
        goto state228;
    }
    goto state0;
    
state228: // A\t/=\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state226;
    }
    goto state0;
    
state229: // A\t/="
    switch (*ptr++) {
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state229;
    case '"': 
        goto state230;
    }
    goto state0;
    
state230: // A\t/=""
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    }
    goto state0;
    
state231: // A\t/=""\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state232;
    case '"': 
        goto state237;
    case '%': 
        goto state238;
    case '/': 
        goto state245;
    case '(': 
        goto state403;
    case '*': 
        goto state408;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state409;
    case '[': 
        goto state410;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state415;
    case ';': 
        goto state418;
    }
    goto state0;
    
state232: // A\t/=""\t\r
    switch (*ptr++) {
    case 10: 
        goto state233;
    }
    goto state0;
    
state233: // A\t/=""\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state234;
    }
    goto state0;
    
state234: // A\t/=""\t\r\n\t
    switch (*ptr++) {
    case 9: case ' ': 
        goto state234;
    case 13: 
        goto state235;
    case '"': 
        goto state237;
    case '%': 
        goto state238;
    case '/': 
        goto state245;
    case '(': 
        goto state403;
    case '*': 
        goto state408;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state409;
    case '[': 
        goto state410;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state415;
    case ';': 
        goto state416;
    }
    goto state0;
    
state235: // A\t/=""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state236;
    }
    goto state0;
    
state236: // A\t/=""\t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state234;
    }
    goto state0;
    
state237: // A\t/=""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state230;
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state237;
    }
    goto state0;
    
state238: // A\t/=""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state239;
    case 'D': case 'd': 
        goto state387;
    case 'X': case 'x': 
        goto state395;
    }
    goto state0;
    
state239: // A\t/=""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state240;
    }
    goto state0;
    
state240: // A\t/=""\t\r\n\t%B0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case '0': case '1': 
        goto state240;
    case 13: 
        goto state241;
    case '-': 
        goto state243;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state383;
    }
    goto state0;
    
state241: // A\t/=""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state242;
    }
    goto state0;
    
state242: // A\t/=""\t\r\n\t%B0\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state234;
    }
    goto state0;
    
state243: // A\t/=""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state244;
    }
    goto state0;
    
state244: // A\t/=""\t\r\n\t%B0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '0': case '1': 
        goto state244;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    }
    goto state0;
    
state245: // A\t/=""\t\r\n\t%B0-0/
    switch (*ptr++) {
    case 9: case ' ': 
        goto state245;
    case 13: 
        goto state246;
    case '"': 
        goto state248;
    case '%': 
        goto state343;
    case '(': 
        goto state368;
    case '*': 
        goto state373;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state374;
    case '[': 
        goto state375;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state380;
    case ';': 
        goto state381;
    }
    goto state0;
    
state246: // A\t/=""\t\r\n\t%B0-0/\r
    switch (*ptr++) {
    case 10: 
        goto state247;
    }
    goto state0;
    
state247: // A\t/=""\t\r\n\t%B0-0/\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state245;
    }
    goto state0;
    
state248: // A\t/=""\t\r\n\t%B0-0/"
    switch (*ptr++) {
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state248;
    case '"': 
        goto state249;
    }
    goto state0;
    
state249: // A\t/=""\t\r\n\t%B0-0/""
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    }
    goto state0;
    
state250: // A\t/=""\t\r\n\t%B0-0/""\t
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state251;
    case '"': 
        goto state256;
    case '%': 
        goto state257;
    case '(': 
        goto state286;
    case '*': 
        goto state331;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state332;
    case '[': 
        goto state333;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state338;
    case ';': 
        goto state341;
    }
    goto state0;
    
state251: // A\t/=""\t\r\n\t%B0-0/""\t\r
    switch (*ptr++) {
    case 10: 
        goto state252;
    }
    goto state0;
    
state252: // A\t/=""\t\r\n\t%B0-0/""\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state253;
    }
    goto state0;
    
state253: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state253;
    case 13: 
        goto state254;
    case '"': 
        goto state256;
    case '%': 
        goto state257;
    case '(': 
        goto state286;
    case '*': 
        goto state331;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state332;
    case '[': 
        goto state333;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state338;
    case ';': 
        goto state339;
    }
    goto state0;
    
state254: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t\r
    switch (*ptr++) {
    case 10: 
        goto state255;
    }
    goto state0;
    
state255: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state253;
    }
    goto state0;
    
state256: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t"
    switch (*ptr++) {
    case '"': 
        goto state249;
    case ' ': case '!': case '#': case '$': case '%': case '&': case '\'':
    case '(': case ')': case '*': case '+': case ',': case '-': case '.':
    case '/': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case ':': case ';': case '<':
    case '=': case '>': case '?': case '@': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case '[': case '\\': case ']': case '^': case '_':
    case '`': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '{':
    case '|': case '}': case '~': 
        goto state256;
    }
    goto state0;
    
state257: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state258;
    case 'D': case 'd': 
        goto state270;
    case 'X': case 'x': 
        goto state278;
    }
    goto state0;
    
state258: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state259;
    }
    goto state0;
    
state259: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case '0': case '1': 
        goto state259;
    case 13: 
        goto state260;
    case '-': 
        goto state262;
    case ';': 
        goto state264;
    case '.': 
        goto state266;
    }
    goto state0;
    
state260: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0\r
    switch (*ptr++) {
    case 10: 
        goto state261;
    }
    goto state0;
    
state261: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0\r\n
    switch (*ptr++) {
    case 0: 
        goto state1;
    case 13: 
        goto state9;
    case ';': 
        goto state12;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state14;
    case 9: case ' ': 
        goto state253;
    }
    goto state0;
    
state262: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state263;
    }
    goto state0;
    
state263: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case '0': case '1': 
        goto state263;
    case ';': 
        goto state264;
    }
    goto state0;
    
state264: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state264;
    case 13: 
        goto state265;
    }
    goto state0;
    
state265: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state261;
    }
    goto state0;
    
state266: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state267;
    }
    goto state0;
    
state267: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': 
        goto state267;
    case '.': 
        goto state268;
    }
    goto state0;
    
state268: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state269;
    }
    goto state0;
    
state269: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state268;
    case '0': case '1': 
        goto state269;
    }
    goto state0;
    
state270: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state271;
    }
    goto state0;
    
state271: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state271;
    case '-': 
        goto state272;
    case '.': 
        goto state274;
    }
    goto state0;
    
state272: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state273;
    }
    goto state0;
    
state273: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state273;
    }
    goto state0;
    
state274: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state275;
    }
    goto state0;
    
state275: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state275;
    case '.': 
        goto state276;
    }
    goto state0;
    
state276: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state277;
    }
    goto state0;
    
state277: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state276;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state277;
    }
    goto state0;
    
state278: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state279;
    }
    goto state0;
    
state279: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state279;
    case '-': 
        goto state280;
    case '.': 
        goto state282;
    }
    goto state0;
    
state280: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state281;
    }
    goto state0;
    
state281: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state281;
    }
    goto state0;
    
state282: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state283;
    }
    goto state0;
    
state283: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state283;
    case '.': 
        goto state284;
    }
    goto state0;
    
state284: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state285;
    }
    goto state0;
    
state285: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state284;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state285;
    }
    goto state0;
    
state286: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(
    switch (*ptr++) {
    case '"': 
        goto state229;
    case 9: case ' ': 
        goto state286;
    case 13: 
        goto state287;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case ';': 
        goto state329;
    }
    goto state0;
    
state287: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(\r
    switch (*ptr++) {
    case 10: 
        goto state288;
    }
    goto state0;
    
state288: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state286;
    }
    goto state0;
    
state289: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state290;
    case 'D': case 'd': 
        goto state300;
    case 'X': case 'x': 
        goto state308;
    }
    goto state0;
    
state290: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state291;
    }
    goto state0;
    
state291: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case '0': case '1': 
        goto state291;
    case '-': 
        goto state292;
    case ';': 
        goto state294;
    case '.': 
        goto state296;
    }
    goto state0;
    
state292: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state293;
    }
    goto state0;
    
state293: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case '0': case '1': 
        goto state293;
    case ';': 
        goto state294;
    }
    goto state0;
    
state294: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state294;
    case 13: 
        goto state295;
    }
    goto state0;
    
state295: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0-0;\r
    switch (*ptr++) {
    case 10: 
        goto state242;
    }
    goto state0;
    
state296: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state297;
    }
    goto state0;
    
state297: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': 
        goto state297;
    case '.': 
        goto state298;
    }
    goto state0;
    
state298: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state299;
    }
    goto state0;
    
state299: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%B0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state298;
    case '0': case '1': 
        goto state299;
    }
    goto state0;
    
state300: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state301;
    }
    goto state0;
    
state301: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state301;
    case '-': 
        goto state302;
    case '.': 
        goto state304;
    }
    goto state0;
    
state302: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state303;
    }
    goto state0;
    
state303: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state303;
    }
    goto state0;
    
state304: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state305;
    }
    goto state0;
    
state305: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state305;
    case '.': 
        goto state306;
    }
    goto state0;
    
state306: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state307;
    }
    goto state0;
    
state307: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%D0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state306;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state307;
    }
    goto state0;
    
state308: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state309;
    }
    goto state0;
    
state309: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state309;
    case '-': 
        goto state310;
    case '.': 
        goto state312;
    }
    goto state0;
    
state310: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state311;
    }
    goto state0;
    
state311: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state311;
    }
    goto state0;
    
state312: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state313;
    }
    goto state0;
    
state313: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state313;
    case '.': 
        goto state314;
    }
    goto state0;
    
state314: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state315;
    }
    goto state0;
    
state315: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(%X0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state314;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state315;
    }
    goto state0;
    
state316: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case 9: case ' ': case '(': 
        goto state316;
    case 13: 
        goto state317;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case ';': 
        goto state327;
    }
    goto state0;
    
state317: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((\r
    switch (*ptr++) {
    case 10: 
        goto state318;
    }
    goto state0;
    
state318: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state316;
    }
    goto state0;
    
state319: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    }
    goto state0;
    
state320: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*A
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state320;
    }
    goto state0;
    
state321: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case 9: case ' ': case '[': 
        goto state321;
    case 13: 
        goto state322;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case ';': 
        goto state325;
    }
    goto state0;
    
state322: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[\r
    switch (*ptr++) {
    case 10: 
        goto state323;
    }
    goto state0;
    
state323: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state321;
    }
    goto state0;
    
state324: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[0
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    }
    goto state0;
    
state325: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state325;
    case 13: 
        goto state326;
    }
    goto state0;
    
state326: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((*[;\r
    switch (*ptr++) {
    case 10: 
        goto state323;
    }
    goto state0;
    
state327: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state327;
    case 13: 
        goto state328;
    }
    goto state0;
    
state328: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t((;\r
    switch (*ptr++) {
    case 10: 
        goto state318;
    }
    goto state0;
    
state329: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state329;
    case 13: 
        goto state330;
    }
    goto state0;
    
state330: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state288;
    }
    goto state0;
    
state331: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*
    switch (*ptr++) {
    case '"': 
        goto state256;
    case '%': 
        goto state257;
    case '(': 
        goto state286;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state331;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state332;
    case '[': 
        goto state333;
    }
    goto state0;
    
state332: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*A
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state332;
    }
    goto state0;
    
state333: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case 9: case ' ': 
        goto state333;
    case 13: 
        goto state334;
    case ';': 
        goto state336;
    }
    goto state0;
    
state334: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state335;
    }
    goto state0;
    
state335: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state333;
    }
    goto state0;
    
state336: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state336;
    case 13: 
        goto state337;
    }
    goto state0;
    
state337: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state335;
    }
    goto state0;
    
state338: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t0
    switch (*ptr++) {
    case '"': 
        goto state256;
    case '%': 
        goto state257;
    case '(': 
        goto state286;
    case '*': 
        goto state331;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state332;
    case '[': 
        goto state333;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state338;
    }
    goto state0;
    
state339: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state339;
    case 13: 
        goto state340;
    }
    goto state0;
    
state340: // A\t/=""\t\r\n\t%B0-0/""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state255;
    }
    goto state0;
    
state341: // A\t/=""\t\r\n\t%B0-0/""\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state341;
    case 13: 
        goto state342;
    }
    goto state0;
    
state342: // A\t/=""\t\r\n\t%B0-0/""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state252;
    }
    goto state0;
    
state343: // A\t/=""\t\r\n\t%B0-0/%
    switch (*ptr++) {
    case 'B': case 'b': 
        goto state344;
    case 'D': case 'd': 
        goto state352;
    case 'X': case 'x': 
        goto state360;
    }
    goto state0;
    
state344: // A\t/=""\t\r\n\t%B0-0/%B
    switch (*ptr++) {
    case '0': case '1': 
        goto state345;
    }
    goto state0;
    
state345: // A\t/=""\t\r\n\t%B0-0/%B0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': 
        goto state345;
    case '-': 
        goto state346;
    case '.': 
        goto state348;
    }
    goto state0;
    
state346: // A\t/=""\t\r\n\t%B0-0/%B0-
    switch (*ptr++) {
    case '0': case '1': 
        goto state347;
    }
    goto state0;
    
state347: // A\t/=""\t\r\n\t%B0-0/%B0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': 
        goto state347;
    }
    goto state0;
    
state348: // A\t/=""\t\r\n\t%B0-0/%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state349;
    }
    goto state0;
    
state349: // A\t/=""\t\r\n\t%B0-0/%B0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': 
        goto state349;
    case '.': 
        goto state350;
    }
    goto state0;
    
state350: // A\t/=""\t\r\n\t%B0-0/%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state351;
    }
    goto state0;
    
state351: // A\t/=""\t\r\n\t%B0-0/%B0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state350;
    case '0': case '1': 
        goto state351;
    }
    goto state0;
    
state352: // A\t/=""\t\r\n\t%B0-0/%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state353;
    }
    goto state0;
    
state353: // A\t/=""\t\r\n\t%B0-0/%D0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state353;
    case '-': 
        goto state354;
    case '.': 
        goto state356;
    }
    goto state0;
    
state354: // A\t/=""\t\r\n\t%B0-0/%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state355;
    }
    goto state0;
    
state355: // A\t/=""\t\r\n\t%B0-0/%D0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state355;
    }
    goto state0;
    
state356: // A\t/=""\t\r\n\t%B0-0/%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state357;
    }
    goto state0;
    
state357: // A\t/=""\t\r\n\t%B0-0/%D0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state357;
    case '.': 
        goto state358;
    }
    goto state0;
    
state358: // A\t/=""\t\r\n\t%B0-0/%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state359;
    }
    goto state0;
    
state359: // A\t/=""\t\r\n\t%B0-0/%D0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state358;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state359;
    }
    goto state0;
    
state360: // A\t/=""\t\r\n\t%B0-0/%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state361;
    }
    goto state0;
    
state361: // A\t/=""\t\r\n\t%B0-0/%X0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state361;
    case '-': 
        goto state362;
    case '.': 
        goto state364;
    }
    goto state0;
    
state362: // A\t/=""\t\r\n\t%B0-0/%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state363;
    }
    goto state0;
    
state363: // A\t/=""\t\r\n\t%B0-0/%X0-0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state363;
    }
    goto state0;
    
state364: // A\t/=""\t\r\n\t%B0-0/%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state365;
    }
    goto state0;
    
state365: // A\t/=""\t\r\n\t%B0-0/%X0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state365;
    case '.': 
        goto state366;
    }
    goto state0;
    
state366: // A\t/=""\t\r\n\t%B0-0/%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state367;
    }
    goto state0;
    
state367: // A\t/=""\t\r\n\t%B0-0/%X0.0.0
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '.': 
        goto state366;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state367;
    }
    goto state0;
    
state368: // A\t/=""\t\r\n\t%B0-0/(
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case 9: case ' ': 
        goto state368;
    case 13: 
        goto state369;
    case ';': 
        goto state371;
    }
    goto state0;
    
state369: // A\t/=""\t\r\n\t%B0-0/(\r
    switch (*ptr++) {
    case 10: 
        goto state370;
    }
    goto state0;
    
state370: // A\t/=""\t\r\n\t%B0-0/(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state368;
    }
    goto state0;
    
state371: // A\t/=""\t\r\n\t%B0-0/(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state371;
    case 13: 
        goto state372;
    }
    goto state0;
    
state372: // A\t/=""\t\r\n\t%B0-0/(;\r
    switch (*ptr++) {
    case 10: 
        goto state370;
    }
    goto state0;
    
state373: // A\t/=""\t\r\n\t%B0-0/*
    switch (*ptr++) {
    case '"': 
        goto state248;
    case '%': 
        goto state343;
    case '(': 
        goto state368;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state373;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state374;
    case '[': 
        goto state375;
    }
    goto state0;
    
state374: // A\t/=""\t\r\n\t%B0-0/*A
    switch (*ptr++) {
    case '/': 
        goto state245;
    case 9: case ' ': 
        goto state250;
    case 13: 
        goto state260;
    case ';': 
        goto state264;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state374;
    }
    goto state0;
    
state375: // A\t/=""\t\r\n\t%B0-0/*[
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case 9: case ' ': 
        goto state375;
    case 13: 
        goto state376;
    case ';': 
        goto state378;
    }
    goto state0;
    
state376: // A\t/=""\t\r\n\t%B0-0/*[\r
    switch (*ptr++) {
    case 10: 
        goto state377;
    }
    goto state0;
    
state377: // A\t/=""\t\r\n\t%B0-0/*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state375;
    }
    goto state0;
    
state378: // A\t/=""\t\r\n\t%B0-0/*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state378;
    case 13: 
        goto state379;
    }
    goto state0;
    
state379: // A\t/=""\t\r\n\t%B0-0/*[;\r
    switch (*ptr++) {
    case 10: 
        goto state377;
    }
    goto state0;
    
state380: // A\t/=""\t\r\n\t%B0-0/0
    switch (*ptr++) {
    case '"': 
        goto state248;
    case '%': 
        goto state343;
    case '(': 
        goto state368;
    case '*': 
        goto state373;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state374;
    case '[': 
        goto state375;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state380;
    }
    goto state0;
    
state381: // A\t/=""\t\r\n\t%B0-0/;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state381;
    case 13: 
        goto state382;
    }
    goto state0;
    
state382: // A\t/=""\t\r\n\t%B0-0/;\r
    switch (*ptr++) {
    case 10: 
        goto state247;
    }
    goto state0;
    
state383: // A\t/=""\t\r\n\t%B0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state384;
    }
    goto state0;
    
state384: // A\t/=""\t\r\n\t%B0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': 
        goto state384;
    case '.': 
        goto state385;
    }
    goto state0;
    
state385: // A\t/=""\t\r\n\t%B0.0.
    switch (*ptr++) {
    case '0': case '1': 
        goto state386;
    }
    goto state0;
    
state386: // A\t/=""\t\r\n\t%B0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state385;
    case '0': case '1': 
        goto state386;
    }
    goto state0;
    
state387: // A\t/=""\t\r\n\t%D
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state388;
    }
    goto state0;
    
state388: // A\t/=""\t\r\n\t%D0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state388;
    case '-': 
        goto state389;
    case '.': 
        goto state391;
    }
    goto state0;
    
state389: // A\t/=""\t\r\n\t%D0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state390;
    }
    goto state0;
    
state390: // A\t/=""\t\r\n\t%D0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state390;
    }
    goto state0;
    
state391: // A\t/=""\t\r\n\t%D0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state392;
    }
    goto state0;
    
state392: // A\t/=""\t\r\n\t%D0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state392;
    case '.': 
        goto state393;
    }
    goto state0;
    
state393: // A\t/=""\t\r\n\t%D0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state394;
    }
    goto state0;
    
state394: // A\t/=""\t\r\n\t%D0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state393;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state394;
    }
    goto state0;
    
state395: // A\t/=""\t\r\n\t%X
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state396;
    }
    goto state0;
    
state396: // A\t/=""\t\r\n\t%X0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state396;
    case '-': 
        goto state397;
    case '.': 
        goto state399;
    }
    goto state0;
    
state397: // A\t/=""\t\r\n\t%X0-
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state398;
    }
    goto state0;
    
state398: // A\t/=""\t\r\n\t%X0-0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state398;
    }
    goto state0;
    
state399: // A\t/=""\t\r\n\t%X0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state400;
    }
    goto state0;
    
state400: // A\t/=""\t\r\n\t%X0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state400;
    case '.': 
        goto state401;
    }
    goto state0;
    
state401: // A\t/=""\t\r\n\t%X0.0.
    switch (*ptr++) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state402;
    }
    goto state0;
    
state402: // A\t/=""\t\r\n\t%X0.0.0
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '.': 
        goto state401;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case 'A': case 'B': case 'C': case 'D':
    case 'E': case 'F': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': 
        goto state402;
    }
    goto state0;
    
state403: // A\t/=""\t\r\n\t(
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case 9: case ' ': 
        goto state403;
    case 13: 
        goto state404;
    case ';': 
        goto state406;
    }
    goto state0;
    
state404: // A\t/=""\t\r\n\t(\r
    switch (*ptr++) {
    case 10: 
        goto state405;
    }
    goto state0;
    
state405: // A\t/=""\t\r\n\t(\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state403;
    }
    goto state0;
    
state406: // A\t/=""\t\r\n\t(;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state406;
    case 13: 
        goto state407;
    }
    goto state0;
    
state407: // A\t/=""\t\r\n\t(;\r
    switch (*ptr++) {
    case 10: 
        goto state405;
    }
    goto state0;
    
state408: // A\t/=""\t\r\n\t*
    switch (*ptr++) {
    case '"': 
        goto state237;
    case '%': 
        goto state238;
    case '(': 
        goto state403;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state408;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state409;
    case '[': 
        goto state410;
    }
    goto state0;
    
state409: // A\t/=""\t\r\n\t*A
    switch (*ptr++) {
    case 9: case ' ': 
        goto state231;
    case 13: 
        goto state241;
    case '/': 
        goto state245;
    case ';': 
        goto state294;
    case '-': case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case 'A': case 'B': case 'C':
    case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
    case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
    case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        goto state409;
    }
    goto state0;
    
state410: // A\t/=""\t\r\n\t*[
    switch (*ptr++) {
    case '"': 
        goto state229;
    case '%': 
        goto state289;
    case '(': 
        goto state316;
    case '*': 
        goto state319;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state320;
    case '[': 
        goto state321;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state324;
    case 9: case ' ': 
        goto state410;
    case 13: 
        goto state411;
    case ';': 
        goto state413;
    }
    goto state0;
    
state411: // A\t/=""\t\r\n\t*[\r
    switch (*ptr++) {
    case 10: 
        goto state412;
    }
    goto state0;
    
state412: // A\t/=""\t\r\n\t*[\r\n
    switch (*ptr++) {
    case 9: case ' ': 
        goto state410;
    }
    goto state0;
    
state413: // A\t/=""\t\r\n\t*[;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state413;
    case 13: 
        goto state414;
    }
    goto state0;
    
state414: // A\t/=""\t\r\n\t*[;\r
    switch (*ptr++) {
    case 10: 
        goto state412;
    }
    goto state0;
    
state415: // A\t/=""\t\r\n\t0
    switch (*ptr++) {
    case '"': 
        goto state237;
    case '%': 
        goto state238;
    case '(': 
        goto state403;
    case '*': 
        goto state408;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b':
    case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z': 
        goto state409;
    case '[': 
        goto state410;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': 
        goto state415;
    }
    goto state0;
    
state416: // A\t/=""\t\r\n\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state416;
    case 13: 
        goto state417;
    }
    goto state0;
    
state417: // A\t/=""\t\r\n\t;\r
    switch (*ptr++) {
    case 10: 
        goto state236;
    }
    goto state0;
    
state418: // A\t/=""\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state418;
    case 13: 
        goto state419;
    }
    goto state0;
    
state419: // A\t/=""\t;\r
    switch (*ptr++) {
    case 10: 
        goto state233;
    }
    goto state0;
    
state420: // A\t/=;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state420;
    case 13: 
        goto state421;
    }
    goto state0;
    
state421: // A\t/=;\r
    switch (*ptr++) {
    case 10: 
        goto state228;
    }
    goto state0;
    
state422: // A\t;
    switch (*ptr++) {
    case 9: case ' ': case '!': case '"': case '#': case '$': case '%':
    case '&': case '\'': case '(': case ')': case '*': case '+': case ',':
    case '-': case '.': case '/': case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': case '8': case '9': case ':':
    case ';': case '<': case '=': case '>': case '?': case '@': case 'A':
    case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
    case 'W': case 'X': case 'Y': case 'Z': case '[': case '\\': case ']':
    case '^': case '_': case '`': case 'a': case 'b': case 'c': case 'd':
    case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
    case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~': 
        goto state422;
    case 13: 
        goto state423;
    }
    goto state0;
    
state423: // A\t;\r
    switch (*ptr++) {
    case 10: 
        goto state224;
    }
    goto state0;
    
}
