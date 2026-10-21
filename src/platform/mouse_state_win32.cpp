// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/mouse_state.h"

#include <windows.h>

namespace rm {
namespace platform {
namespace {

bool down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

}  // namespace

int side_button_pressed_edge() {
    static bool last_x1 = false;
    static bool last_x2 = false;

    const bool x1 = down(VK_XBUTTON1);
    const bool x2 = down(VK_XBUTTON2);

    int pressed = 0;
    if (x1 && !last_x1) {
        pressed = 1;
    } else if (x2 && !last_x2) {
        pressed = 2;
    }

    last_x1 = x1;
    last_x2 = x2;
    return pressed;
}

bool side_button_down(int button) {
    return down(button == 1 ? VK_XBUTTON1 : VK_XBUTTON2);
}

}  // namespace platform
}  // namespace rm
