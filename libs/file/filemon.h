// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// filemon.h - dim file
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"

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
    virtual ~IFileChangeNotify () {}

    virtual void onFileChange(std::string_view fullpath) = 0;
};

struct FileMonitorHandle : HandleBase {};

// The notifier, if present, is always called with the fullpath of the root
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

// Normalized path that would be monitored for file, relative to base 
// directory. False for invalid parameters (bad dir, file outside of dir, 
// etc.)
bool fileMonitorPath(
    std::string & out, 
    FileMonitorHandle dir, 
    std::string_view file
);

} // namespace