// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "common/common_types.h"

namespace Common {

/// Make a string lowercase
std::string ToLower(std::string str);

/// Make a string uppercase
std::string ToUpper(std::string str);

std::string StringFromBuffer(const std::vector<u8>& data);

std::string StripSpaces(const std::string& s);
std::string StripQuotes(const std::string& s);

std::string StringFromBool(bool value);

std::string TabsToSpaces(int tab_size, std::string in);

void SplitString(const std::string& str, char delim, std::vector<std::string>& output);

// "C:/Windows/winhelp.exe" to "C:/Windows/", "winhelp", ".exe"
bool SplitPath(const std::string& full_path, std::string* _pPath, std::string* _pFilename,
               std::string* _pExtension);

void BuildCompleteFilename(std::string& _CompleteFilename, const std::string& _Path,
                           const std::string& _Filename);
std::string ReplaceAll(std::string result, const std::string& src, const std::string& dest);

std::string UTF16ToUTF8(const std::u16string& input);
std::u16string UTF8ToUTF16(const std::string& input);

#ifdef _WIN32
std::string UTF16ToUTF8(const std::wstring& input);
std::wstring UTF8ToUTF16W(const std::string& str);

#endif

/**
 * Compares the string defined by the range [`begin`, `end`) to the null-terminated C-string
 * `other` for equality.
 */
template <typename InIt>
bool ComparePartialString(InIt begin, InIt end, const char* other) {
    for (; begin != end && *other != '\0'; ++begin, ++other) {
        if (*begin != *other) {
            return false;
        }
    }
    // Only return true if both strings finished at the same point
    return (begin == end) == (*other == '\0');
}

/**
 * Creates a std::string from a fixed-size NUL-terminated char buffer. If the buffer isn't
 * NUL-terminated then the string ends at max_len characters.
 */
std::string StringFromFixedZeroTerminatedBuffer(const char* buffer, std::size_t max_len);

/**
 * Creates a UTF-16 std::u16string from a fixed-size NUL-terminated char buffer. If the buffer isn't
 * null-terminated, then the string ends at the greatest multiple of two less then or equal to
 * max_len_bytes.
 */
std::u16string UTF16StringFromFixedZeroTerminatedBuffer(std::u16string_view buffer,
                                                        std::size_t max_len);

/**
 * Attempts to trim an arbitrary prefix from `path`, leaving only the part starting at `root`. It's
 * intended to be used to strip a system-specific build directory from the `__FILE__` macro,
 * leaving only the path relative to the sources root.
 *
 * @param path The input file path as a null-terminated string
 * @param root The name of the root source directory as a null-terminated string. Path up to and
 *             including the last occurrence of this name will be stripped
 * @return A pointer to the same string passed as `path`, but starting at the trimmed portion
 */
const char* TrimSourcePath(const char* path, const char* root = "src");

} // namespace Common
