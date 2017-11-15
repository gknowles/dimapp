// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// tempheap.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kInitBufferSize = 256;
const unsigned kMaxBufferSize = 4096;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

static_assert(kMaxBufferSize < numeric_limits<unsigned>::max());

struct Buffer {
    Buffer * m_next;
    unsigned m_avail;
    unsigned m_reserve;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/


/****************************************************************************
*
*   TempHeap
*
***/

//===========================================================================
TempHeap::~TempHeap() {
    clear();
}

//===========================================================================
TempHeap & TempHeap::operator=(TempHeap && from) {
    swap(from);
    from.clear();
    return *this;
}

//===========================================================================
void TempHeap::clear() {
    Buffer * ptr = (Buffer *)m_buffer;
    while (ptr) {
        Buffer * next = ptr->m_next;
        free(ptr);
        ptr = next;
    }
    m_buffer = nullptr;
}

//===========================================================================
void TempHeap::swap(TempHeap & from) {
    std::swap(m_buffer, from.m_buffer);
}

//===========================================================================
char * TempHeap::alloc(size_t bytes, size_t align) {
    Buffer * buf = (Buffer *)m_buffer;
    Buffer * tmp;
    constexpr unsigned kBufferLen = sizeof(Buffer);
    for (;;) {
        if (buf) {
            void * ptr = (char *)buf + buf->m_reserve - buf->m_avail;
            size_t avail = buf->m_avail;
            if (std::align(align, bytes, ptr, avail)) {
                buf->m_avail = unsigned(avail - bytes);
                return (char *)ptr;
            }
        }
        auto required = unsigned(bytes + align);
        if (required > kMaxBufferSize / 3) {
            tmp = (Buffer *)malloc(kBufferLen + required);
            assert(tmp != nullptr);
            tmp->m_avail = required;
            tmp->m_reserve = kBufferLen + required;
            if (buf) {
                tmp->m_next = buf->m_next;
                buf->m_next = tmp;
            } else {
                tmp->m_next = nullptr;
                m_buffer = tmp;
            }
        } else {
            auto reserve = buf
                ? min(kMaxBufferSize, 2 * buf->m_reserve)
                : kInitBufferSize;
            if (reserve < kBufferLen + required)
                reserve = kBufferLen + required;
            tmp = (Buffer *)malloc(reserve);
            assert(tmp != nullptr);
            tmp->m_next = buf;
            tmp->m_avail = reserve - kBufferLen;
            tmp->m_reserve = reserve;
            m_buffer = tmp;
        }
        buf = tmp;
    }
}
