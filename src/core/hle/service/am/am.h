// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <queue>
#include "core/hle/kernel/writable_event.h"
#include "core/hle/service/service.h"

namespace Service {
namespace NVFlinger {
class NVFlinger;
}

namespace AM {

enum SystemLanguage {
    Japanese = 0,
    English = 1, // en-US
    French = 2,
    German = 3,
    Italian = 4,
    Spanish = 5,
    Chinese = 6,
    Korean = 7,
    Dutch = 8,
    Portuguese = 9,
    Russian = 10,
    Taiwanese = 11,
    BritishEnglish = 12, // en-GB
    CanadianFrench = 13,
    LatinAmericanSpanish = 14, // es-419
    // 4.0.0+
    SimplifiedChinese = 15,
    TraditionalChinese = 16,
};

class AppletMessageQueue {
public:
    enum class AppletMessage : u32 {
        NoMessage = 0,
        FocusStateChanged = 15,
        OperationModeChanged = 30,
        PerformanceModeChanged = 31,
    };

    AppletMessageQueue();
    ~AppletMessageQueue();

    const Kernel::SharedPtr<Kernel::ReadableEvent>& GetMesssageRecieveEvent() const;
    const Kernel::SharedPtr<Kernel::ReadableEvent>& GetOperationModeChangedEvent() const;
    void PushMessage(AppletMessage msg);
    AppletMessage PopMessage();
    std::size_t GetMessageCount() const;
    void OperationModeChanged();

private:
    std::queue<AppletMessage> messages;
    Kernel::EventPair on_new_message;
    Kernel::EventPair on_operation_mode_changed;
};

class IWindowController final : public ServiceFramework<IWindowController> {
public:
    IWindowController();
    ~IWindowController() override;

private:
    void GetAppletResourceUserId(Kernel::HLERequestContext& ctx);
    void AcquireForegroundRights(Kernel::HLERequestContext& ctx);
};

class IAudioController final : public ServiceFramework<IAudioController> {
public:
    IAudioController();
    ~IAudioController() override;

private:
    void SetExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetMainAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletExpectedMasterVolume(Kernel::HLERequestContext& ctx);

    u32 volume{100};
};

class IDisplayController final : public ServiceFramework<IDisplayController> {
public:
    IDisplayController();
    ~IDisplayController() override;
};

class IDebugFunctions final : public ServiceFramework<IDebugFunctions> {
public:
    IDebugFunctions();
    ~IDebugFunctions() override;
};

class ISelfController final : public ServiceFramework<ISelfController> {
public:
    explicit ISelfController(std::shared_ptr<NVFlinger::NVFlinger> nvflinger);
    ~ISelfController() override;

private:
    void SetFocusHandlingMode(Kernel::HLERequestContext& ctx);
    void SetRestartMessageEnabled(Kernel::HLERequestContext& ctx);
    void SetPerformanceModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetOperationModeChangedNotification(Kernel::HLERequestContext& ctx);
    void SetOutOfFocusSuspendingEnabled(Kernel::HLERequestContext& ctx);
    void LockExit(Kernel::HLERequestContext& ctx);
    void UnlockExit(Kernel::HLERequestContext& ctx);
    void GetLibraryAppletLaunchableEvent(Kernel::HLERequestContext& ctx);
    void SetScreenShotImageOrientation(Kernel::HLERequestContext& ctx);
    void CreateManagedDisplayLayer(Kernel::HLERequestContext& ctx);
    void SetScreenShotPermission(Kernel::HLERequestContext& ctx);
    void SetHandlesRequestToDisplay(Kernel::HLERequestContext& ctx);
    void SetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);
    void GetIdleTimeDetectionExtension(Kernel::HLERequestContext& ctx);

    std::shared_ptr<NVFlinger::NVFlinger> nvflinger;
    Kernel::EventPair launchable_event;
    u32 idle_time_detection_extension = 0;
};

class ICommonStateGetter final : public ServiceFramework<ICommonStateGetter> {
public:
    explicit ICommonStateGetter(std::shared_ptr<AppletMessageQueue> msg_queue);
    ~ICommonStateGetter() override;

private:
    enum class FocusState : u8 {
        InFocus = 1,
        NotInFocus = 2,
    };

