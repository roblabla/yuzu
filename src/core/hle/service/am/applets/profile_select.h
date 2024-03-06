// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include "common/common_funcs.h"
#include "core/hle/service/acc/profile_manager.h"
#include "core/hle/service/am/applets/applets.h"

namespace Service::AM::Applets {

struct UserSelectionConfig {
    // TODO(DarkLordZach): RE this structure
    // It seems to be flags and the like that determine the UI of the applet on the switch... from
    // my research this is safe to ignore for now.
    INSERT_PADDING_BYTES(0xA0);
};
static_assert(sizeof(UserSelectionConfig) == 0xA0, "UserSelectionConfig has incorrect size.");

struct UserSelectionOutput {
    u64 result;
    u128 uuid_selected;
};
static_assert(sizeof(UserSelectionOutput) == 0x18, "UserSelectionOutput has incorrect size.");

class ProfileSelect final : public Applet {
public:
    ProfileSelect();
    ~ProfileSelect() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void SelectionComplete(std::optional<Account::UUID> uuid);

private:
    UserSelectionConfig config;
    bool complete = false;
    ResultCode status = RESULT_SUCCESS;
    std::vector<u8> final_data;
};

} // namespace Service::AM::Applets
