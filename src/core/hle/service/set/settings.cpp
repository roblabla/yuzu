// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/set/set.h"
#include "core/hle/service/set/set_cal.h"
#include "core/hle/service/set/set_fd.h"
#include "core/hle/service/set/set_sys.h"
#include "core/hle/service/set/settings.h"

namespace Service::Set {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<SET>()->InstallAsService(service_manager);
    std::make_shared<SET_CAL>()->InstallAsService(service_manager);
    std::make_shared<SET_FD>()->InstallAsService(service_manager);
    std::make_shared<SET_SYS>()->InstallAsService(service_manager);
}

} // namespace Service::Set
