// socketmgr.h - dim core
#pragma once

#include "config.h"

#include "address.h"
#include "appsocket.h"

#include <memory>

namespace Dim {

void sockMgrListen(
    AppSocket::Family fam,
    std::string type,
    Endpoint end);
    

} // namespace
