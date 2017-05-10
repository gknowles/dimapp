// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim app
//
// Initialize and destroy methods called by appRun()

#pragma once

#include <string_view>

namespace Dim {

// App
// Add task to be run immediately after onAppRun()
void iAppPushStartupTask(ITaskNotify & task);

// Config
void iConfigInitialize();

// LogFile
void iLogFileInitialize();

// WebAdmin
void iWebAdminInitialize();

} // namespace
