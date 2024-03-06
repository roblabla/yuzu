// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <queue>
#include "common/swap.h"
#include "core/hle/kernel/object.h"
#include "core/hle/kernel/writable_event.h"

union ResultCode;

namespace Service::AM {

class IStorage;

namespace Applets {

class AppletDataBroker final {
public:
    AppletDataBroker();
    ~AppletDataBroker();

    std::unique_ptr<IStorage> PopNormalDataToGame();
    std::unique_ptr<IStorage> PopNormalDataToApplet();

    std::unique_ptr<IStorage> PopInteractiveDataToGame();
    std::unique_ptr<IStorage> PopInteractiveDataToApplet();

    void PushNormalDataFromGame(IStorage storage);
    void PushNormalDataFromApplet(IStorage storage);

    void PushInteractiveDataFromGame(IStorage storage);
    void PushInteractiveDataFromApplet(IStorage storage);

    void SignalStateChanged() const;

    Kernel::SharedPtr<Kernel::ReadableEvent> GetNormalDataEvent() const;
    Kernel::SharedPtr<Kernel::ReadableEvent> GetInteractiveDataEvent() const;
    Kernel::SharedPtr<Kernel::ReadableEvent> GetStateChangedEvent() const;

private:
    // Queues are named from applet's perspective

    // PopNormalDataToApplet and PushNormalDataFromGame
    std::queue<std::unique_ptr<IStorage>> in_channel;

    // PopNormalDataToGame and PushNormalDataFromApplet
    std::queue<std::unique_ptr<IStorage>> out_channel;

    // PopInteractiveDataToApplet and PushInteractiveDataFromGame
    std::queue<std::unique_ptr<IStorage>> in_interactive_channel;

    // PopInteractiveDataToGame and PushInteractiveDataFromApplet
    std::queue<std::unique_ptr<IStorage>> out_interactive_channel;

    Kernel::EventPair state_changed_event;

    // Signaled on PushNormalDataFromApplet
    Kernel::EventPair pop_out_data_event;

    // Signaled on PushInteractiveDataFromApplet
    Kernel::EventPair pop_interactive_out_data_event;
};

class Applet {
public:
    Applet();
    virtual ~Applet();

    virtual void Initialize();

    virtual bool TransactionComplete() const = 0;
    virtual ResultCode GetStatus() const = 0;
    virtual void ExecuteInteractive() = 0;
    virtual void Execute() = 0;

    bool IsInitialized() const {
        return initialized;
    }

    AppletDataBroker& GetBroker() {
        return broker;
    }

    const AppletDataBroker& GetBroker() const {
        return broker;
    }

protected:
    struct CommonArguments {
        u32_le arguments_version;
        u32_le size;
        u32_le library_version;
        u32_le theme_color;
        u8 play_startup_sound;
        u64_le system_tick;
    };
    static_assert(sizeof(CommonArguments) == 0x20, "CommonArguments has incorrect size.");

    CommonArguments common_args{};
    AppletDataBroker broker;
    bool initialized = false;
};

} // namespace Applets
} // namespace Service::AM