    enum class OperationMode : u8 {
        Handheld = 0,
        Docked = 1,
    };

    void GetEventHandle(Kernel::HLERequestContext& ctx);
    void ReceiveMessage(Kernel::HLERequestContext& ctx);
    void GetCurrentFocusState(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolutionChangeEvent(Kernel::HLERequestContext& ctx);
    void GetOperationMode(Kernel::HLERequestContext& ctx);
    void GetPerformanceMode(Kernel::HLERequestContext& ctx);
    void GetBootMode(Kernel::HLERequestContext& ctx);
    void GetDefaultDisplayResolution(Kernel::HLERequestContext& ctx);

    std::shared_ptr<AppletMessageQueue> msg_queue;
};

class IStorage final : public ServiceFramework<IStorage> {
public:
    explicit IStorage(std::vector<u8> buffer);
    ~IStorage() override;

    const std::vector<u8>& GetData() const;

private:
    void Open(Kernel::HLERequestContext& ctx);

    std::vector<u8> buffer;

    friend class IStorageAccessor;
};

class IStorageAccessor final : public ServiceFramework<IStorageAccessor> {
public:
    explicit IStorageAccessor(IStorage& backing);
    ~IStorageAccessor() override;

private:
    void GetSize(Kernel::HLERequestContext& ctx);
    void Write(Kernel::HLERequestContext& ctx);
    void Read(Kernel::HLERequestContext& ctx);

    IStorage& backing;
};

class ILibraryAppletCreator final : public ServiceFramework<ILibraryAppletCreator> {
public:
    ILibraryAppletCreator();
    ~ILibraryAppletCreator() override;

private:
    void CreateLibraryApplet(Kernel::HLERequestContext& ctx);
    void CreateStorage(Kernel::HLERequestContext& ctx);
    void CreateTransferMemoryStorage(Kernel::HLERequestContext& ctx);
};

class IApplicationFunctions final : public ServiceFramework<IApplicationFunctions> {
public:
    IApplicationFunctions();
    ~IApplicationFunctions() override;

private:
    void PopLaunchParameter(Kernel::HLERequestContext& ctx);
    void CreateApplicationAndRequestToStartForQuest(Kernel::HLERequestContext& ctx);
    void EnsureSaveData(Kernel::HLERequestContext& ctx);
    void SetTerminateResult(Kernel::HLERequestContext& ctx);
    void GetDisplayVersion(Kernel::HLERequestContext& ctx);
    void GetDesiredLanguage(Kernel::HLERequestContext& ctx);
    void InitializeGamePlayRecording(Kernel::HLERequestContext& ctx);
    void SetGamePlayRecordingState(Kernel::HLERequestContext& ctx);
    void NotifyRunning(Kernel::HLERequestContext& ctx);
    void GetPseudoDeviceId(Kernel::HLERequestContext& ctx);
    void ExtendSaveData(Kernel::HLERequestContext& ctx);
    void GetSaveDataSize(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButtonShortAndLongPressed(Kernel::HLERequestContext& ctx);
    void BeginBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EndBlockingHomeButton(Kernel::HLERequestContext& ctx);
    void EnableApplicationCrashReport(Kernel::HLERequestContext& ctx);
};

class IHomeMenuFunctions final : public ServiceFramework<IHomeMenuFunctions> {
public:
    IHomeMenuFunctions();
    ~IHomeMenuFunctions() override;

private:
    void RequestToGetForeground(Kernel::HLERequestContext& ctx);
};

class IGlobalStateController final : public ServiceFramework<IGlobalStateController> {
public:
    IGlobalStateController();
    ~IGlobalStateController() override;
};

class IApplicationCreator final : public ServiceFramework<IApplicationCreator> {
public:
    IApplicationCreator();
    ~IApplicationCreator() override;
};

class IProcessWindingController final : public ServiceFramework<IProcessWindingController> {
public:
    IProcessWindingController();
    ~IProcessWindingController() override;
};

/// Registers all AM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager,
                       std::shared_ptr<NVFlinger::NVFlinger> nvflinger);

} // namespace AM
} // namespace Service
