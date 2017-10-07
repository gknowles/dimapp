// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// algo.h - dim core
#pragma once

#include "cppconf/cppconf.h"

namespace Dim {


/****************************************************************************
*
*   Algorithms
*
***/

//===========================================================================
// The ranges must be sorted according to operator<
template<typename Input1, typename Input2, typename Add, typename Remove>
void for_each_diff(
    Input1 dst, Input1 edst,  // values to update to
    Input2 src, Input2 esrc,  // values to update from
    Add add,        // unary function called for each value in dst but not src
    Remove remove   // unary function called for each in src but not dst
) {
    if (src == esrc)
        goto REST_OF_NEXT;
    if (dst == edst)
        goto REST_OF_PREV;
    for (;;) {
        while (*dst < *src) {
        MORE_NEXT:
            add(*dst);
            if (++dst == edst)
                goto REST_OF_PREV;
        }
        while (*src < *dst) {
            remove(*src);
            if (++src == esrc)
                goto REST_OF_NEXT;
        }
        if (*dst < *src)
            goto MORE_NEXT;

        ++src, ++dst;
        if (src == esrc) {
            if (dst == edst)
                return;
            goto REST_OF_NEXT;
        }
        if (dst == edst)
            goto REST_OF_PREV;
    }

REST_OF_NEXT:
    for (;;) {
        add(*dst);
        if (++dst == edst)
            return;
    }

REST_OF_PREV:
    for (;;) {
        remove(*src);
        if (++src == esrc)
            return;
    }
}

} // namespace
