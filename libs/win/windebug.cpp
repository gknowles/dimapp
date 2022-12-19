// Copyright Glen Knowles 2020 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// windebug.cpp - dim windows platform
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;

#include <array>


/****************************************************************************
*
*   OtherHeap
*
***/

namespace {

class OtherHeap : public ITempHeap {
public:
    OtherHeap(HANDLE h);
    ~OtherHeap();

    // ITempHeap
    using ITempHeap::alloc;
    char * alloc(size_t bytes, size_t alignment) override;

    // Inherited via std::pmr::memory_resource
    void * do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void * ptr, size_t bytes, size_t alignment) override;
    bool do_is_equal(
        const std::pmr::memory_resource & other
    ) const noexcept override;

private:
    HANDLE m_heap = INVALID_HANDLE_VALUE;
};

} // namespace

//===========================================================================
OtherHeap::OtherHeap(HANDLE h)
    : m_heap(h)
{}

//===========================================================================
OtherHeap::~OtherHeap() {
    if (!HeapDestroy(m_heap))
        logMsgError() << "HeapDestroy(Other): " << WinError{};
}

//===========================================================================
inline void * OtherHeap::do_allocate(size_t bytes, size_t alignment) {
    assert(has_single_bit(alignment));
    if (alignment <= MEMORY_ALLOCATION_ALIGNMENT)
        return HeapAlloc(m_heap, 0, bytes);
    auto space = bytes + alignment - MEMORY_ALLOCATION_ALIGNMENT;
    auto ptr = HeapAlloc(m_heap, 0, space);
    return align(alignment, bytes, ptr, space);
}

//===========================================================================
inline void OtherHeap::do_deallocate(
    void * ptr,
    size_t bytes,
    size_t alignment
) {
    HeapFree(m_heap, 0, ptr);
}

//===========================================================================
inline bool OtherHeap::do_is_equal(
    const pmr::memory_resource & other
) const noexcept {
    return this == &other;
}

//===========================================================================
char * OtherHeap::alloc(size_t bytes, size_t alignment) {
    return (char *) do_allocate(bytes, alignment);
}


/****************************************************************************
*
*   Public API
*
***/

namespace {

const int kNoMansLandSize = 4;
const unsigned char kNoMansLandImage[] = {
    0xfd, 0xfd, 0xfd, 0xfd
};
static_assert(kNoMansLandSize == size(kNoMansLandImage));

struct CrtMemBlockHeader {
    CrtMemBlockHeader * blockHeaderNext;
    CrtMemBlockHeader * blockHeaderPrev;
    const char * fileName;
    int lineNumber;
    int blockUse;
    size_t dataSize;
    long requestNumber;
    unsigned char gap[kNoMansLandSize];

    // Followed by:
    // unsigned char data[dataSize];
    // unsigned char anotherGap[kNoMansLandSize];
};

struct MemRef {
    const char * fileName;
    int lineNumber;
    size_t dataSize;

    bool operator==(const MemRef &) const = default;
};

} // namespace

template<>
struct std::hash<MemRef> {
    size_t operator()(const MemRef & val) const {
        auto hash = std::hash<const char *>()(val.fileName);
        hashCombine(&hash, std::hash<int>()(val.lineNumber));
        hashCombine(&hash, std::hash<size_t>()(val.dataSize));
        return hash;
    }
};

