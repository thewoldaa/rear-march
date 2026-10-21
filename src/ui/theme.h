// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <imgui.h>

namespace rm {
namespace ui {

void apply_theme(float dpi_scale);

namespace colors {
inline constexpr ImU32 kAccent = IM_COL32(0x4C, 0xC2, 0xA6, 0xFF);
inline constexpr ImU32 kWarning = IM_COL32(0xE5, 0xB5, 0x6B, 0xFF);
inline constexpr ImU32 kDanger = IM_COL32(0xE5, 0x6B, 0x6B, 0xFF);
inline constexpr ImU32 kMuted = IM_COL32(0x8A, 0x92, 0xA0, 0xFF);
}  // namespace colors

}  // namespace ui
}  // namespace rm
