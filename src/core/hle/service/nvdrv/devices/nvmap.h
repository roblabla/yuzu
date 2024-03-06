// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/service/nvdrv/devices/nvdevice.h"

namespace Service::Nvidia::Devices {

class nvmap final : public nvdevice {
public:
    nvmap();
    ~nvmap() override;

    /// Returns the allocated address of an nvmap object given its handle.
    VAddr GetObjectAddress(u32 handle) const;

    u32 ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) override;

    /// Represents an nvmap object.
    struct Object {
        enum class Status { Created, Allocated };
        u32 id;
        u32 size;
        u32 flags;
        u32 align;
        u8 kind;
        VAddr addr;
        Status status;
        u32 refcount;
    };

    std::shared_ptr<Object> GetObject(u32 handle) const {
        auto itr = handles.find(handle);
        if (itr != handles.end()) {
            return itr->second;
        }
        return {};
    }

private:
    /// Id to use for the next handle that is created.
    u32 next_handle = 1;

    /// Id to use for the next object that is created.
    u32 next_id = 1;

    /// Mapping of currently allocated handles to the objects they represent.
    std::unordered_map<u32, std::shared_ptr<Object>> handles;

    enum class IoctlCommand : u32 {
        Create = 0xC0080101,
        FromId = 0xC0080103,
        Alloc = 0xC0200104,
        Free = 0xC0180105,
        Param = 0xC00C0109,
        GetId = 0xC008010E,
    };
    struct IocCreateParams {
        // Input
        u32_le size;
        // Output
        u32_le handle;
    };
    static_assert(sizeof(IocCreateParams) == 8, "IocCreateParams has wrong size");

    struct IocFromIdParams {
        // Input
        u32_le id;
        // Output
        u32_le handle;
    };
    static_assert(sizeof(IocFromIdParams) == 8, "IocFromIdParams has wrong size");

    struct IocAllocParams {
        // Input
        u32_le handle;
        u32_le heap_mask;
        u32_le flags;
        u32_le align;
        u8 kind;
        INSERT_PADDING_BYTES(7);
        u64_le addr;
    };
    static_assert(sizeof(IocAllocParams) == 32, "IocAllocParams has wrong size");

    struct IocFreeParams {
        u32_le handle;
        INSERT_PADDING_BYTES(4);
        u64_le address;
        u32_le size;
        u32_le flags;
    };
    static_assert(sizeof(IocFreeParams) == 24, "IocFreeParams has wrong size");

    struct IocParamParams {
        // Input
        u32_le handle;
        u32_le param;
        // Output
        u32_le result;
    };
    static_assert(sizeof(IocParamParams) == 12, "IocParamParams has wrong size");

    struct IocGetIdParams {
        // Output
        u32_le id;
        // Input
        u32_le handle;
    };
    static_assert(sizeof(IocGetIdParams) == 8, "IocGetIdParams has wrong size");

    u32 IocCreate(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocAlloc(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocGetId(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocFromId(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocParam(const std::vector<u8>& input, std::vector<u8>& output);
    u32 IocFree(const std::vector<u8>& input, std::vector<u8>& output);
};

} // namespace Service::Nvidia::Devices
