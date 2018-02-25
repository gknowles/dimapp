// Copyright Glen Knowles 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// intern.h - dim msgpack
#pragma once

namespace Dim {


/****************************************************************************
*
*   Declarations
*
***/

namespace MsgPack {

enum Format : uint8_t {
    kInvalid = 0xc1,
    kFixArrayMask = 0x90,
    kArray16 = 0xdc,
    kArray32 = 0xdd,
    kFixMapMask = 0x80,
    kMap16 = 0xde,
    kMap32 = 0xdf,
    kNil = 0xc0,
    kFalse = 0xc2,
    kTrue = 0xc3,
    kPositiveFixIntMask = 0x00,
    kNegativeFixIntMask = 0xe0,
    kInt8 = 0xd0,
    kInt16 = 0xd1,
    kInt32 = 0xd2,
    kInt64 = 0xd3,
    kUint8 = 0xcc,
    kUint16 = 0xcd,
    kUint32 = 0xce,
    kUint64 = 0xcf,
    kFloat32 = 0xca,
    kFloat64 = 0xcb,
    kFixStrMask = 0xa0,
    kStr8 = 0xd9,
    kStr16 = 0xda,
    kStr32 = 0xdb,
    kBin8 = 0xc4,
    kBin16 = 0xc5,
    kBin32 = 0xc6,
    kExt8 = 0xc7,
    kExt16 = 0xc8,
    kExt32 = 0xc9,
    kFixExt1 = 0xd4,
    kFixExt2 = 0xd5,
    kFixExt4 = 0xd6,
    kFixExt8 = 0xd7,
    kFixExt16 = 0xd8,
};

} // namespace

} // namespace
