// Copyright Glen Knowles 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// tempheap.cpp - dim core
#include "pch.h"
#pragma hdrstop

using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kBufferSize = 4096;


/****************************************************************************
*
*   Declarations
*
***/

namespace {

struct Buffer {
    Buffer * m_next;
    size_t m_avail;
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
char * TempHeap::alloc(size_t bytes, size_t align) {
    Buffer * buf = (Buffer *)m_buffer;
    Buffer * tmp;
    for (;;) {
        if (buf) {
            void * ptr = (char *)buf + kBufferSize - buf->m_avail;
            if (std::align(align, bytes, ptr, buf->m_avail)) {
                buf->m_avail -= bytes;
                return (char *)ptr;
            }
        }
        if (bytes + align > kBufferSize / 3) {
            tmp = (Buffer *)malloc(sizeof(Buffer) + bytes + align);
            assert(tmp != nullptr);
            tmp->m_avail = bytes + align;
            if (buf) {
                tmp->m_next = buf->m_next;
                buf->m_next = tmp;
            } else {
                tmp->m_next = nullptr;
                m_buffer = tmp;
            }
        } else {
            tmp = (Buffer *)malloc(kBufferSize);
            assert(tmp != nullptr);
            tmp->m_next = buf;
            tmp->m_avail = kBufferSize - sizeof(Buffer);
            m_buffer = tmp;
        }
        buf = tmp;
    }
}
