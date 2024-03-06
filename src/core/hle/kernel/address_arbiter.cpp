// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <vector>

#include "common/assert.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/core_cpu.h"
#include "core/hle/kernel/errors.h"
#include "core/hle/kernel/object.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/scheduler.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/result.h"
#include "core/memory.h"

namespace Kernel {
namespace AddressArbiter {

// Performs actual address waiting logic.
static ResultCode WaitForAddress(VAddr address, s64 timeout) {
    SharedPtr<Thread> current_thread = GetCurrentThread();
    current_thread->SetArbiterWaitAddress(address);
    current_thread->SetStatus(ThreadStatus::WaitArb);
    current_thread->InvalidateWakeupCallback();

    current_thread->WakeAfterDelay(timeout);

    Core::System::GetInstance().CpuCore(current_thread->GetProcessorID()).PrepareReschedule();
    return RESULT_TIMEOUT;
}

// Gets the threads waiting on an address.
static std::vector<SharedPtr<Thread>> GetThreadsWaitingOnAddress(VAddr address) {
    const auto RetrieveWaitingThreads = [](std::size_t core_index,
                                           std::vector<SharedPtr<Thread>>& waiting_threads,
                                           VAddr arb_addr) {
        const auto& scheduler = Core::System::GetInstance().Scheduler(core_index);
        const auto& thread_list = scheduler.GetThreadList();

        for (const auto& thread : thread_list) {
            if (thread->GetArbiterWaitAddress() == arb_addr)
                waiting_threads.push_back(thread);
        }
    };

    // Retrieve all threads that are waiting for this address.
    std::vector<SharedPtr<Thread>> threads;
    RetrieveWaitingThreads(0, threads, address);
    RetrieveWaitingThreads(1, threads, address);
    RetrieveWaitingThreads(2, threads, address);
    RetrieveWaitingThreads(3, threads, address);

    // Sort them by priority, such that the highest priority ones come first.
    std::sort(threads.begin(), threads.end(),
              [](const SharedPtr<Thread>& lhs, const SharedPtr<Thread>& rhs) {
                  return lhs->GetPriority() < rhs->GetPriority();
              });

    return threads;
}

// Wake up num_to_wake (or all) threads in a vector.
static void WakeThreads(std::vector<SharedPtr<Thread>>& waiting_threads, s32 num_to_wake) {
    // Only process up to 'target' threads, unless 'target' is <= 0, in which case process
    // them all.
    std::size_t last = waiting_threads.size();
    if (num_to_wake > 0)
        last = num_to_wake;

    // Signal the waiting threads.
    for (std::size_t i = 0; i < last; i++) {
        ASSERT(waiting_threads[i]->GetStatus() == ThreadStatus::WaitArb);
        waiting_threads[i]->SetWaitSynchronizationResult(RESULT_SUCCESS);
        waiting_threads[i]->SetArbiterWaitAddress(0);
        waiting_threads[i]->ResumeFromWait();
    }
}

// Signals an address being waited on.
ResultCode SignalToAddress(VAddr address, s32 num_to_wake) {
    std::vector<SharedPtr<Thread>> waiting_threads = GetThreadsWaitingOnAddress(address);

    WakeThreads(waiting_threads, num_to_wake);
    return RESULT_SUCCESS;
}

// Signals an address being waited on and increments its value if equal to the value argument.
ResultCode IncrementAndSignalToAddressIfEqual(VAddr address, s32 value, s32 num_to_wake) {
    // Ensure that we can write to the address.
    if (!Memory::IsValidVirtualAddress(address)) {
        return ERR_INVALID_ADDRESS_STATE;
    }

    if (static_cast<s32>(Memory::Read32(address)) == value) {
        Memory::Write32(address, static_cast<u32>(value + 1));
    } else {
        return ERR_INVALID_STATE;
    }

    return SignalToAddress(address, num_to_wake);
}

// Signals an address being waited on and modifies its value based on waiting thread count if equal
// to the value argument.
ResultCode ModifyByWaitingCountAndSignalToAddressIfEqual(VAddr address, s32 value,
                                                         s32 num_to_wake) {
    // Ensure that we can write to the address.
    if (!Memory::IsValidVirtualAddress(address)) {
        return ERR_INVALID_ADDRESS_STATE;
    }

    // Get threads waiting on the address.
    std::vector<SharedPtr<Thread>> waiting_threads = GetThreadsWaitingOnAddress(address);

    // Determine the modified value depending on the waiting count.
    s32 updated_value;
    if (waiting_threads.empty()) {
        updated_value = value - 1;
    } else if (num_to_wake <= 0 || waiting_threads.size() <= static_cast<u32>(num_to_wake)) {
        updated_value = value + 1;
    } else {
        updated_value = value;
    }

    if (static_cast<s32>(Memory::Read32(address)) == value) {
        Memory::Write32(address, static_cast<u32>(updated_value));
    } else {
        return ERR_INVALID_STATE;
    }

    WakeThreads(waiting_threads, num_to_wake);
    return RESULT_SUCCESS;
}

// Waits on an address if the value passed is less than the argument value, optionally decrementing.
ResultCode WaitForAddressIfLessThan(VAddr address, s32 value, s64 timeout, bool should_decrement) {
    // Ensure that we can read the address.
    if (!Memory::IsValidVirtualAddress(address)) {
        return ERR_INVALID_ADDRESS_STATE;
    }

    s32 cur_value = static_cast<s32>(Memory::Read32(address));
    if (cur_value < value) {
        if (should_decrement) {
            Memory::Write32(address, static_cast<u32>(cur_value - 1));
        }
    } else {
        return ERR_INVALID_STATE;
    }
    // Short-circuit without rescheduling, if timeout is zero.
    if (timeout == 0) {
        return RESULT_TIMEOUT;
    }

    return WaitForAddress(address, timeout);
}

// Waits on an address if the value passed is equal to the argument value.
ResultCode WaitForAddressIfEqual(VAddr address, s32 value, s64 timeout) {
    // Ensure that we can read the address.
    if (!Memory::IsValidVirtualAddress(address)) {
        return ERR_INVALID_ADDRESS_STATE;
    }
    // Only wait for the address if equal.
    if (static_cast<s32>(Memory::Read32(address)) != value) {
        return ERR_INVALID_STATE;
    }
    // Short-circuit without rescheduling, if timeout is zero.
    if (timeout == 0) {
        return RESULT_TIMEOUT;
    }

    return WaitForAddress(address, timeout);
}
} // namespace AddressArbiter
} // namespace Kernel
