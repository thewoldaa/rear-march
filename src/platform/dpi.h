// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace rm {
namespace dpi {

// Scale factor for a window's monitor (1.0 at 96 DPI, 1.5 at 144 DPI).
float scale_for_window(void* window);

// Scale factor of the primary monitor, for sizing a window before it exists.
float system_scale();

void enable_awareness();

}  // namespace dpi
}  // namespace rm
