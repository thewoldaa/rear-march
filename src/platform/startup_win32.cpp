// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/startup.h"

#include <vector>

#include <windows.h>

#include "platform/paths.h"

namespace rm {
namespace startup {
namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring exe_path() {
    // GetModuleFileNameW offers no way to ask for the required length, so grow
    // the buffer until the returned length fits.
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2);
    }
}

}  // namespace

std::wstring command_line() {
    // Quoted because the install path may contain spaces.
    return L"\"" + exe_path() + L"\" --minimized";
}

bool is_enabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) !=
        ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[1024]{};
    DWORD size = sizeof(value);
    DWORD type = 0;
    const LONG result = RegQueryValueExW(key, kValueName, nullptr, &type,
                                         reinterpret_cast<LPBYTE>(value), &size);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_SZ) {
        return false;
    }

    return _wcsicmp(value, command_line().c_str()) == 0;
}

bool set_enabled(bool enabled) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring command = command_line();
        result = RegSetValueExW(key, kValueName, 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(command.c_str()),
                                static_cast<DWORD>((command.size() + 1) *
                                                   sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(key, kValueName);
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

}  // namespace startup
}  // namespace rm
