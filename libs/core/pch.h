// Copyright Glen Knowles 2015 - 2025.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim core

// Public header
#include "core/core.h"

// External library public headers
#include "basic/basic.h"

// Standard headers
#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <cerrno>
#include <charconv>
#include <condition_variable>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <shared_mutex>
#include <span>
#ifdef __cpp_lib_stacktrace
#include <stacktrace>
#endif
#include <thread>
#include <utility>
#include <vector>

// Platform headers
// External library internal headers
// Internal headers
#include "core/appint.h"
#include "core/threadint.h"
#include "core/timeint.h"
