// Copyright Glen Knowles 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//
// algo.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include <utility> // std::move

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
        goto REST_OF_DST;
    if (dst == edst)
        goto REST_OF_SRC;
    for (;;) {
        while (*dst < *src) {
        MORE_DST:
            add(*dst);
            if (++dst == edst)
                goto REST_OF_SRC;
        }
        while (*src < *dst) {
            remove(*src);
            if (++src == esrc)
                goto REST_OF_DST;
        }
        if (*dst < *src)
            goto MORE_DST;

        ++src, ++dst;
        if (src == esrc) {
            if (dst == edst)
                return;
            goto REST_OF_DST;
        }
        if (dst == edst)
            goto REST_OF_SRC;
    }

REST_OF_DST:
    for (;;) {
        add(*dst);
        if (++dst == edst)
            return;
    }

REST_OF_SRC:
    for (;;) {
        remove(*src);
        if (++src == esrc)
            return;
    }
}

//===========================================================================
template<typename Container, typename Pred>
void erase_unordered_if(Container c, Pred p) {
    auto i = c.begin(),
        ei = c.end();
    while (i != ei) {
        if (!p(*i)) {
            i += 1;
        } else {
            if (--ei == i)
                break;
            *i = std::move(*ei);
        }
    }
    c.erase(ei, c.end());
}


} // namespace
