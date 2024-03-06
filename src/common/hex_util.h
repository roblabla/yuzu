// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>
#include <fmt/format.h>
#include "common/common_types.h"

namespace Common {

u8 ToHexNibble(char c1);

std::vector<u8> HexStringToVector(std::string_view str, bool little_endian);

template <std::size_t Size, bool le = false>
std::array<u8, Size> HexStringToArray(std::string_view str) {
    std::array<u8, Size> out{};
    if constexpr (le) {
        for (std::size_t i = 2 * Size - 2; i <= 2 * Size; i -= 2)
            out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
    } else {
        for (std::size_t i = 0; i < 2 * Size; i += 2)
            out[i / 2] = (ToHexNibble(str[i]) << 4) | ToHexNibble(str[i + 1]);
    }
    return out;
}

std::string HexVectorToString(const std::vector<u8>& vector, bool upper = true);

template <std::size_t Size>
std::string HexArrayToString(std::array<u8, Size> array, bool upper = true) {
    std::string out;
    for (u8 c : array)
        out += fmt::format(upper ? "{:02X}" : "{:02x}", c);
    return out;
}

std::array<u8, 0x10> operator"" _array16(const char* str, std::size_t len);
std::array<u8, 0x20> operator"" _array32(const char* str, std::size_t len);

} // namespace Common
