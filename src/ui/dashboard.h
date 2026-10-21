// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "core/config.h"
#include "core/i18n.h"
#include "core/turn.h"

namespace rm {

class TurnWorker;

namespace ui {

// Live, read-only state the dashboard displays. Owned by the application and
// refreshed once per frame.
struct DashboardState {
    bool hook_running = false;
    bool locked = false;
    bool calibrating = false;
    bool elevated = false;
    int dpi_scale_percent = 100;  // Windows' own DPI setting, informational

    uint64_t turns_this_session = 0;
    int64_t last_turn_counts = 0;
    int last_turn_sign = 1;
    bool has_turned = false;

    std::string message;  // transient status line, empty when none
};

// Everything the dashboard can change or ask the application to do.
struct DashboardContext {
    AppConfig* config = nullptr;
    I18n* i18n = nullptr;
    DashboardState* state = nullptr;

    bool* config_dirty = nullptr;
    bool* request_hide = nullptr;
    bool* request_exit = nullptr;
    bool* request_locale_reload = nullptr;
    bool* capture_active = nullptr;

    // Actions that must reach the application rather than only the config
    // object: calibration state is owned by the application, and the registry
    // entry behind "Start with Windows" has to be written when the box is
    // toggled, not only when the file is saved.
    std::function<void(bool)> set_calibrating;
    std::function<void(bool)> set_start_with_windows;

    // Returns the number of turns requested so far, for the live counter.
    std::function<uint64_t()> get_turns;
};

void draw_dashboard(DashboardContext& context);

}  // namespace ui
}  // namespace rm
