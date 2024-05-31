// Copyright Glen Knowles 2024.
// Distributed under the Boost Software License, Version 1.0.
//
// memory.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   operator new(bytes)
*
***/

//===========================================================================
static void * alloc(size_t count) {
    const char * file = nullptr;
    byte buf[256];
    pmr::monotonic_buffer_resource res(buf, size(buf));
    pmr::polymorphic_allocator<stacktrace_entry> pa(&res);
    auto stack = pmr::stacktrace::current(2, 1, pa);
    if (!stack.empty())
        file = (const char *) stack[0].native_handle();
    auto out = _malloc_dbg(count, _NORMAL_BLOCK, file, 0);
    assert(out);
    return out;
}

//===========================================================================
void * operator new(size_t count) {
    return alloc(count);
}

//===========================================================================
void * operator new[](size_t count) {
    return alloc(count);
}


/****************************************************************************
*
*   operator new(source_location)
*
***/

//===========================================================================
void * operator new(size_t count, const source_location & loc) noexcept {
    auto out = _malloc_dbg(
        count,
        _NORMAL_BLOCK,
        loc.file_name(),
        loc.line()
    );
    return out;
}

//===========================================================================
void * operator new[](size_t count, const source_location & loc) noexcept {
    return operator new(count, loc);
}

//===========================================================================
void operator delete(void * ptr, const source_location & loc) noexcept {
    _free_dbg(ptr, _NORMAL_BLOCK);
}

//===========================================================================
void operator delete[](void * ptr, const source_location & loc) noexcept {
    return operator delete(ptr, loc);
}
