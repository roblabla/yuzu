// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/file_sys/vfs.h"
#include "core/hle/result.h"

namespace FileSys {

class RegisteredCache;

/// File system interface to the SDCard archive
class SDMCFactory {
public:
    explicit SDMCFactory(VirtualDir dir);
    ~SDMCFactory();

    ResultVal<VirtualDir> Open();
    RegisteredCache* GetSDMCContents() const;

private:
    VirtualDir dir;

    std::unique_ptr<RegisteredCache> contents;
};

} // namespace FileSys
