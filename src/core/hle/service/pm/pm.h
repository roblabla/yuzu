// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Service::SM {
class ServiceManager;
}

namespace Service::PM {

enum class SystemBootMode {
    Normal,
    Maintenance,
};

/// Registers all PM services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::PM
