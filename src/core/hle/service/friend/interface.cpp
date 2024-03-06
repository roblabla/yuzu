// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/friend/interface.h"

namespace Service::Friend {

Friend::Friend(std::shared_ptr<Module> module, const char* name)
    : Interface(std::move(module), name) {
    static const FunctionInfo functions[] = {
        {0, &Friend::CreateFriendService, "CreateFriendService"},
        {1, nullptr, "CreateNotificationService"},
        {2, nullptr, "CreateDaemonSuspendSessionService"},
    };
    RegisterHandlers(functions);
}

Friend::~Friend() = default;

} // namespace Service::Friend
