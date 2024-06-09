// Copyright Glen Knowles 2020 - 2024.
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
*   Declarations
*
***/

static wchar_t s_fname[MAX_PATH] = L"memleak.log";

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
    mutable long request;
    unsigned char * data;

    bool operator==(const MemRef & other) const {
        return operator<=>(other) == 0;
    }
    strong_ordering operator<=>(const MemRef & other) const {
        return tie(lineNumber, dataSize, fileName) <=>
            tie(other.lineNumber, other.dataSize, other.fileName);
    }
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


/****************************************************************************
*
*   OtherHeap
*
***/

namespace {

class OtherHeap : public IHeap {
public:
    OtherHeap();
    ~OtherHeap();
    explicit operator bool() const { return m_heap != INVALID_HANDLE_VALUE; }

    HANDLE handle() const { return m_heap; }

    // Inherited via std::pmr::memory_resource
    void * do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void * ptr, size_t bytes, size_t alignment) override;

private:
    HANDLE m_heap = INVALID_HANDLE_VALUE;
};

} // namespace

//===========================================================================
OtherHeap::OtherHeap() {
    auto h = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
    if (!h) {
        logMsgError() << "HeapCreate: " << WinError{};
        return;
    }
    m_heap = h;
}

//===========================================================================
OtherHeap::~OtherHeap() {
    if (m_heap != INVALID_HANDLE_VALUE && !HeapDestroy(m_heap))
        logMsgError() << "HeapDestroy(Other): " << WinError{};
}

//===========================================================================
void * OtherHeap::do_allocate(size_t bytes, size_t alignment) {
    assert(has_single_bit(alignment));
    if (alignment <= MEMORY_ALLOCATION_ALIGNMENT)
        return HeapAlloc(m_heap, 0, bytes);
    auto space = bytes + alignment - MEMORY_ALLOCATION_ALIGNMENT;
    auto ptr = HeapAlloc(m_heap, 0, space);
    return align(alignment, bytes, ptr, space);
}

//===========================================================================
void OtherHeap::do_deallocate(
    void * ptr,
    size_t bytes,
    size_t alignment
) {
    HeapFree(m_heap, 0, ptr);
}


/****************************************************************************
*
*   MemTrackData
*
***/

struct MemTrackData {
    OtherHeap m_heap;
    pmr::monotonic_buffer_resource m_res;

    MemTrackData();
    explicit operator bool() const { return (bool) m_heap; }
};

//===========================================================================
MemTrackData::MemTrackData()
    : m_res(&m_heap)
{}


/****************************************************************************
*
*   Helpers
*
***/

//===========================================================================
static void eachAlloc(function<bool(const MemRef &)> fn) {
    HANDLE h = (HANDLE) _get_heap_handle();
    if (!HeapLock(h)) {
        logMsgFatal() << "HeapLock(crtHeap): " << WinError();
        return;
    }

    PROCESS_HEAP_ENTRY ent = {};
    while (HeapWalk(h, &ent)) {
        if (~ent.wFlags & PROCESS_HEAP_ENTRY_BUSY)
            continue;
        if (ent.cbData < sizeof(CrtMemBlockHeader) + kNoMansLandSize)
            continue;
        auto hdr = reinterpret_cast<CrtMemBlockHeader *>(ent.lpData);
        if (memcmp(hdr->gap, kNoMansLandImage, kNoMansLandSize) != 0)
            continue;
        auto hdrEnd = (unsigned char *) ent.lpData + ent.cbData;
        auto anotherGap = hdrEnd - kNoMansLandSize;
        if (memcmp(anotherGap, kNoMansLandImage, kNoMansLandSize) != 0)
            continue;
        MemRef ref = {
            .fileName = hdr->fileName,
            .lineNumber = hdr->lineNumber,
            .dataSize = hdr->dataSize,
            .request = hdr->requestNumber,
            .data = hdrEnd,
        };
        if (!fn(ref))
            break;
    }
    WinError err;
    if (!HeapUnlock(h))
        logMsgFatal() << "HeapUnlock(crtHeap): " << WinError();
    if (err != ERROR_NO_MORE_ITEMS)
        logMsgError() << "HeapWalk(crtHeap): " << err;
}

