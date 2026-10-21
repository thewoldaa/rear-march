// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <chrono>
#include <cstdint>

namespace rm {
namespace platform {

// Sleeps with the high-resolution waitable timer where available.
void sleep_for(std::chrono::nanoseconds duration);

// Monotonic microseconds since an unspecified epoch.
int64_t monotonic_us();

}  // namespace platform
}  // namespace rm
