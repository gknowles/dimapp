// Copyright Glen Knowles 2017 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// file-t.cpp - dimapp test file
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Application
*
***/

//===========================================================================
static void app(Cli & cli) {
    using enum File::OpenMode;

    string fn = "file-t.tmp";

    FileHandle file;
    auto ec = fileOpen(&file, fn, fCreat | fTrunc | fReadWrite | fBlocking);
    if (ec)
        return appSignalShutdown(EX_DATAERR);
    FileAlignment fa;
    if (auto ec = fileAlignment(&fa, file); ec)
        return appSignalShutdown(EX_DATAERR);
    auto psize = fa.physicalSector;
    fileWriteWait(nullptr, file, 0, "aaaa", 4);

    const char * base;
    ec = fileOpenView(base, file, File::View::kReadOnly, 0, 0, 1001 * psize);
    if (ec)
        return appSignalShutdown(EX_DATAERR);

    ec = fileExtendView(file, base, 1001 * psize);
    if (ec)
        return appSignalShutdown(EX_DATAERR);

    unsigned num = 0;
    static char v = 0;
    for (size_t i = 1; i < 1000; ++i) {
        v = 0;
        fileWriteWait(nullptr, file, i * psize, "bbbb", 4);
        v = base[i * psize];
        if (v == 'b')
            num += 1;
    }

    ec = fileExtendView(file, base, psize);
    static char buf[5] = {};
    for (unsigned i = 0; i < 100; ++i) {
        fileReadWait(nullptr, buf, 4, file, psize);
    }
    ec = fileCloseView(file, base);

    auto content = string(10, '#');
    if (auto ec = fileResize(file, size(content)); ec)
        return appSignalShutdown(EX_DATAERR);
    fileWriteWait(nullptr, file, 0, content.data(), content.size());
    struct Reader : IFileReadNotify {
        bool onFileRead(
            size_t * bytesUsed,
            const FileReadData & data
        ) override {
            m_out += data.data;
            *bytesUsed = data.data.size();
            if (!data.more) {
                m_done = true;
                m_cv.notify_all();
            }
            return true;
        }

        string m_out;
        mutex m_mut;
        condition_variable m_cv;
        bool m_done{false};
    } reader;
    fileRead(
        &reader,
        buf,
        size(buf) - 1,
        file,
        0,
        size(content),
        taskComputeQueue()
    );
    unique_lock lk{reader.m_mut};
    while (!reader.m_done)
        reader.m_cv.wait(lk);

    if (reader.m_out != content)
        logMsgError() << "File content mismatch";

    fileClose(file);

    testSignalShutdown();
}


/****************************************************************************
*
*   main
*
***/

//===========================================================================
int main(int argc, char * argv[]) {
    Cli().action(app);
    int code = appRun(argc, argv);
    return code;
}
