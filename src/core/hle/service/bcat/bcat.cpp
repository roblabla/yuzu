// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/bcat/bcat.h"

namespace Service::BCAT {

BCAT::BCAT(std::shared_ptr<Module> module, const char* name)
    : Module::Interface(std::move(module), name) {
    static const FunctionInfo functions[] = {
        {0, &BCAT::CreateBcatService, "CreateBcatService"},
    };
    RegisterHandlers(functions);
}

BCAT::~BCAT() = default;
} // namespace Service::BCAT
