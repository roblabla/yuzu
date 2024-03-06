// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/core.h"
#include "core/memory.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra::Engines {

KeplerMemory::KeplerMemory(VideoCore::RasterizerInterface& rasterizer,
                           MemoryManager& memory_manager)
    : memory_manager(memory_manager), rasterizer{rasterizer} {}

KeplerMemory::~KeplerMemory() = default;

void KeplerMemory::CallMethod(const GPU::MethodCall& method_call) {
    ASSERT_MSG(method_call.method < Regs::NUM_REGS,
               "Invalid KeplerMemory register, increase the size of the Regs structure");

    regs.reg_array[method_call.method] = method_call.argument;

    switch (method_call.method) {
    case KEPLERMEMORY_REG_INDEX(exec): {
        state.write_offset = 0;
        break;
    }
    case KEPLERMEMORY_REG_INDEX(data): {
        ProcessData(method_call.argument);
        break;
    }
    }
}

void KeplerMemory::ProcessData(u32 data) {
    ASSERT_MSG(regs.exec.linear, "Non-linear uploads are not supported");
    ASSERT(regs.dest.x == 0 && regs.dest.y == 0 && regs.dest.z == 0);

    GPUVAddr address = regs.dest.Address();
    VAddr dest_address =
        *memory_manager.GpuToCpuAddress(address + state.write_offset * sizeof(u32));

    // We have to invalidate the destination region to evict any outdated surfaces from the cache.
    // We do this before actually writing the new data because the destination address might contain
    // a dirty surface that will have to be written back to memory.
    rasterizer.InvalidateRegion(dest_address, sizeof(u32));

    Memory::Write32(dest_address, data);
    Core::System::GetInstance().GPU().Maxwell3D().dirty_flags.OnMemoryWrite();

    state.write_offset++;
}

} // namespace Tegra::Engines
