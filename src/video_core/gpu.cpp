// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/core_timing.h"
#include "core/memory.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_compute.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/gpu.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra {

u32 FramebufferConfig::BytesPerPixel(PixelFormat format) {
    switch (format) {
    case PixelFormat::ABGR8:
        return 4;
    default:
        return 4;
    }

    UNREACHABLE();
}

GPU::GPU(VideoCore::RasterizerInterface& rasterizer) {
    memory_manager = std::make_unique<Tegra::MemoryManager>();
    dma_pusher = std::make_unique<Tegra::DmaPusher>(*this);
    maxwell_3d = std::make_unique<Engines::Maxwell3D>(rasterizer, *memory_manager);
    fermi_2d = std::make_unique<Engines::Fermi2D>(rasterizer, *memory_manager);
    maxwell_compute = std::make_unique<Engines::MaxwellCompute>();
    maxwell_dma = std::make_unique<Engines::MaxwellDMA>(rasterizer, *memory_manager);
    kepler_memory = std::make_unique<Engines::KeplerMemory>(rasterizer, *memory_manager);
}

GPU::~GPU() = default;

Engines::Maxwell3D& GPU::Maxwell3D() {
    return *maxwell_3d;
}

const Engines::Maxwell3D& GPU::Maxwell3D() const {
    return *maxwell_3d;
}

MemoryManager& GPU::MemoryManager() {
    return *memory_manager;
}

const MemoryManager& GPU::MemoryManager() const {
    return *memory_manager;
}

DmaPusher& GPU::DmaPusher() {
    return *dma_pusher;
}

const DmaPusher& GPU::DmaPusher() const {
    return *dma_pusher;
}

u32 RenderTargetBytesPerPixel(RenderTargetFormat format) {
    ASSERT(format != RenderTargetFormat::NONE);

    switch (format) {
    case RenderTargetFormat::RGBA32_FLOAT:
    case RenderTargetFormat::RGBA32_UINT:
        return 16;
    case RenderTargetFormat::RGBA16_UINT:
    case RenderTargetFormat::RGBA16_UNORM:
    case RenderTargetFormat::RGBA16_FLOAT:
    case RenderTargetFormat::RG32_FLOAT:
    case RenderTargetFormat::RG32_UINT:
        return 8;
    case RenderTargetFormat::RGBA8_UNORM:
    case RenderTargetFormat::RGBA8_SNORM:
    case RenderTargetFormat::RGBA8_SRGB:
    case RenderTargetFormat::RGBA8_UINT:
    case RenderTargetFormat::RGB10_A2_UNORM:
    case RenderTargetFormat::BGRA8_UNORM:
    case RenderTargetFormat::BGRA8_SRGB:
    case RenderTargetFormat::RG16_UNORM:
    case RenderTargetFormat::RG16_SNORM:
    case RenderTargetFormat::RG16_UINT:
    case RenderTargetFormat::RG16_SINT:
    case RenderTargetFormat::RG16_FLOAT:
    case RenderTargetFormat::R32_FLOAT:
    case RenderTargetFormat::R11G11B10_FLOAT:
    case RenderTargetFormat::R32_UINT:
        return 4;
    case RenderTargetFormat::R16_UNORM:
    case RenderTargetFormat::R16_SNORM:
    case RenderTargetFormat::R16_UINT:
    case RenderTargetFormat::R16_SINT:
    case RenderTargetFormat::R16_FLOAT:
    case RenderTargetFormat::RG8_UNORM:
    case RenderTargetFormat::RG8_SNORM:
        return 2;
    case RenderTargetFormat::R8_UNORM:
    case RenderTargetFormat::R8_UINT:
        return 1;
    default:
        UNIMPLEMENTED_MSG("Unimplemented render target format {}", static_cast<u32>(format));
        return 1;
    }
}

u32 DepthFormatBytesPerPixel(DepthFormat format) {
    switch (format) {
    case DepthFormat::Z32_S8_X24_FLOAT:
        return 8;
    case DepthFormat::Z32_FLOAT:
    case DepthFormat::S8_Z24_UNORM:
    case DepthFormat::Z24_X8_UNORM:
    case DepthFormat::Z24_S8_UNORM:
    case DepthFormat::Z24_C8_UNORM:
        return 4;
    case DepthFormat::Z16_UNORM:
        return 2;
    default:
        UNIMPLEMENTED_MSG("Unimplemented Depth format {}", static_cast<u32>(format));
        return 1;
    }
}

// Note that, traditionally, methods are treated as 4-byte addressable locations, and hence
// their numbers are written down multiplied by 4 in Docs. Here we are not multiply by 4.
// So the values you see in docs might be multiplied by 4.
enum class BufferMethods {
    BindObject = 0x0,
    Nop = 0x2,
    SemaphoreAddressHigh = 0x4,
    SemaphoreAddressLow = 0x5,
    SemaphoreSequence = 0x6,
    SemaphoreTrigger = 0x7,
    NotifyIntr = 0x8,
    WrcacheFlush = 0x9,
    Unk28 = 0xA,
    Unk2c = 0xB,
    RefCnt = 0x14,
    SemaphoreAcquire = 0x1A,
    SemaphoreRelease = 0x1B,
    Unk70 = 0x1C,
    Unk74 = 0x1D,
    Unk78 = 0x1E,
    Unk7c = 0x1F,
    Yield = 0x20,
    NonPullerMethods = 0x40,
};

enum class GpuSemaphoreOperation {
    AcquireEqual = 0x1,
    WriteLong = 0x2,
    AcquireGequal = 0x4,
    AcquireMask = 0x8,
};

