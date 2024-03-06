// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace Service::Nvidia::Devices {

/// Represents an abstract nvidia device node. It is to be subclassed by concrete device nodes to
/// implement the ioctl interface.
class nvdevice {
public:
    nvdevice() = default;
    virtual ~nvdevice() = default;
    union Ioctl {
        u32_le raw;
        BitField<0, 8, u32_le> cmd;
        BitField<8, 8, u32_le> group;
        BitField<16, 14, u32_le> length;
        BitField<30, 1, u32_le> is_in;
        BitField<31, 1, u32_le> is_out;
    };

    /**
     * Handles an ioctl request.
     * @param command The ioctl command id.
     * @param input A buffer containing the input data for the ioctl.
     * @param output A buffer where the output data will be written to.
     * @returns The result code of the ioctl.
     */
    virtual u32 ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) = 0;
};

} // namespace Service::Nvidia::Devices
