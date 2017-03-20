// socketmgr.h - dim core
#pragma once

#include "config/config.h"

#include "net/address.h"
#include "net/appsocket.h"

#include <memory>

namespace Dim {

void sockMgrListen(AppSocket::Family fam, std::string type, Endpoint end);


} // namespace
