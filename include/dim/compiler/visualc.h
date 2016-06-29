// visualc.h - dim compiler config
#pragma once

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(                                                               \
    disable : 4324) // structure was padded due to alignment specifier
#pragma warning(                                                               \
    disable : 4456) // declaration of 'elem' hides previous local declaration
#pragma warning(disable : 5030) // attribute 'identifier' is not recognized

// alignment changed after including header
// #pragma warning(disable: 4103)
// the result of the unary '&' operator may be unaligned
// #pragma warning(disable: 4366)
// #pragma pack(1)

#define _NO_LOCALES 0
#define _ITERATOR_DEBUG_LEVEL 0