void GPU::CallMethod(const MethodCall& method_call) {
    LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method_call.method,
              method_call.subchannel);

    ASSERT(method_call.subchannel < bound_engines.size());

    if (ExecuteMethodOnEngine(method_call)) {
        CallEngineMethod(method_call);
    } else {
        CallPullerMethod(method_call);
    }
}

bool GPU::ExecuteMethodOnEngine(const MethodCall& method_call) {
    const auto method = static_cast<BufferMethods>(method_call.method);
    return method >= BufferMethods::NonPullerMethods;
}

void GPU::CallPullerMethod(const MethodCall& method_call) {
    regs.reg_array[method_call.method] = method_call.argument;
    const auto method = static_cast<BufferMethods>(method_call.method);

    switch (method) {
    case BufferMethods::BindObject: {
        ProcessBindMethod(method_call);
        break;
    }
    case BufferMethods::Nop:
    case BufferMethods::SemaphoreAddressHigh:
    case BufferMethods::SemaphoreAddressLow:
    case BufferMethods::SemaphoreSequence:
    case BufferMethods::RefCnt:
        break;
    case BufferMethods::SemaphoreTrigger: {
        ProcessSemaphoreTriggerMethod();
        break;
    }
    case BufferMethods::NotifyIntr: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method NotifyIntr not implemented");
        break;
    }
    case BufferMethods::WrcacheFlush: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method WrcacheFlush not implemented");
        break;
    }
    case BufferMethods::Unk28: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method Unk28 not implemented");
        break;
    }
    case BufferMethods::Unk2c: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method Unk2c not implemented");
        break;
    }
    case BufferMethods::SemaphoreAcquire: {
        ProcessSemaphoreAcquire();
        break;
    }
    case BufferMethods::SemaphoreRelease: {
        ProcessSemaphoreRelease();
        break;
    }
    case BufferMethods::Yield: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method Yield not implemented");
        break;
    }
    default:
        LOG_ERROR(HW_GPU, "Special puller engine method {:X} not implemented",
                  static_cast<u32>(method));
        break;
    }
}

void GPU::CallEngineMethod(const MethodCall& method_call) {
    const EngineID engine = bound_engines[method_call.subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        fermi_2d->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_B:
        maxwell_3d->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_COMPUTE_B:
        maxwell_compute->CallMethod(method_call);
        break;
    case EngineID::MAXWELL_DMA_COPY_A:
        maxwell_dma->CallMethod(method_call);
        break;
    case EngineID::KEPLER_INLINE_TO_MEMORY_B:
        kepler_memory->CallMethod(method_call);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented engine");
    }
}

void GPU::ProcessBindMethod(const MethodCall& method_call) {
    // Bind the current subchannel to the desired engine id.
    LOG_DEBUG(HW_GPU, "Binding subchannel {} to engine {}", method_call.subchannel,
              method_call.argument);
    bound_engines[method_call.subchannel] = static_cast<EngineID>(method_call.argument);
}

void GPU::ProcessSemaphoreTriggerMethod() {
    const auto semaphoreOperationMask = 0xF;
    const auto op =
        static_cast<GpuSemaphoreOperation>(regs.semaphore_trigger & semaphoreOperationMask);
    if (op == GpuSemaphoreOperation::WriteLong) {
        auto address = memory_manager->GpuToCpuAddress(regs.smaphore_address.SmaphoreAddress());
        struct Block {
            u32 sequence;
            u32 zeros = 0;
            u64 timestamp;
        };

        Block block{};
        block.sequence = regs.semaphore_sequence;
        // TODO(Kmather73): Generate a real GPU timestamp and write it here instead of
        // CoreTiming
        block.timestamp = CoreTiming::GetTicks();
        Memory::WriteBlock(*address, &block, sizeof(block));
    } else {
        const auto address =
            memory_manager->GpuToCpuAddress(regs.smaphore_address.SmaphoreAddress());
        const u32 word = Memory::Read32(*address);
        if ((op == GpuSemaphoreOperation::AcquireEqual && word == regs.semaphore_sequence) ||
            (op == GpuSemaphoreOperation::AcquireGequal &&
             static_cast<s32>(word - regs.semaphore_sequence) > 0) ||
            (op == GpuSemaphoreOperation::AcquireMask && (word & regs.semaphore_sequence))) {
            // Nothing to do in this case
        } else {
            regs.acquire_source = true;
            regs.acquire_value = regs.semaphore_sequence;
            if (op == GpuSemaphoreOperation::AcquireEqual) {
                regs.acquire_active = true;
                regs.acquire_mode = false;
            } else if (op == GpuSemaphoreOperation::AcquireGequal) {
                regs.acquire_active = true;
                regs.acquire_mode = true;
            } else if (op == GpuSemaphoreOperation::AcquireMask) {
                // TODO(kemathe) The acquire mask operation waits for a value that, ANDed with
                // semaphore_sequence, gives a non-0 result
                LOG_ERROR(HW_GPU, "Invalid semaphore operation AcquireMask not implemented");
            } else {
                LOG_ERROR(HW_GPU, "Invalid semaphore operation");
            }
        }
    }
}

void GPU::ProcessSemaphoreRelease() {
    const auto address = memory_manager->GpuToCpuAddress(regs.smaphore_address.SmaphoreAddress());
    Memory::Write32(*address, regs.semaphore_release);
}

void GPU::ProcessSemaphoreAcquire() {
    const auto address = memory_manager->GpuToCpuAddress(regs.smaphore_address.SmaphoreAddress());
    const u32 word = Memory::Read32(*address);
    const auto value = regs.semaphore_acquire;
    if (word != value) {
        regs.acquire_active = true;
        regs.acquire_value = value;
        // TODO(kemathe73) figure out how to do the acquire_timeout
        regs.acquire_mode = false;
        regs.acquire_source = false;
    }
}

} // namespace Tegra
