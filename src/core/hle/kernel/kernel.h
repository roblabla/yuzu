// Copyright 2014 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <unordered_map>
#include "core/hle/kernel/object.h"

template <typename T>
class ResultVal;

namespace CoreTiming {
struct EventType;
}

namespace Kernel {

class ClientPort;
class HandleTable;
class Process;
class ResourceLimit;
class Thread;

/// Represents a single instance of the kernel.
class KernelCore {
private:
    using NamedPortTable = std::unordered_map<std::string, SharedPtr<ClientPort>>;

public:
    KernelCore();
    ~KernelCore();

    KernelCore(const KernelCore&) = delete;
    KernelCore& operator=(const KernelCore&) = delete;

    KernelCore(KernelCore&&) = delete;
    KernelCore& operator=(KernelCore&&) = delete;

    /// Resets the kernel to a clean slate for use.
    void Initialize();

    /// Clears all resources in use by the kernel instance.
    void Shutdown();

    /// Retrieves a shared pointer to the system resource limit instance.
    SharedPtr<ResourceLimit> GetSystemResourceLimit() const;

    /// Retrieves a shared pointer to a Thread instance within the thread wakeup handle table.
    SharedPtr<Thread> RetrieveThreadFromWakeupCallbackHandleTable(Handle handle) const;

    /// Adds the given shared pointer to an internal list of active processes.
    void AppendNewProcess(SharedPtr<Process> process);

    /// Makes the given process the new current process.
    void MakeCurrentProcess(Process* process);

    /// Retrieves a pointer to the current process.
    Process* CurrentProcess();

    /// Retrieves a const pointer to the current process.
    const Process* CurrentProcess() const;

    /// Adds a port to the named port table
    void AddNamedPort(std::string name, SharedPtr<ClientPort> port);

    /// Finds a port within the named port table with the given name.
    NamedPortTable::iterator FindNamedPort(const std::string& name);

    /// Finds a port within the named port table with the given name.
    NamedPortTable::const_iterator FindNamedPort(const std::string& name) const;

    /// Determines whether or not the given port is a valid named port.
    bool IsValidNamedPort(NamedPortTable::const_iterator port) const;

private:
    friend class Object;
    friend class Process;
    friend class Thread;

    /// Creates a new object ID, incrementing the internal object ID counter.
    u32 CreateNewObjectID();

    /// Creates a new process ID, incrementing the internal process ID counter;
    u64 CreateNewProcessID();

    /// Creates a new thread ID, incrementing the internal thread ID counter.
    u64 CreateNewThreadID();

    /// Retrieves the event type used for thread wakeup callbacks.
    CoreTiming::EventType* ThreadWakeupCallbackEventType() const;

    /// Provides a reference to the thread wakeup callback handle table.
    Kernel::HandleTable& ThreadWakeupCallbackHandleTable();

    /// Provides a const reference to the thread wakeup callback handle table.
    const Kernel::HandleTable& ThreadWakeupCallbackHandleTable() const;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Kernel