//===========================================================================
void Dim::debugDumpMemory(IJBuilder * out) {
    auto h2 = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
    if (!h2) {
        logMsgError() << "HeapCreate: " << WinError{};
        return;
    }
    OtherHeap heap2(h2);
    pmr::monotonic_buffer_resource mr(&heap2);
    auto files = heap2.emplace<pmr::unordered_set<const char *>>(&mr);
    auto groups = heap2.emplace<pmr::unordered_map<MemRef, size_t>>(&mr);

    HANDLE h = (HANDLE) _get_heap_handle();
    if (!HeapLock(h)) {
        logMsgFatal() << "HeapLock(crtHeap): " << WinError();
        return;
    }

    WinError err = 0;
    {
        Finally funlock([h]() {
            if (!HeapUnlock(h))
                logMsgFatal() << "HeapUnlock(crtHeap): " << WinError();
        });

        PROCESS_HEAP_ENTRY ent = {};
        while (HeapWalk(h, &ent)) {
            if (~ent.wFlags & PROCESS_HEAP_ENTRY_BUSY)
                continue;
            if (ent.cbData < sizeof(CrtMemBlockHeader) + kNoMansLandSize)
                continue;
            auto hdr = reinterpret_cast<CrtMemBlockHeader *>(ent.lpData);
            if (memcmp(hdr->gap, kNoMansLandImage, kNoMansLandSize) != 0)
                continue;
            auto anotherGap = (unsigned char *) ent.lpData + ent.cbData
                - kNoMansLandSize;
            if (memcmp(anotherGap, kNoMansLandImage, kNoMansLandSize) != 0)
                continue;
            if (!files->contains(hdr->fileName)) {
                // validate as pointer to valid filename:
                //  - in bounds
                //  - less than 256 chars
                files->insert(hdr->fileName);
            }
            MemRef ref = {
                .fileName = hdr->fileName,
                .lineNumber = hdr->lineNumber,
                .dataSize = hdr->dataSize,
            };
            (*groups)[ref] += 1;
        }
        err.set();
    }
    if (err != ERROR_NO_MORE_ITEMS)
        logMsgError() << "HeapWalk(crtHeap): " << err;

    out->member("blocks").array();
    for (auto&& mem : *groups) {
        out->object();
        if (mem.first.fileName) {
            out->member("file", mem.first.fileName)
                .member("line", mem.first.lineNumber);
        } else {
            out->member("file", "UNKNOWN")
                .member("line", 0);
        }
        out->member("size", mem.first.dataSize);
        out->member("count", mem.second);
        out->end();
    }
    out->end();
}


/****************************************************************************
*
*   Internal API
*
***/

static wchar_t s_fname[MAX_PATH] = L"memleak.log";

//===========================================================================
extern "C" static int attachMemLeakHandle() {
    auto leakFlags = 0;
#ifndef __SANITIZE_ADDRESS__
    leakFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
#endif
    if (~leakFlags & _CRTDBG_LEAK_CHECK_DF) {
        // Leak check disabled.
        return 0;
    }

    _CrtMemState state;
    _CrtMemCheckpoint(&state);
    auto leaks = state.lCounts[_CLIENT_BLOCK] + state.lCounts[_NORMAL_BLOCK];
    if (leaks) {
        if (GetConsoleWindow() != NULL) {
            // Has console attached.
            wchar_t buf[256];
            auto len = swprintf(
                buf,
                size(buf),
                L"\nMemory leaks: %zu, see %s for details.\n",
                leaks,
                s_fname
            );
            ConsoleScopedAttr attr(kConsoleError);
            HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            WriteConsoleW(hOutput, buf, len, NULL, NULL);
        }
        auto f = CreateFileW(
            s_fname,
            GENERIC_READ | GENERIC_WRITE,
            0,      // sharing
            NULL,   // security attributes
            CREATE_ALWAYS,
            0,      // flags and attrs
            NULL    // template file
        );
        if (f == INVALID_HANDLE_VALUE) {
            WinError err;
        } else {
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, f);
            _CrtDumpMemoryLeaks();
            CloseHandle(f);
        }

        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        if (IsDebuggerPresent())
            __debugbreak();
    }
    return 0;
}

// pragma sections
// .CRT$Xpq
//  p (category) values:
//      I   C init
//      C   C++ init, aka namespace scope constructors
//      D   DLL TLS init (per thread)
//      L   Loader TLS Callback (DLL_PROCESS_ATTACH, DLL_THREAD_ATTACH,
//              DLL_THREAD_DETACH, and DLL_PROCESS_DETACH)
//      P   pre-terminators
//      T   terminators
//  q (segment) values:
//      A   reserved for start of list sentinel (A[A-Z] can be used)
//      C   compiler
//      L   library
//      U   user
//      X   terminator ?
//      Z   reserved for end of segments sentinel
//
// Segments within a terminators category are executed in alphabetical order,
// XTU is the user termination segment which runs after the compiler and
// library segments have been terminated.

#pragma section(".CRT$XTU", long, read)
#pragma data_seg(push)
#pragma data_seg(".CRT$XTU")
static auto s_attachMemLeakHandle = attachMemLeakHandle;
#pragma data_seg(pop)

//===========================================================================
void Dim::winDebugInitialize(PlatformInit phase) {
    if (phase == PlatformInit::kBeforeAppVars) {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        return;
    }

    assert(phase == PlatformInit::kAfterAppVars);
    auto fname = Path("memleak-" + appName()).defaultExt("log");
    Path out;
    if (appLogPath(&out, fname))
        strCopy(s_fname, size(s_fname), out.c_str());
}
