// Copyright Glen Knowles 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// socketmgr.h - dim net
#pragma once

#include "config/config.h"

#include "net/address.h"
#include "net/appsocket.h"

#include <memory>

namespace Dim {

void sockMgrListen(AppSocket::Family fam, std::string type, Endpoint end);


} // namespace
