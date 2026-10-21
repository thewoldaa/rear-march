// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <filesystem>
#include <string>

namespace rm {

// Per-user application directories. No admin rights are required and nothing
// is written outside the user's profile.
namespace paths {

std::filesystem::path data_dir();
std::filesystem::path config_file();
std::filesystem::path exe_dir();
std::filesystem::path locale_dir();

bool ensure_data_dir();

}  // namespace paths

}  // namespace rm
