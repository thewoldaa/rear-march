// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace rm {
namespace platform {

// Mouse side button edge detection for the dashboard's trigger picker.
//
// ImGui has no key identifiers for X1/X2, so the picker polls the physical
// button state instead. The previous state is kept in a function-local static;
// only the UI thread calls these.
int side_button_pressed_edge();  // 1, 2, or 0
bool side_button_down(int button);

}  // namespace platform
}  // namespace rm
