// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "core/memory_hook.h"

namespace Memory {

/**
 * Maps an allocated buffer onto a region of the emulated process address space.
 *
 * @param page_table The page table of the emulated process.
 * @param base The address to start mapping at. Must be page-aligned.
 * @param size The amount of bytes to map. Must be page-aligned.
 * @param target Buffer with the memory backing the mapping. Must be of length at least `size`.
 */
void MapMemoryRegion(PageTable& page_table, VAddr base, u64 size, u8* target);

/**
 * Maps a region of the emulated process address space as a IO region.
 * @param page_table The page table of the emulated process.
 * @param base The address to start mapping at. Must be page-aligned.
 * @param size The amount of bytes to map. Must be page-aligned.
 * @param mmio_handler The handler that backs the mapping.
 */
void MapIoRegion(PageTable& page_table, VAddr base, u64 size, MemoryHookPointer mmio_handler);

void UnmapRegion(PageTable& page_table, VAddr base, u64 size);

void AddDebugHook(PageTable& page_table, VAddr base, u64 size, MemoryHookPointer hook);
void RemoveDebugHook(PageTable& page_table, VAddr base, u64 size, MemoryHookPointer hook);

} // namespace Memory
