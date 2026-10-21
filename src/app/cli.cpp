// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/cli.h"

#include <cwctype>
#include <string_view>

namespace rm {
namespace {

bool equals_ignore_case(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }
    return true;
}

// Language codes are ASCII. Anything else cannot name a bundled translation, so
// it is reported as empty, which means "detect the system language".
std::string to_ascii_code(std::wstring_view value) {
    std::string result;
    result.reserve(value.size());
    for (const wchar_t ch : value) {
        if (ch > 0x7F) {
            return {};
        }
        result.push_back(static_cast<char>(ch));
    }
    return result;
}

// Renders an argument for an error message. Characters that cannot appear in a
// narrow message become '?'.
std::string to_printable(std::wstring_view value) {
    std::string result;
    result.reserve(value.size());
    for (const wchar_t ch : value) {
        result.push_back(ch >= 0x20 && ch <= 0x7E ? static_cast<char>(ch) : '?');
    }
    return result;
}

}  // namespace

bool CliOptions::parse(int argc, wchar_t** argv, CliOptions& out, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        const std::wstring_view arg = argv[i];

        if (equals_ignore_case(arg, L"--help") || equals_ignore_case(arg, L"-h")) {
            out.show_help = true;
        } else if (equals_ignore_case(arg, L"--version") || equals_ignore_case(arg, L"-v")) {
            out.show_version = true;
        } else if (equals_ignore_case(arg, L"--safe-mode")) {
            out.safe_mode = true;
        } else if (equals_ignore_case(arg, L"--minimized")) {
            out.minimized = true;
        } else if (equals_ignore_case(arg, L"--no-autostart")) {
            out.no_autostart = true;
        } else if (equals_ignore_case(arg, L"--locale")) {
            if (i + 1 >= argc) {
                error = "--locale requires a language code";
                return false;
            }
            out.locale = to_ascii_code(argv[++i]);
        } else if (equals_ignore_case(arg, L"--config")) {
            if (i + 1 >= argc) {
                error = "--config requires a path";
                return false;
            }
            out.config_path = argv[++i];
        } else {
            error = "Unknown option: " + to_printable(arg);
            return false;
        }
    }
    return true;
}

std::string usage_text() {
    return
        "RearMarch -- one key turns your game camera 180 degrees\n"
        "\n"
        "Usage: RearMarch.exe [options]\n"
        "\n"
        "Options:\n"
        "  -h, --help           Show this help and exit\n"
        "  -v, --version        Print the version and exit\n"
        "      --minimized      Start hidden in the tray\n"
        "      --no-autostart   Do not create or change the Windows startup entry\n"
        "      --locale CODE    Start in this language (en, id)\n"
        "      --safe-mode      Start disabled, with the default configuration\n"
        "      --config PATH    Use an alternative configuration file\n";
}

}  // namespace rm
