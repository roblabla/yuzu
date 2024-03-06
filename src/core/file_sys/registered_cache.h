// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <boost/container/flat_map.hpp>
#include "common/common_types.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/vfs.h"

namespace FileSys {
class CNMT;
class NCA;
class NSP;
class XCI;

enum class ContentRecordType : u8;
enum class TitleType : u8;

struct ContentRecord;

using NcaID = std::array<u8, 0x10>;
using RegisteredCacheParsingFunction = std::function<VirtualFile(const VirtualFile&, const NcaID&)>;
using VfsCopyFunction = std::function<bool(const VirtualFile&, const VirtualFile&, size_t)>;

enum class InstallResult {
    Success,
    ErrorAlreadyExists,
    ErrorCopyFailed,
    ErrorMetaFailed,
};

struct RegisteredCacheEntry {
    u64 title_id;
    ContentRecordType type;

    std::string DebugInfo() const;
};

constexpr u64 GetUpdateTitleID(u64 base_title_id) {
    return base_title_id | 0x800;
}

// boost flat_map requires operator< for O(log(n)) lookups.
bool operator<(const RegisteredCacheEntry& lhs, const RegisteredCacheEntry& rhs);

// std unique requires operator== to identify duplicates.
bool operator==(const RegisteredCacheEntry& lhs, const RegisteredCacheEntry& rhs);
bool operator!=(const RegisteredCacheEntry& lhs, const RegisteredCacheEntry& rhs);

/*
 * A class that catalogues NCAs in the registered directory structure.
 * Nintendo's registered format follows this structure:
 *
 * Root
 *   | 000000XX <- XX is the ____ two digits of the NcaID
 *       | <hash>.nca <- hash is the NcaID (first half of SHA256 over entire file) (folder)
 *         | 00
 *         | 01 <- Actual content split along 4GB boundaries. (optional)
 *
 * (This impl also supports substituting the nca dir for an nca file, as that's more convenient
 * when 4GB splitting can be ignored.)
 */
class RegisteredCache {
    friend class RegisteredCacheUnion;

public:
    // Parsing function defines the conversion from raw file to NCA. If there are other steps
    // besides creating the NCA from the file (e.g. NAX0 on SD Card), that should go in a custom
    // parsing function.
    explicit RegisteredCache(VirtualDir dir,
                             RegisteredCacheParsingFunction parsing_function =
                                 [](const VirtualFile& file, const NcaID& id) { return file; });
    ~RegisteredCache();

    void Refresh();

    bool HasEntry(u64 title_id, ContentRecordType type) const;
    bool HasEntry(RegisteredCacheEntry entry) const;

    std::optional<u32> GetEntryVersion(u64 title_id) const;

    VirtualFile GetEntryUnparsed(u64 title_id, ContentRecordType type) const;
    VirtualFile GetEntryUnparsed(RegisteredCacheEntry entry) const;

    VirtualFile GetEntryRaw(u64 title_id, ContentRecordType type) const;
    VirtualFile GetEntryRaw(RegisteredCacheEntry entry) const;

    std::unique_ptr<NCA> GetEntry(u64 title_id, ContentRecordType type) const;
    std::unique_ptr<NCA> GetEntry(RegisteredCacheEntry entry) const;

    std::vector<RegisteredCacheEntry> ListEntries() const;
    // If a parameter is not std::nullopt, it will be filtered for from all entries.
    std::vector<RegisteredCacheEntry> ListEntriesFilter(
        std::optional<TitleType> title_type = {}, std::optional<ContentRecordType> record_type = {},
        std::optional<u64> title_id = {}) const;

    // Raw copies all the ncas from the xci/nsp to the csache. Does some quick checks to make sure
    // there is a meta NCA and all of them are accessible.
    InstallResult InstallEntry(const XCI& xci, bool overwrite_if_exists = false,
                               const VfsCopyFunction& copy = &VfsRawCopy);
    InstallResult InstallEntry(const NSP& nsp, bool overwrite_if_exists = false,
                               const VfsCopyFunction& copy = &VfsRawCopy);

    // Due to the fact that we must use Meta-type NCAs to determine the existance of files, this
    // poses quite a challenge. Instead of creating a new meta NCA for this file, yuzu will create a
    // dir inside the NAND called 'yuzu_meta' and store the raw CNMT there.
    // TODO(DarkLordZach): Author real meta-type NCAs and install those.
    InstallResult InstallEntry(const NCA& nca, TitleType type, bool overwrite_if_exists = false,
                               const VfsCopyFunction& copy = &VfsRawCopy);

private:
    template <typename T>
    void IterateAllMetadata(std::vector<T>& out,
                            std::function<T(const CNMT&, const ContentRecord&)> proc,
                            std::function<bool(const CNMT&, const ContentRecord&)> filter) const;
    std::vector<NcaID> AccumulateFiles() const;
    void ProcessFiles(const std::vector<NcaID>& ids);
    void AccumulateYuzuMeta();
    std::optional<NcaID> GetNcaIDFromMetadata(u64 title_id, ContentRecordType type) const;
    VirtualFile GetFileAtID(NcaID id) const;
    VirtualFile OpenFileOrDirectoryConcat(const VirtualDir& dir, std::string_view path) const;
    InstallResult RawInstallNCA(const NCA& nca, const VfsCopyFunction& copy,
                                bool overwrite_if_exists, std::optional<NcaID> override_id = {});
    bool RawInstallYuzuMeta(const CNMT& cnmt);

    VirtualDir dir;
    RegisteredCacheParsingFunction parser;
    Core::Crypto::KeyManager keys;

    // maps tid -> NcaID of meta
    boost::container::flat_map<u64, NcaID> meta_id;
    // maps tid -> meta
    boost::container::flat_map<u64, CNMT> meta;
    // maps tid -> meta for CNMT in yuzu_meta
    boost::container::flat_map<u64, CNMT> yuzu_meta;
};

// Combines multiple RegisteredCaches (i.e. SysNAND, UserNAND, SDMC) into one interface.
class RegisteredCacheUnion {
public:
    explicit RegisteredCacheUnion(std::vector<RegisteredCache*> caches);

    void Refresh();

    bool HasEntry(u64 title_id, ContentRecordType type) const;
    bool HasEntry(RegisteredCacheEntry entry) const;

    std::optional<u32> GetEntryVersion(u64 title_id) const;

    VirtualFile GetEntryUnparsed(u64 title_id, ContentRecordType type) const;
    VirtualFile GetEntryUnparsed(RegisteredCacheEntry entry) const;

    VirtualFile GetEntryRaw(u64 title_id, ContentRecordType type) const;
    VirtualFile GetEntryRaw(RegisteredCacheEntry entry) const;

    std::unique_ptr<NCA> GetEntry(u64 title_id, ContentRecordType type) const;
    std::unique_ptr<NCA> GetEntry(RegisteredCacheEntry entry) const;

    std::vector<RegisteredCacheEntry> ListEntries() const;
    // If a parameter is not std::nullopt, it will be filtered for from all entries.
    std::vector<RegisteredCacheEntry> ListEntriesFilter(
        std::optional<TitleType> title_type = {}, std::optional<ContentRecordType> record_type = {},
        std::optional<u64> title_id = {}) const;

private:
    std::vector<RegisteredCache*> caches;
};

} // namespace FileSys
