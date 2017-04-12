// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// pch.h - dim tls

// Public header
#include "tls/tls.h"
#include "tlsrec.h"

// External library public headers
#include "core/core.h"

// External library internal headers
// Standard headers
#include <cassert>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

// Platform headers
// Internal headers
#include "tlsint.h"
