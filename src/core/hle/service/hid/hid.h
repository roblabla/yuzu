// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "controllers/controller_base.h"
#include "core/hle/service/service.h"

namespace CoreTiming {
struct EventType;
}

namespace Kernel {
class SharedMemory;
}

namespace SM {
class ServiceManager;
}

namespace Service::HID {

enum class HidController : std::size_t {
    DebugPad,
    Touchscreen,
    Mouse,
    Keyboard,
    XPad,
    Unknown1,
    Unknown2,
    Unknown3,
    SixAxisSensor,
    NPad,
    Gesture,

    MaxControllers,
};

class IAppletResource final : public ServiceFramework<IAppletResource> {
public:
    IAppletResource();
    ~IAppletResource() override;

    void ActivateController(HidController controller);
    void DeactivateController(HidController controller);

    template <typename T>
    T& GetController(HidController controller) {
        return static_cast<T&>(*controllers[static_cast<size_t>(controller)]);
    }

    template <typename T>
    const T& GetController(HidController controller) const {
        return static_cast<T&>(*controllers[static_cast<size_t>(controller)]);
    }

private:
    template <typename T>
    void MakeController(HidController controller) {
        controllers[static_cast<std::size_t>(controller)] = std::make_unique<T>();
    }

    void GetSharedMemoryHandle(Kernel::HLERequestContext& ctx);
    void UpdateControllers(u64 userdata, int cycles_late);

    Kernel::SharedPtr<Kernel::SharedMemory> shared_mem;

    CoreTiming::EventType* pad_update_event;

    std::array<std::unique_ptr<ControllerBase>, static_cast<size_t>(HidController::MaxControllers)>
        controllers{};
};

class Hid final : public ServiceFramework<Hid> {
public:
    Hid();
    ~Hid() override;

    std::shared_ptr<IAppletResource> GetAppletResource();

private:
    void CreateAppletResource(Kernel::HLERequestContext& ctx);
    void ActivateXpad(Kernel::HLERequestContext& ctx);
    void ActivateDebugPad(Kernel::HLERequestContext& ctx);
    void ActivateTouchScreen(Kernel::HLERequestContext& ctx);
    void ActivateMouse(Kernel::HLERequestContext& ctx);
    void ActivateKeyboard(Kernel::HLERequestContext& ctx);
    void ActivateGesture(Kernel::HLERequestContext& ctx);
    void ActivateNpadWithRevision(Kernel::HLERequestContext& ctx);
    void StartSixAxisSensor(Kernel::HLERequestContext& ctx);
    void SetGyroscopeZeroDriftMode(Kernel::HLERequestContext& ctx);
    void IsSixAxisSensorAtRest(Kernel::HLERequestContext& ctx);
    void SetSupportedNpadStyleSet(Kernel::HLERequestContext& ctx);
    void GetSupportedNpadStyleSet(Kernel::HLERequestContext& ctx);
    void SetSupportedNpadIdType(Kernel::HLERequestContext& ctx);
    void ActivateNpad(Kernel::HLERequestContext& ctx);
    void AcquireNpadStyleSetUpdateEventHandle(Kernel::HLERequestContext& ctx);
    void DisconnectNpad(Kernel::HLERequestContext& ctx);
    void GetPlayerLedPattern(Kernel::HLERequestContext& ctx);
    void SetNpadJoyHoldType(Kernel::HLERequestContext& ctx);
    void GetNpadJoyHoldType(Kernel::HLERequestContext& ctx);
    void SetNpadJoyAssignmentModeSingleByDefault(Kernel::HLERequestContext& ctx);
    void BeginPermitVibrationSession(Kernel::HLERequestContext& ctx);
    void EndPermitVibrationSession(Kernel::HLERequestContext& ctx);
    void SendVibrationValue(Kernel::HLERequestContext& ctx);
    void SendVibrationValues(Kernel::HLERequestContext& ctx);
    void GetActualVibrationValue(Kernel::HLERequestContext& ctx);
    void SetNpadJoyAssignmentModeDual(Kernel::HLERequestContext& ctx);
    void MergeSingleJoyAsDualJoy(Kernel::HLERequestContext& ctx);
    void SetNpadHandheldActivationMode(Kernel::HLERequestContext& ctx);
    void GetVibrationDeviceInfo(Kernel::HLERequestContext& ctx);
    void CreateActiveVibrationDeviceList(Kernel::HLERequestContext& ctx);
    void ActivateConsoleSixAxisSensor(Kernel::HLERequestContext& ctx);
    void StartConsoleSixAxisSensor(Kernel::HLERequestContext& ctx);
    void StopSixAxisSensor(Kernel::HLERequestContext& ctx);
    void SetIsPalmaAllConnectable(Kernel::HLERequestContext& ctx);
    void SetPalmaBoostMode(Kernel::HLERequestContext& ctx);

    std::shared_ptr<IAppletResource> applet_resource;
};

/// Reload input devices. Used when input configuration changed
void ReloadInputDevices();

/// Registers all HID services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::HID
