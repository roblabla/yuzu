// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <boost/container/static_vector.hpp>
#include "common/common_types.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/process_capability.h"
#include "core/hle/kernel/vm_manager.h"
#include "core/hle/kernel/wait_object.h"
#include "core/hle/result.h"

namespace FileSys {
class ProgramMetadata;
}

namespace Kernel {

class KernelCore;
class ResourceLimit;
class Thread;

struct AddressMapping {
    // Address and size must be page-aligned
    VAddr address;
    u64 size;
    bool read_only;
    bool unk_flag;
};

enum class MemoryRegion : u16 {
    APPLICATION = 1,
    SYSTEM = 2,
    BASE = 3,
};

/**
 * Indicates the status of a Process instance.
 *
 * @note These match the values as used by kernel,
 *       so new entries should only be added if RE
 *       shows that a new value has been introduced.
 */
enum class ProcessStatus {
    Created,
    CreatedWithDebuggerAttached,
    Running,
    WaitingForDebuggerToAttach,
    DebuggerAttached,
    Exiting,
    Exited,
    DebugBreak,
};

struct CodeSet final {
    struct Segment {
        std::size_t offset = 0;
        VAddr addr = 0;
        u32 size = 0;
    };

    explicit CodeSet();
    ~CodeSet();

    Segment& CodeSegment() {
        return segments[0];
    }

    const Segment& CodeSegment() const {
        return segments[0];
    }

    Segment& RODataSegment() {
        return segments[1];
    }

    const Segment& RODataSegment() const {
        return segments[1];
    }

    Segment& DataSegment() {
        return segments[2];
    }

    const Segment& DataSegment() const {
        return segments[2];
    }

    std::shared_ptr<std::vector<u8>> memory;

    std::array<Segment, 3> segments;
    VAddr entrypoint = 0;
};

class Process final : public WaitObject {
public:
    enum : u64 {
        /// Lowest allowed process ID for a kernel initial process.
        InitialKIPIDMin = 1,
        /// Highest allowed process ID for a kernel initial process.
        InitialKIPIDMax = 80,

        /// Lowest allowed process ID for a userland process.
        ProcessIDMin = 81,
        /// Highest allowed process ID for a userland process.
        ProcessIDMax = 0xFFFFFFFFFFFFFFFF,
    };

    static constexpr std::size_t RANDOM_ENTROPY_SIZE = 4;

    static SharedPtr<Process> Create(KernelCore& kernel, std::string&& name);

    std::string GetTypeName() const override {
        return "Process";
    }
    std::string GetName() const override {
        return name;
    }

    static const HandleType HANDLE_TYPE = HandleType::Process;
    HandleType GetHandleType() const override {
        return HANDLE_TYPE;
    }

    /// Gets a reference to the process' memory manager.
    Kernel::VMManager& VMManager() {
        return vm_manager;
    }

    /// Gets a const reference to the process' memory manager.
    const Kernel::VMManager& VMManager() const {
        return vm_manager;
    }

    /// Gets a reference to the process' handle table.
    HandleTable& GetHandleTable() {
        return handle_table;
    }

    /// Gets a const reference to the process' handle table.
    const HandleTable& GetHandleTable() const {
        return handle_table;
    }

    /// Gets the current status of the process
    ProcessStatus GetStatus() const {
        return status;
    }

    /// Gets the unique ID that identifies this particular process.
    u64 GetProcessID() const {
        return process_id;
    }

    /// Gets the title ID corresponding to this process.
    u64 GetTitleID() const {
        return program_id;
    }

    /// Gets the resource limit descriptor for this process
    SharedPtr<ResourceLimit> GetResourceLimit() const;

    /// Gets the ideal CPU core ID for this process
    u8 GetIdealCore() const {
        return ideal_core;
    }

    /// Gets the bitmask of allowed cores that this process' threads can run on.
    u64 GetCoreMask() const {
        return capabilities.GetCoreMask();
    }

    /// Gets the bitmask of allowed thread priorities.
    u64 GetPriorityMask() const {
        return capabilities.GetPriorityMask();
    }

    u32 IsVirtualMemoryEnabled() const {
        return is_virtual_address_memory_enabled;
    }

