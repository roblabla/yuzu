// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "common/math_util.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"
#include "core/hle/service/nvflinger/buffer_queue.h"

namespace Service::Nvidia::Devices {

class nvmap;

class nvdisp_disp0 final : public nvdevice {
public:
    explicit nvdisp_disp0(std::shared_ptr<nvmap> nvmap_dev);
    ~nvdisp_disp0();

    u32 ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) override;

    /// Performs a screen flip, drawing the buffer pointed to by the handle.
    void flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height, u32 stride,
              NVFlinger::BufferQueue::BufferTransformFlags transform,
              const MathUtil::Rectangle<int>& crop_rect);

private:
    std::shared_ptr<nvmap> nvmap_dev;
};

} // namespace Service::Nvidia::Devices
