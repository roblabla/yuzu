// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::NS {

/// Registers all NS services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager);

} // namespace Service::NS
