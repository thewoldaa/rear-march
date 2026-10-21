// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace rm {

// The version string, also stamped into the executable's resources by CMake.
// REARMARCH_VERSION is a public compile definition of rearmarch_core.
namespace version {
inline constexpr const char* kVersion = REARMARCH_VERSION;
inline constexpr const char* kAppName = "RearMarch";
}  // namespace version

}  // namespace rm
