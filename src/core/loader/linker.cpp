// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/loader/linker.h"
#include "core/memory.h"

namespace Loader {

enum class RelocationType : u32 { ABS64 = 257, GLOB_DAT = 1025, JUMP_SLOT = 1026, RELATIVE = 1027 };

enum DynamicType : u32 {
    DT_NULL = 0,
    DT_PLTRELSZ = 2,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_STRSZ = 10,
    DT_JMPREL = 23,
};

struct Elf64_Rela {
    u64_le offset;
    RelocationType type;
    u32_le symbol;
    s64_le addend;
};
static_assert(sizeof(Elf64_Rela) == 0x18, "Elf64_Rela has incorrect size.");

struct Elf64_Dyn {
    u64_le tag;
    u64_le value;
};
static_assert(sizeof(Elf64_Dyn) == 0x10, "Elf64_Dyn has incorrect size.");

struct Elf64_Sym {
    u32_le name;
    INSERT_PADDING_BYTES(0x2);
    u16_le shndx;
    u64_le value;
    u64_le size;
};
static_assert(sizeof(Elf64_Sym) == 0x18, "Elf64_Sym has incorrect size.");

void Linker::WriteRelocations(std::vector<u8>& program_image, const std::vector<Symbol>& symbols,
                              u64 relocation_offset, u64 size, VAddr load_base) {
    for (u64 i = 0; i < size; i += sizeof(Elf64_Rela)) {
        Elf64_Rela rela;
        std::memcpy(&rela, &program_image[relocation_offset + i], sizeof(Elf64_Rela));

        const Symbol& symbol = symbols[rela.symbol];
        switch (rela.type) {
        case RelocationType::RELATIVE: {
            const u64 value = load_base + rela.addend;
            if (!symbol.name.empty()) {
                exports[symbol.name] = value;
            }
            std::memcpy(&program_image[rela.offset], &value, sizeof(u64));
            break;
        }
        case RelocationType::JUMP_SLOT:
        case RelocationType::GLOB_DAT:
            if (!symbol.value) {
                imports[symbol.name] = {rela.offset + load_base, 0};
            } else {
                exports[symbol.name] = symbol.value;
                std::memcpy(&program_image[rela.offset], &symbol.value, sizeof(u64));
            }
            break;
        case RelocationType::ABS64:
            if (!symbol.value) {
                imports[symbol.name] = {rela.offset + load_base, rela.addend};
            } else {
                const u64 value = symbol.value + rela.addend;
                exports[symbol.name] = value;
                std::memcpy(&program_image[rela.offset], &value, sizeof(u64));
            }
            break;
        default:
            LOG_CRITICAL(Loader, "Unknown relocation type: {}", static_cast<int>(rela.type));
            break;
        }
    }
}

void Linker::Relocate(std::vector<u8>& program_image, u32 dynamic_section_offset, VAddr load_base) {
    std::map<u64, u64> dynamic;
    while (dynamic_section_offset < program_image.size()) {
        Elf64_Dyn dyn;
        std::memcpy(&dyn, &program_image[dynamic_section_offset], sizeof(Elf64_Dyn));
        dynamic_section_offset += sizeof(Elf64_Dyn);

        if (dyn.tag == DT_NULL) {
            break;
        }
        dynamic[dyn.tag] = dyn.value;
    }

    u64 offset = dynamic[DT_SYMTAB];
    std::vector<Symbol> symbols;
    while (offset < program_image.size()) {
        Elf64_Sym sym;
        std::memcpy(&sym, &program_image[offset], sizeof(Elf64_Sym));
        offset += sizeof(Elf64_Sym);

        if (sym.name >= dynamic[DT_STRSZ]) {
            break;
        }

        std::string name = reinterpret_cast<char*>(&program_image[dynamic[DT_STRTAB] + sym.name]);
        if (sym.value) {
            exports[name] = load_base + sym.value;
            symbols.emplace_back(std::move(name), load_base + sym.value);
        } else {
            symbols.emplace_back(std::move(name), 0);
        }
    }

    if (dynamic.find(DT_RELA) != dynamic.end()) {
        WriteRelocations(program_image, symbols, dynamic[DT_RELA], dynamic[DT_RELASZ], load_base);
    }

    if (dynamic.find(DT_JMPREL) != dynamic.end()) {
        WriteRelocations(program_image, symbols, dynamic[DT_JMPREL], dynamic[DT_PLTRELSZ],
                         load_base);
    }
}

void Linker::ResolveImports() {
    // Resolve imports
    for (const auto& import : imports) {
        const auto& search = exports.find(import.first);
        if (search != exports.end()) {
            Memory::Write64(import.second.ea, search->second + import.second.addend);
        } else {
            LOG_ERROR(Loader, "Unresolved import: {}", import.first);
        }
    }
}

} // namespace Loader
