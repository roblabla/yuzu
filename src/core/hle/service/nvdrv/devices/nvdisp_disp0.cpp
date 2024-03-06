// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/devices/nvmap.h"
#include "core/perf_stats.h"
#include "video_core/gpu.h"
#include "video_core/renderer_base.h"

namespace Service::Nvidia::Devices {

nvdisp_disp0::nvdisp_disp0(std::shared_ptr<nvmap> nvmap_dev) : nvmap_dev(std::move(nvmap_dev)) {}
nvdisp_disp0 ::~nvdisp_disp0() = default;

u32 nvdisp_disp0::ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) {
    UNIMPLEMENTED_MSG("Unimplemented ioctl");
    return 0;
}

void nvdisp_disp0::flip(u32 buffer_handle, u32 offset, u32 format, u32 width, u32 height,
                        u32 stride, NVFlinger::BufferQueue::BufferTransformFlags transform,
                        const MathUtil::Rectangle<int>& crop_rect) {
    VAddr addr = nvmap_dev->GetObjectAddress(buffer_handle);
    LOG_WARNING(Service,
                "Drawing from address {:X} offset {:08X} Width {} Height {} Stride {} Format {}",
                addr, offset, width, height, stride, format);

    using PixelFormat = Tegra::FramebufferConfig::PixelFormat;
    const Tegra::FramebufferConfig framebuffer{
        addr,      offset,   width, height, stride, static_cast<PixelFormat>(format),
        transform, crop_rect};

    auto& instance = Core::System::GetInstance();
    instance.GetPerfStats().EndGameFrame();
    instance.Renderer().SwapBuffers(framebuffer);
}

} // namespace Service::Nvidia::Devices
