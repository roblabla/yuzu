// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/hle/kernel/object.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"

namespace Kernel {

WaitObject::WaitObject(KernelCore& kernel) : Object{kernel} {}
WaitObject::~WaitObject() = default;

void WaitObject::AddWaitingThread(SharedPtr<Thread> thread) {
    auto itr = std::find(waiting_threads.begin(), waiting_threads.end(), thread);
    if (itr == waiting_threads.end())
        waiting_threads.push_back(std::move(thread));
}

void WaitObject::RemoveWaitingThread(Thread* thread) {
    auto itr = std::find(waiting_threads.begin(), waiting_threads.end(), thread);
    // If a thread passed multiple handles to the same object,
    // the kernel might attempt to remove the thread from the object's
    // waiting threads list multiple times.
    if (itr != waiting_threads.end())
        waiting_threads.erase(itr);
}

SharedPtr<Thread> WaitObject::GetHighestPriorityReadyThread() {
    Thread* candidate = nullptr;
    u32 candidate_priority = THREADPRIO_LOWEST + 1;

    for (const auto& thread : waiting_threads) {
        const ThreadStatus thread_status = thread->GetStatus();

        // The list of waiting threads must not contain threads that are not waiting to be awakened.
        ASSERT_MSG(thread_status == ThreadStatus::WaitSynchAny ||
                       thread_status == ThreadStatus::WaitSynchAll ||
                       thread_status == ThreadStatus::WaitHLEEvent,
                   "Inconsistent thread statuses in waiting_threads");

        if (thread->GetPriority() >= candidate_priority)
            continue;

        if (ShouldWait(thread.get()))
            continue;

        // A thread is ready to run if it's either in ThreadStatus::WaitSynchAny or
        // in ThreadStatus::WaitSynchAll and the rest of the objects it is waiting on are ready.
        bool ready_to_run = true;
        if (thread_status == ThreadStatus::WaitSynchAll) {
            ready_to_run = thread->AllWaitObjectsReady();
        }

        if (ready_to_run) {
            candidate = thread.get();
            candidate_priority = thread->GetPriority();
        }
    }

    return candidate;
}

void WaitObject::WakeupWaitingThread(SharedPtr<Thread> thread) {
    ASSERT(!ShouldWait(thread.get()));

    if (!thread)
        return;

    if (!thread->IsSleepingOnWaitAll()) {
        Acquire(thread.get());
    } else {
        for (const auto& object : thread->GetWaitObjects()) {
            ASSERT(!object->ShouldWait(thread.get()));
            object->Acquire(thread.get());
        }
    }

    const std::size_t index = thread->GetWaitObjectIndex(this);

    for (const auto& object : thread->GetWaitObjects())
        object->RemoveWaitingThread(thread.get());
    thread->ClearWaitObjects();

    thread->CancelWakeupTimer();

    bool resume = true;

    if (thread->HasWakeupCallback())
        resume = thread->InvokeWakeupCallback(ThreadWakeupReason::Signal, thread, this, index);

    if (resume)
        thread->ResumeFromWait();
}

void WaitObject::WakeupAllWaitingThreads() {
    while (auto thread = GetHighestPriorityReadyThread()) {
        WakeupWaitingThread(thread);
    }
}

const std::vector<SharedPtr<Thread>>& WaitObject::GetWaitingThreads() const {
    return waiting_threads;
}

} // namespace Kernel