    /// Whether this process is an AArch64 or AArch32 process.
    bool Is64BitProcess() const {
        return is_64bit_process;
    }

    /// Gets the total running time of the process instance in ticks.
    u64 GetCPUTimeTicks() const {
        return total_process_running_time_ticks;
    }

    /// Updates the total running time, adding the given ticks to it.
    void UpdateCPUTimeTicks(u64 ticks) {
        total_process_running_time_ticks += ticks;
    }

    /// Gets 8 bytes of random data for svcGetInfo RandomEntropy
    u64 GetRandomEntropy(std::size_t index) const {
        return random_entropy.at(index);
    }

    /// Clears the signaled state of the process if and only if it's signaled.
    ///
    /// @pre The process must not be already terminated. If this is called on a
    ///      terminated process, then ERR_INVALID_STATE will be returned.
    ///
    /// @pre The process must be in a signaled state. If this is called on a
    ///      process instance that is not signaled, ERR_INVALID_STATE will be
    ///      returned.
    ResultCode ClearSignalState();

    /**
     * Loads process-specifics configuration info with metadata provided
     * by an executable.
     *
     * @param metadata The provided metadata to load process specific info from.
     *
     * @returns RESULT_SUCCESS if all relevant metadata was able to be
     *          loaded and parsed. Otherwise, an error code is returned.
     */
    ResultCode LoadFromMetadata(const FileSys::ProgramMetadata& metadata);

    /**
     * Applies address space changes and launches the process main thread.
     */
    void Run(VAddr entry_point, s32 main_thread_priority, u32 stack_size);

    /**
     * Prepares a process for termination by stopping all of its threads
     * and clearing any other resources.
     */
    void PrepareForTermination();

    void LoadModule(CodeSet module_, VAddr base_addr);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Thread-local storage management

    // Marks the next available region as used and returns the address of the slot.
    VAddr MarkNextAvailableTLSSlotAsUsed(Thread& thread);

    // Frees a used TLS slot identified by the given address
    void FreeTLSSlot(VAddr tls_address);

private:
    explicit Process(KernelCore& kernel);
    ~Process() override;

    /// Checks if the specified thread should wait until this process is available.
    bool ShouldWait(Thread* thread) const override;

    /// Acquires/locks this process for the specified thread if it's available.
    void Acquire(Thread* thread) override;

    /// Changes the process status. If the status is different
    /// from the current process status, then this will trigger
    /// a process signal.
    void ChangeStatus(ProcessStatus new_status);

    /// Memory manager for this process.
    Kernel::VMManager vm_manager;

    /// Current status of the process
    ProcessStatus status;

    /// The ID of this process
    u64 process_id = 0;

    /// Title ID corresponding to the process
    u64 program_id = 0;

    /// Resource limit descriptor for this process
    SharedPtr<ResourceLimit> resource_limit;

    /// The ideal CPU core for this process, threads are scheduled on this core by default.
    u8 ideal_core = 0;
    u32 is_virtual_address_memory_enabled = 0;

    /// The Thread Local Storage area is allocated as processes create threads,
    /// each TLS area is 0x200 bytes, so one page (0x1000) is split up in 8 parts, and each part
    /// holds the TLS for a specific thread. This vector contains which parts are in use for each
    /// page as a bitmask.
    /// This vector will grow as more pages are allocated for new threads.
    std::vector<std::bitset<8>> tls_slots;

    /// Contains the parsed process capability descriptors.
    ProcessCapabilities capabilities;

    /// Whether or not this process is AArch64, or AArch32.
    /// By default, we currently assume this is true, unless otherwise
    /// specified by metadata provided to the process during loading.
    bool is_64bit_process = true;

    /// Whether or not this process is signaled. This occurs
    /// upon the process changing to a different state.
    bool is_signaled = false;

    /// Total running time for the process in ticks.
    u64 total_process_running_time_ticks = 0;

    /// Per-process handle table for storing created object handles in.
    HandleTable handle_table;

    /// Random values for svcGetInfo RandomEntropy
    std::array<u64, RANDOM_ENTROPY_SIZE> random_entropy;

    std::string name;
};

} // namespace Kernel
