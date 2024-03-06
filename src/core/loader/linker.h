// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <string>
#include "common/common_types.h"

namespace Loader {

class Linker {
protected:
    struct Symbol {
        Symbol(std::string&& name, u64 value) : name(std::move(name)), value(value) {}
        std::string name;
        u64 value;
    };

    struct Import {
        VAddr ea;
        s64 addend;
    };

    void WriteRelocations(std::vector<u8>& program_image, const std::vector<Symbol>& symbols,
                          u64 relocation_offset, u64 size, VAddr load_base);
    void Relocate(std::vector<u8>& program_image, u32 dynamic_section_offset, VAddr load_base);

    void ResolveImports();

    std::map<std::string, Import> imports;
    std::map<std::string, VAddr> exports;
};

} // namespace Loader
