// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include "common/common_types.h"
#include "core/file_sys/patch_manager.h"
#include "core/loader/linker.h"
#include "core/loader/loader.h"

namespace Kernel {
class Process;
}

namespace Loader {

constexpr u64 NSO_ARGUMENT_DATA_ALLOCATION_SIZE = 0x9000;

struct NSOArgumentHeader {
    u32_le allocated_size;
    u32_le actual_size;
    INSERT_PADDING_BYTES(0x18);
};
static_assert(sizeof(NSOArgumentHeader) == 0x20, "NSOArgumentHeader has incorrect size.");

/// Loads an NSO file
class AppLoader_NSO final : public AppLoader, Linker {
public:
    explicit AppLoader_NSO(FileSys::VirtualFile file);

    /**
     * Returns the type of the file
     * @param file std::shared_ptr<VfsFile> open file
     * @return FileType found, or FileType::Error if this loader doesn't know it
     */
    static FileType IdentifyType(const FileSys::VirtualFile& file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    static std::optional<VAddr> LoadModule(Kernel::Process& process, const FileSys::VfsFile& file,
                                           VAddr load_base, bool should_pass_arguments,
                                           std::optional<FileSys::PatchManager> pm = {});

    ResultStatus Load(Kernel::Process& process) override;
};

} // namespace Loader
