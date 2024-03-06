// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#ifdef _WIN32
#include <windows.h>

#include <wincon.h>
#endif

#include "common/logging/backend.h"
#include "yuzu/debugger/console.h"
#include "yuzu/ui_settings.h"

namespace Debugger {
void ToggleConsole() {
    static bool console_shown = false;
    if (console_shown == UISettings::values.show_console) {
        return;
    } else {
        console_shown = UISettings::values.show_console;
    }

#if defined(_WIN32) && !defined(_DEBUG)
    FILE* temp;
    if (UISettings::values.show_console) {
        if (AllocConsole()) {
            // The first parameter for freopen_s is a out parameter, so we can just ignore it
            freopen_s(&temp, "CONIN$", "r", stdin);
            freopen_s(&temp, "CONOUT$", "w", stdout);
            freopen_s(&temp, "CONOUT$", "w", stderr);
            Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
        }
    } else {
        if (FreeConsole()) {
            // In order to close the console, we have to also detach the streams on it.
            // Just redirect them to NUL if there is no console window
            Log::RemoveBackend(Log::ColorConsoleBackend::Name());
            freopen_s(&temp, "NUL", "r", stdin);
            freopen_s(&temp, "NUL", "w", stdout);
            freopen_s(&temp, "NUL", "w", stderr);
        }
    }
#else
    if (UISettings::values.show_console) {
        Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
    } else {
        Log::RemoveBackend(Log::ColorConsoleBackend::Name());
    }
#endif
}
} // namespace Debugger
