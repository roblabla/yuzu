// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <optional>

#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/result.h"

namespace Service::Account {
constexpr std::size_t MAX_USERS = 8;
constexpr u128 INVALID_UUID{{0, 0}};

struct UUID {
    // UUIDs which are 0 are considered invalid!
    u128 uuid = INVALID_UUID;
    UUID() = default;
    explicit UUID(const u128& id) : uuid{id} {}
    explicit UUID(const u64 lo, const u64 hi) : uuid{{lo, hi}} {}

    explicit operator bool() const {
        return uuid != INVALID_UUID;
    }

    bool operator==(const UUID& rhs) const {
        return uuid == rhs.uuid;
    }

    bool operator!=(const UUID& rhs) const {
        return !operator==(rhs);
    }

    // TODO(ogniK): Properly generate uuids based on RFC-4122
    static UUID Generate();

    // Set the UUID to {0,0} to be considered an invalid user
    void Invalidate() {
        uuid = INVALID_UUID;
    }

    std::string Format() const;
    std::string FormatSwitch() const;
};
static_assert(sizeof(UUID) == 16, "UUID is an invalid size!");

constexpr std::size_t profile_username_size = 32;
using ProfileUsername = std::array<u8, profile_username_size>;
using UserIDArray = std::array<UUID, MAX_USERS>;

/// Contains extra data related to a user.
/// TODO: RE this structure
struct ProfileData {
    INSERT_PADDING_WORDS(1);
    u32 icon_id;
    u8 bg_color_id;
    INSERT_PADDING_BYTES(0x7);
    INSERT_PADDING_BYTES(0x10);
    INSERT_PADDING_BYTES(0x60);
};
static_assert(sizeof(ProfileData) == 0x80, "ProfileData structure has incorrect size");

/// This holds general information about a users profile. This is where we store all the information
/// based on a specific user
struct ProfileInfo {
    UUID user_uuid;
    ProfileUsername username;
    u64 creation_time;
    ProfileData data; // TODO(ognik): Work out what this is
    bool is_open;
};

struct ProfileBase {
    UUID user_uuid;
    u64_le timestamp;
    ProfileUsername username;

    // Zero out all the fields to make the profile slot considered "Empty"
    void Invalidate() {
        user_uuid.Invalidate();
        timestamp = 0;
        username.fill(0);
    }
};
static_assert(sizeof(ProfileBase) == 0x38, "ProfileBase is an invalid size");

/// The profile manager is used for handling multiple user profiles at once. It keeps track of open
/// users, all the accounts registered on the "system" as well as fetching individual "ProfileInfo"
/// objects
class ProfileManager {
public:
    ProfileManager();
    ~ProfileManager();

    ResultCode AddUser(const ProfileInfo& user);
    ResultCode CreateNewUser(UUID uuid, const ProfileUsername& username);
    ResultCode CreateNewUser(UUID uuid, const std::string& username);
    std::optional<UUID> GetUser(std::size_t index) const;
    std::optional<std::size_t> GetUserIndex(const UUID& uuid) const;
    std::optional<std::size_t> GetUserIndex(const ProfileInfo& user) const;
    bool GetProfileBase(std::optional<std::size_t> index, ProfileBase& profile) const;
    bool GetProfileBase(UUID uuid, ProfileBase& profile) const;
    bool GetProfileBase(const ProfileInfo& user, ProfileBase& profile) const;
    bool GetProfileBaseAndData(std::optional<std::size_t> index, ProfileBase& profile,
                               ProfileData& data) const;
    bool GetProfileBaseAndData(UUID uuid, ProfileBase& profile, ProfileData& data) const;
    bool GetProfileBaseAndData(const ProfileInfo& user, ProfileBase& profile,
                               ProfileData& data) const;
    std::size_t GetUserCount() const;
    std::size_t GetOpenUserCount() const;
    bool UserExists(UUID uuid) const;
    bool UserExistsIndex(std::size_t index) const;
    void OpenUser(UUID uuid);
    void CloseUser(UUID uuid);
    UserIDArray GetOpenUsers() const;
    UserIDArray GetAllUsers() const;
    UUID GetLastOpenedUser() const;

    bool CanSystemRegisterUser() const;

    bool RemoveUser(UUID uuid);
    bool SetProfileBase(UUID uuid, const ProfileBase& profile_new);

private:
    void ParseUserSaveFile();
    void WriteUserSaveFile();
    std::optional<std::size_t> AddToProfiles(const ProfileInfo& profile);
    bool RemoveProfileAtIndex(std::size_t index);

    std::array<ProfileInfo, MAX_USERS> profiles{};
    std::size_t user_count = 0;
    UUID last_opened_user{INVALID_UUID};
};

}; // namespace Service::Account
