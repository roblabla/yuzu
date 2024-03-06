// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace FileSys {

enum class SaveDataSpaceId : u8 {
    NandSystem = 0,
    NandUser = 1,
    SdCardSystem = 2,
    TemporaryStorage = 3,
    SdCardUser = 4,
    ProperSystem = 100,
};

enum class SaveDataType : u8 {
    SystemSaveData = 0,
    SaveData = 1,
    BcatDeliveryCacheStorage = 2,
    DeviceSaveData = 3,
    TemporaryStorage = 4,
    CacheStorage = 5,
};

struct SaveDataDescriptor {
    u64_le title_id;
    u128 user_id;
    u64_le save_id;
    SaveDataType type;
    INSERT_PADDING_BYTES(7);
    u64_le zero_1;
    u64_le zero_2;
    u64_le zero_3;

    std::string DebugInfo() const;
};
static_assert(sizeof(SaveDataDescriptor) == 0x40, "SaveDataDescriptor has incorrect size.");

struct SaveDataSize {
    u64 normal;
    u64 journal;
};

/// File system interface to the SaveData archive
class SaveDataFactory {
public:
    explicit SaveDataFactory(VirtualDir dir);
    ~SaveDataFactory();

    ResultVal<VirtualDir> Open(SaveDataSpaceId space, SaveDataDescriptor meta);

    VirtualDir GetSaveDataSpaceDirectory(SaveDataSpaceId space) const;

    static std::string GetSaveDataSpaceIdPath(SaveDataSpaceId space);
    static std::string GetFullPath(SaveDataSpaceId space, SaveDataType type, u64 title_id,
                                   u128 user_id, u64 save_id);

    SaveDataSize ReadSaveDataSize(SaveDataType type, u64 title_id, u128 user_id) const;
    void WriteSaveDataSize(SaveDataType type, u64 title_id, u128 user_id, SaveDataSize new_value);

private:
    VirtualDir dir;
};

} // namespace FileSys
