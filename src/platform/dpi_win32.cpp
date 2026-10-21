// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/dpi.h"

#include <windows.h>

namespace rm {
namespace dpi {

float scale_for_window(void* window) {
    const UINT value = GetDpiForWindow(static_cast<HWND>(window));
    return value == 0 ? system_scale() : static_cast<float>(value) / 96.0f;
}

float system_scale() {
    const UINT value = GetDpiForSystem();
    return value == 0 ? 1.0f : static_cast<float>(value) / 96.0f;
}

void enable_awareness() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

}  // namespace dpi
}  // namespace rm
