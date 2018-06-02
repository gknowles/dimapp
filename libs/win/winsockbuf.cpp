// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// winsockbuf.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Tuning parameters
*
***/

const unsigned kDefaultBufferSliceSize = 4096;
const unsigned kDefaultBufferSize = 256 * kDefaultBufferSliceSize;


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

struct BufferHandle : HandleBase {
    BufferHandle() {}
    explicit BufferHandle(const SocketBuffer & sbuf) { pos = sbuf.owner; }
};

struct Buffer : HandleContent, ListBaseLink<> {
    RIO_BUFFERID id;
    char * base;
    int sliceSize;
    int reserved;
    int size;
    int used;
    int firstFree;
    BufferHandle h;
};

struct BufferSlice {
    int nextPos;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static RIO_EXTENSION_FUNCTION_TABLE s_rio;

static int s_sliceSize = kDefaultBufferSliceSize;
static size_t s_bufferSize = kDefaultBufferSize;
static EnvMemoryConfig s_memcfg;

// buffers are kept sorted by state (full, partial, empty)
static HandleMap<BufferHandle, Buffer> s_buffers;
static List<Buffer> s_fullBufs;
static List<Buffer> s_partialBufs;
static List<Buffer> s_emptyBufs;
static mutex s_mut;

static auto & s_perfSlices = uperf("sock.buffer slices");


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static BufferSlice * getSlice(const Buffer & buf, int pos) {
    char * ptr = (char *)buf.base + pos * buf.sliceSize;
    return (BufferSlice *)ptr;
}

//===========================================================================
static void findBufferSlice(
    BufferSlice ** sliceOut,
    Buffer ** bufferOut,
    const SocketBuffer & sbuf
) {
    auto * slice = (BufferSlice *) sbuf.data;
    BufferHandle h{sbuf};
    auto * buf = s_buffers.find(h);
    assert(h == buf->h);
    // BufferSlice must be aligned within the owning registered buffer
    assert((char *)slice >= buf->base && slice < getSlice(*buf, buf->size));
    assert(((char *)slice - buf->base) % buf->sliceSize == 0);

    *sliceOut = slice;
    *bufferOut = buf;
}

//===========================================================================
static void createEmptyBuffer() {
    size_t bytes = s_bufferSize;
    size_t granularity = s_memcfg.allocAlign;
    DWORD fLargePages = 0;
    if (s_memcfg.minLargeAlloc && s_bufferSize >= s_memcfg.minLargeAlloc) {
        fLargePages = MEM_LARGE_PAGES;
        granularity = s_memcfg.minLargeAlloc;
    }
    // round up, but not to exceed DWORD
    bytes += granularity - 1;
    bytes = bytes - bytes % granularity;
    if (bytes > numeric_limits<DWORD>::max()) {
        bytes = numeric_limits<DWORD>::max() / granularity * granularity;
    }

    auto buf = new Buffer;
    buf->h = s_buffers.insert(buf);
    s_emptyBufs.link(buf);
    buf->sliceSize = s_sliceSize;
    buf->reserved = int(bytes / s_sliceSize);
    buf->size = 0;
    buf->used = 0;
    buf->firstFree = 0;
    buf->base = (char *)VirtualAlloc(
        nullptr,
        bytes,
        MEM_COMMIT | MEM_RESERVE | fLargePages,
        PAGE_READWRITE
    );
    if (!buf->base) {
        logMsgFatal() << "VirtualAlloc(sockbuf): " << WinError{};
        __assume(0);
    }

    buf->id = s_rio.RIORegisterBuffer(buf->base, (DWORD)bytes);
    if (buf->id == RIO_INVALID_BUFFERID)
        logMsgFatal() << "RIORegisterBuffer: " << WinError();
}

//===========================================================================
static void createBufferSlice(
    BufferSlice ** sliceOut,
    Buffer ** bufOut
) {
    // use the first partial or, if there aren't any, the first empty
    auto pbuf = s_partialBufs.front();
    if (!pbuf)
        pbuf = s_emptyBufs.front();
    if (!pbuf) {
        // all buffers full? create a new one
        createEmptyBuffer();
        pbuf = s_emptyBufs.front();
    }
    auto & buf = *pbuf;

    BufferSlice * slice;
    if (buf.used < buf.size) {
        slice = getSlice(buf, buf.firstFree);
        buf.firstFree = slice->nextPos;
    } else {
        assert(buf.size < buf.reserved);
        slice = getSlice(buf, buf.size);
        buf.size += 1;
    }
    buf.used += 1;

    if (buf.used == buf.reserved) {
        // no longer partial: move to full list
        s_fullBufs.linkFront(pbuf);
    } else if (buf.used == 1) {
        // no longer empty: move to partials
        s_partialBufs.linkFront(pbuf);
    }

    s_perfSlices += 1;

    *bufOut = pbuf;
    *sliceOut = slice;
}

//===========================================================================
static void destroyEmptyBuffer() {
    assert(!s_emptyBufs.empty());
    auto buf = s_emptyBufs.unlinkFront();
    auto hostage = unique_ptr<Buffer>(s_buffers.release(buf->h));
    assert(buf == hostage.get());
    assert(!buf->used);
    s_rio.RIODeregisterBuffer(buf->id);
    VirtualFree(buf->base, 0, MEM_RELEASE);
}

//===========================================================================
static void destroyBufferSlice(const SocketBuffer & sbuf) {
    // get the header
    BufferSlice * slice;
    Buffer * pbuf;
    findBufferSlice(&slice, &pbuf, sbuf);
    auto & buf = *pbuf;

    slice->nextPos = buf.firstFree;
    buf.firstFree = int((char *)slice - buf.base) / buf.sliceSize;
    buf.used -= 1;

    if (buf.used == buf.reserved - 1) {
        // no longer full: move to partial list
        s_partialBufs.linkFront(pbuf);
    } else if (!buf.used) {
        // newly empty: move to empty list
        s_emptyBufs.linkFront(pbuf);

        // over half the buffers are empty? destroy one
        if (s_emptyBufs.size() > s_fullBufs.size() + s_partialBufs.size())
            destroyEmptyBuffer();
    }

    s_perfSlices -= 1;
}


/****************************************************************************
*
*   SocketBuffer
*
***/

//===========================================================================
SocketBuffer::~SocketBuffer() {
    scoped_lock lk{s_mut};
    destroyBufferSlice(*this);
}


/****************************************************************************
*
*   Shutdown
*
***/

namespace {

class ShutdownNotify : public IShutdownNotify {
    void onShutdownConsole(bool firstTry) override;
};

} // namespace

static ShutdownNotify s_cleanup;

//===========================================================================
void ShutdownNotify::onShutdownConsole(bool firstTry) {
    if (firstTry)
        return shutdownIncomplete();

    scoped_lock lk{s_mut};
    while (!s_buffers.empty())
        destroyEmptyBuffer();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iSocketBufferInitialize(RIO_EXTENSION_FUNCTION_TABLE & rio) {
    shutdownMonitor(&s_cleanup);

    s_rio = rio;

    s_memcfg = envMemoryConfig();
    s_bufferSize = max(s_memcfg.minLargeAlloc, s_bufferSize);
}

//===========================================================================
void Dim::copy(
    RIO_BUF * out,
    const SocketBuffer & sbuf,
    size_t bytes
) {
    scoped_lock lk{s_mut};

    assert(bytes <= (size_t) sbuf.capacity);
    BufferSlice * slice;
    Buffer * pbuf;
    findBufferSlice(&slice, &pbuf, sbuf);
    out->BufferId = pbuf->id;
    out->Offset = ULONG((char *)sbuf.data - (char *)pbuf->base);
    out->Length = (int)bytes;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
unique_ptr<SocketBuffer> Dim::socketGetBuffer() {
    scoped_lock lk{s_mut};

    BufferSlice * slice;
    Buffer * pbuf;
    createBufferSlice(&slice, &pbuf);

    // set pointer to just passed the header
    auto out = make_unique<SocketBuffer>(
        (char *) slice,
        pbuf->sliceSize - 1,
        pbuf->h.pos
    );

    return out;
}
