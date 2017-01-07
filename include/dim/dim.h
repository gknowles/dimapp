// dim.h - dim primary client header
#pragma once

#include "config.h"
#ifdef DIM_LIB_STANDALONE
#error DIM_LIB_STANDALONE invalid when building entire collection
#endif

#include "address.h"
#include "app.h"
#include "appsocket.h"
#include "charbuf.h"
#include "cli.h"
#include "console.h"
#include "file.h"
#include "handle.h"
#include "http.h"
#include "httpRoute.h"
#include "list.h"
#include "log.h"
#include "socket.h"
#include "socketmgr.h"
#include "task.h"
#include "tempheap.h"
#include "timer.h"
#include "tls.h"
#include "tokentable.h"
#include "types.h"
#include "util.h"
#include "xml.h"

#define DIM_LIB_KEEP_MACROS
#include "config_suffix.h"
