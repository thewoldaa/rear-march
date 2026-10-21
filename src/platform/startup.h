// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace rm {
namespace startup {

// The Run value name. Must match the installer's [Registry] entry.
inline constexpr wchar_t kValueName[] = L"RearMarch";

// The command line stored in HKCU\...\Run: the quoted executable plus
// --minimized.
std::wstring command_line();

bool is_enabled();
bool set_enabled(bool enabled);

}  // namespace startup
}  // namespace rm
