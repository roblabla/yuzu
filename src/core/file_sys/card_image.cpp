// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <string>

#include <fmt/ostream.h>

#include "common/logging/log.h"
#include "core/file_sys/card_image.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/submission_package.h"
#include "core/file_sys/vfs_offset.h"
#include "core/loader/loader.h"

namespace FileSys {

constexpr std::array<const char*, 0x4> partition_names = {"update", "normal", "secure", "logo"};

XCI::XCI(VirtualFile file_)
    : file(std::move(file_)), program_nca_status{Loader::ResultStatus::ErrorXCIMissingProgramNCA},
      partitions(0x4) {
    if (file->ReadObject(&header) != sizeof(GamecardHeader)) {
        status = Loader::ResultStatus::ErrorBadXCIHeader;
        return;
    }

    if (header.magic != Common::MakeMagic('H', 'E', 'A', 'D')) {
        status = Loader::ResultStatus::ErrorBadXCIHeader;
        return;
    }

    PartitionFilesystem main_hfs(
        std::make_shared<OffsetVfsFile>(file, header.hfs_size, header.hfs_offset));

    if (main_hfs.GetStatus() != Loader::ResultStatus::Success) {
        status = main_hfs.GetStatus();
        return;
    }

    for (XCIPartition partition :
         {XCIPartition::Update, XCIPartition::Normal, XCIPartition::Secure, XCIPartition::Logo}) {
        auto raw = main_hfs.GetFile(partition_names[static_cast<std::size_t>(partition)]);
        if (raw != nullptr)
            partitions[static_cast<std::size_t>(partition)] =
                std::make_shared<PartitionFilesystem>(raw);
    }

    secure_partition = std::make_shared<NSP>(
        main_hfs.GetFile(partition_names[static_cast<std::size_t>(XCIPartition::Secure)]));

    const auto secure_ncas = secure_partition->GetNCAsCollapsed();
    std::copy(secure_ncas.begin(), secure_ncas.end(), std::back_inserter(ncas));

    program =
        secure_partition->GetNCA(secure_partition->GetProgramTitleID(), ContentRecordType::Program);
    program_nca_status = secure_partition->GetProgramStatus(secure_partition->GetProgramTitleID());
    if (program_nca_status == Loader::ResultStatus::ErrorNSPMissingProgramNCA)
        program_nca_status = Loader::ResultStatus::ErrorXCIMissingProgramNCA;

    auto result = AddNCAFromPartition(XCIPartition::Update);
    if (result != Loader::ResultStatus::Success) {
        status = result;
        return;
    }

    result = AddNCAFromPartition(XCIPartition::Normal);
    if (result != Loader::ResultStatus::Success) {
        status = result;
        return;
    }

    if (GetFormatVersion() >= 0x2) {
        result = AddNCAFromPartition(XCIPartition::Logo);
        if (result != Loader::ResultStatus::Success) {
            status = result;
            return;
        }
    }

    status = Loader::ResultStatus::Success;
}

XCI::~XCI() = default;

Loader::ResultStatus XCI::GetStatus() const {
    return status;
}

Loader::ResultStatus XCI::GetProgramNCAStatus() const {
    return program_nca_status;
}

VirtualDir XCI::GetPartition(XCIPartition partition) const {
    return partitions[static_cast<std::size_t>(partition)];
}

std::shared_ptr<NSP> XCI::GetSecurePartitionNSP() const {
    return secure_partition;
}

VirtualDir XCI::GetSecurePartition() const {
    return GetPartition(XCIPartition::Secure);
}

VirtualDir XCI::GetNormalPartition() const {
    return GetPartition(XCIPartition::Normal);
}

VirtualDir XCI::GetUpdatePartition() const {
    return GetPartition(XCIPartition::Update);
}

VirtualDir XCI::GetLogoPartition() const {
    return GetPartition(XCIPartition::Logo);
}

u64 XCI::GetProgramTitleID() const {
    return secure_partition->GetProgramTitleID();
}

bool XCI::HasProgramNCA() const {
    return program != nullptr;
}

VirtualFile XCI::GetProgramNCAFile() const {
    if (!HasProgramNCA()) {
        return nullptr;
    }

    return program->GetBaseFile();
}

const std::vector<std::shared_ptr<NCA>>& XCI::GetNCAs() const {
    return ncas;
}

std::shared_ptr<NCA> XCI::GetNCAByType(NCAContentType type) const {
    const auto iter =
        std::find_if(ncas.begin(), ncas.end(),
                     [type](const std::shared_ptr<NCA>& nca) { return nca->GetType() == type; });
    return iter == ncas.end() ? nullptr : *iter;
}

VirtualFile XCI::GetNCAFileByType(NCAContentType type) const {
    auto nca = GetNCAByType(type);
    if (nca != nullptr)
        return nca->GetBaseFile();
    return nullptr;
}

std::vector<VirtualFile> XCI::GetFiles() const {
    return {};
}

std::vector<VirtualDir> XCI::GetSubdirectories() const {
    return {};
}

std::string XCI::GetName() const {
    return file->GetName();
}

VirtualDir XCI::GetParentDirectory() const {
    return file->GetContainingDirectory();
}

Loader::ResultStatus XCI::AddNCAFromPartition(XCIPartition part) {
    if (partitions[static_cast<std::size_t>(part)] == nullptr) {
        return Loader::ResultStatus::ErrorXCIMissingPartition;
    }

    for (const VirtualFile& file : partitions[static_cast<std::size_t>(part)]->GetFiles()) {
        if (file->GetExtension() != "nca")
            continue;
        auto nca = std::make_shared<NCA>(file, nullptr, 0, keys);
        // TODO(DarkLordZach): Add proper Rev1+ Support
        if (nca->IsUpdate())
            continue;
        if (nca->GetType() == NCAContentType::Program) {
            program_nca_status = nca->GetStatus();
        }
        if (nca->GetStatus() == Loader::ResultStatus::Success) {
            ncas.push_back(std::move(nca));
        } else {
            const u16 error_id = static_cast<u16>(nca->GetStatus());
            LOG_CRITICAL(Loader, "Could not load NCA {}/{}, failed with error code {:04X} ({})",
                         partition_names[static_cast<std::size_t>(part)], nca->GetName(), error_id,
                         nca->GetStatus());
        }
    }

    return Loader::ResultStatus::Success;
}

u8 XCI::GetFormatVersion() const {
    return GetLogoPartition() == nullptr ? 0x1 : 0x2;
}
} // namespace FileSys
