// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

namespace FileSys {

enum class Mode : u32 {
    Read = 1,
    Write = 2,
    ReadWrite = 3,
    Append = 4,
    WriteAppend = 6,
};

inline u32 operator&(Mode lhs, Mode rhs) {
    return static_cast<u32>(lhs) & static_cast<u32>(rhs);
}

} // namespace FileSys
