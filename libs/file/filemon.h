// Copyright Glen Knowles 2015 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// filemon.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"
#include "file/path.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Dim {


/****************************************************************************
*
*   Monitor
*
***/

class IFileChangeNotify {
public:
    virtual ~IFileChangeNotify () = default;
    virtual void onFileChange(std::string_view fullpath) = 0;
};

struct FileMonitorHandle : HandleBase {};

// The notifier, if present, is always called with the full path of the root
// directory and a nullptr for the File*. This happens for every rename or
// change to last write time of any file within it's scope.
bool fileMonitorDir(
    FileMonitorHandle * handle,
    std::string_view dir,
    bool recurse,
    IFileChangeNotify * notify = nullptr
);
void fileMonitorCloseWait(FileMonitorHandle dir);

// Absolute path to directory being monitored
std::string_view fileMonitorPath(FileMonitorHandle dir);

void fileMonitor(
    FileMonitorHandle dir,
    std::string_view file,
    IFileChangeNotify * notify
);
void fileMonitorCloseWait(
    FileMonitorHandle dir,
    std::string_view file,
    IFileChangeNotify * notify
);

// Normalized path that would be monitored for the file, relative to base
// directory. Returns false for invalid parameters (bad directory, file
// outside of directory, etc.)
bool fileMonitorPath(
    Path * out,
    FileMonitorHandle dir,
    std::string_view file
);

} // namespace
