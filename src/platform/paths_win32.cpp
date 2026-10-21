// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/paths.h"

#include <vector>

#include <windows.h>
#include <shlobj.h>

namespace rm {
namespace paths {
namespace {

std::filesystem::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR raw = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &raw)) && raw) {
        std::filesystem::path result(raw);
        CoTaskMemFree(raw);
        return result;
    }
    if (raw) {
        CoTaskMemFree(raw);
    }
    return {};
}

}  // namespace

std::filesystem::path data_dir() {
    static const std::filesystem::path path = [] {
        auto base = known_folder(FOLDERID_LocalAppData);
        if (base.empty()) {
            base = std::filesystem::temp_directory_path();
        }
        return base / L"RearMarch";
    }();
    return path;
}

std::filesystem::path config_file() {
    return data_dir() / L"config.json";
}

std::filesystem::path exe_dir() {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
    return std::filesystem::path(buffer.data()).parent_path();
}

std::filesystem::path locale_dir() {
    const auto installed = exe_dir() / L"locales";
    if (std::filesystem::exists(installed)) {
        return installed;
    }
    // Development layout: the binary lives in build/<config>/, so the locales
    // at the repository root are two levels up. The build also copies the
    // directory next to the binary, making this a fallback for the case where
    // the exe is moved or the copy step did not run.
    const auto repo = exe_dir() / L".." / L".." / L"locales";
    if (std::filesystem::exists(repo)) {
        return std::filesystem::weakly_canonical(repo);
    }
    return installed;
}

bool ensure_data_dir() {
    std::error_code ec;
    std::filesystem::create_directories(data_dir(), ec);
    return !ec;
}

}  // namespace paths
}  // namespace rm
