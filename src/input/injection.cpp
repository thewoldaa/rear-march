// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/injection.h"

#include <windows.h>

namespace rm {
namespace input {
namespace {

INPUT make_move(int32_t dx, int32_t dy) {
    INPUT event{};
    event.type = INPUT_MOUSE;
    event.mi.dx = dx;
    event.mi.dy = dy;
    event.mi.dwFlags = MOUSEEVENTF_MOVE;
    event.mi.dwExtraInfo = kInjectionTag;
    return event;
}

}  // namespace

uint32_t send_move_batch(const std::vector<int32_t>& dxs, int32_t dy) {
    if (dxs.empty()) {
        return 0;
    }

    std::vector<INPUT> events;
    events.reserve(dxs.size());
    for (size_t i = 0; i < dxs.size(); ++i) {
        events.push_back(make_move(dxs[i], i == 0 ? dy : 0));
    }

    return SendInput(static_cast<UINT>(events.size()), events.data(), sizeof(INPUT));
}

void send_move(int32_t dx, int32_t dy) {
    INPUT event = make_move(dx, dy);
    SendInput(1, &event, sizeof(INPUT));
}

bool physical_button_down(int xbutton) {
    const int vk = xbutton == 1 ? VK_XBUTTON1 : VK_XBUTTON2;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

}  // namespace input
}  // namespace rm
