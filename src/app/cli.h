// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace rm {

// Command line options. Parsed before any window or hook is created, so
// --safe-mode can suppress hook installation entirely.
struct CliOptions {
    bool safe_mode = false;
    bool minimized = false;
    bool show_help = false;
    bool show_version = false;

    // Used by the installer when the user declines "start with Windows": the
    // application must then leave the registry entry alone instead of writing
    // the one its default configuration asks for.
    bool no_autostart = false;

    std::string locale;        // "en" or "id"; empty means detect
    std::wstring config_path;  // empty means the default location

    // Returns false and fills `error` on malformed input.
    static bool parse(int argc, wchar_t** argv, CliOptions& out, std::string& error);
};

// Usage text shown for --help.
std::string usage_text();

}  // namespace rm
