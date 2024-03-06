// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::AM {

class OMM final : public ServiceFramework<OMM> {
public:
    explicit OMM();
    ~OMM() override;
};

} // namespace Service::AM