//===========================================================================
static bool checkName(const char * fileName) {
    bool success = true;
    __try {
        if (fileName) {
            if (!memchr(fileName, 0, 512))
                success = false;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        success = false;
    }
    return success;
}

//===========================================================================
static MemRef decode(
    MemTrackData & state,
    const MemRef & mem,
    bool trace
) {
    auto ref = mem;
    if (!checkName(ref.fileName)) {
        ref.fileName = "INVALID";
        return ref;
    }
    if (ref.fileName && ref.lineNumber) {
        return ref;
    }
    if (trace && ref.fileName) {
        stacktrace_entry ent;
        static_assert(sizeof ent == sizeof ref.fileName);
        memcpy(&ent, &ref.fileName, sizeof ent);
        if (ent) {
            ref.fileName = state.m_heap.strDup(ent.source_file());
            ref.lineNumber = ent.source_line();
            return ref;
        }
    }
    ref.fileName = "UNKNOWN";
    return ref;
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::debugDumpMemory(IJBuilder * out) {
    MemTrackData state;
    if (!state)
        return;
    auto groups = state.m_heap.emplace<pmr::unordered_map<MemRef, size_t>>(
        &state.m_res
    );

    eachAlloc([groups](const MemRef & ref) {
        if (auto i = groups->find(ref); i == groups->end()) {
            groups->insert(i, {ref, 1});
        } else {
            if (ref.request > i->first.request)
                i->first.request = ref.request;
            i->second += 1;
        }
        return true;
    });

    out->member("blocks").array();
    for (auto&& mem : *groups) {
        auto ref = decode(state, mem.first, true);
        out->object()
            .member("file", ref.fileName)
            .member("line", ref.lineNumber)
            .member("size", ref.dataSize)
            .member("count", mem.second)
            .member("request", ref.request)
            .end();
    }
    out->end();
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
inline static void dumpMemoryLeaks(HANDLE f) {
    MemTrackData state;
    if (!state)
        return;

    CharBuf out;
    size_t count = 0;
    size_t bytes = 0;
    eachAlloc([&](const MemRef & mem) {
        count += 1;
        bytes += mem.dataSize;
        if (count > 100)
            return true;
        auto ref = decode(state, mem, false);

        // first line, meta data
        out.pushBack('{').append(toChars(ref.request)).append("}");
        if (ref.lineNumber) {
            out.pushBack(' ').append(ref.fileName)
                .pushBack('(').append(toChars(ref.lineNumber)).pushBack(')');
        }
        out.append(": block at ")
            .append(toHexChars(bit_cast<uintptr_t>(ref.data)))
            .append(", ").append(toChars(ref.dataSize))
            .append(" bytes.\n");

        // second line, raw data
        out.pushBack(' ');
        for (auto i = 0; i < 32; ++i) {
            if (i % 4 == 0)
                out.pushBack(' ');
            if (i < ref.dataSize) {
                out.append(hexFromByte(ref.data[i]).data());
            } else {
                out.append("  ");
            }
        }
        out.append("  <");
        for (auto i = 0; i < 32; ++i) {
            if (i < ref.dataSize && isprint(ref.data[i])) {
                out.pushBack(ref.data[i]);
            } else {
                out.pushBack(' ');
            }
        }
        out.append(">\n");
        return true;
    });
    out.append("\nTotal leaks: ")
        .append(toChars(count)).append(" blocks, ")
        .append(toChars(bytes)).append(" bytes.")
        .pushBack('\n');
}

//===========================================================================
extern "C" static int registeredMemLeakCheck() {
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
            wchar_t buf[512];
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
        } else if (false) {
            dumpMemoryLeaks(f);
        } else {
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(_CRT_WARN, f);
            _CrtDumpMemoryLeaks();
        }
        CloseHandle(f);

        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        if (IsDebuggerPresent())
            __debugbreak();
    }
    return 0;
}

//===========================================================================
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
static auto s_registeredMemLeakCheck = registeredMemLeakCheck;
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
