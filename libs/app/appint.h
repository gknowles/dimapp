// Copyright Glen Knowles 2017 - 2022.
// Distributed under the Boost Software License, Version 1.0.
//
// appint.h - dim app
//
// Initialize and destroy methods called by appRun(), and app*() methods
// for use by those methods.
#pragma once

namespace Dim {

// App
// Add task to be run immediately after onAppRun()
void iAppQueueStartupTask(ITaskNotify * task);
void iAppSetFlags(EnumFlags<AppFlags> flags);

// AppPerf
void iAppPerfInitialize();

// Config
void iConfigInitialize();
void iConfigWebInitialize();

// LogFile
void iLogFileInitialize();
void iLogFileWebInitialize();

// WebAdmin
void iWebAdminInitialize();

} // namespace
