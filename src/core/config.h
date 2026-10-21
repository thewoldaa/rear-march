// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/chord.h"
#include "core/turn.h"

namespace rm {

inline constexpr int kConfigSchemaVersion = 1;

// Calibration controls. These are always-active chords, so they must not
// collide with the toggle chord or with each other.
struct CalibrationConfig {
    double step_percent = 2.0;
    Chord up{ModCtrl | ModAlt, key::ArrowUp};
    Chord down{ModCtrl | ModAlt, key::ArrowDown};
    Chord save{ModCtrl | ModAlt, key::Enter};
};

struct FeedbackConfig {
    // Short tone when the mode changes, so a hidden window is not the only
    // feedback. Also used during calibration: a higher tone for up, lower for
    // down.
    bool beep = true;
};

struct GeneralConfig {
    bool start_with_windows = true;
    bool start_minimized = true;
    std::string locale = "en";
};

// The saved configuration. Everything the application needs lives here; there
// is no separate state file.
struct AppConfig {
    int schema = kConfigSchemaVersion;

    bool enabled = true;
    TurnSettings turn;
    Chord toggle{ModCtrl | ModAlt, key::KeyM};
    CalibrationConfig calibration;
    FeedbackConfig feedback;
    GeneralConfig general;

    // The trigger must not duplicate an always-active chord. Returns human
    // readable problems, empty when the configuration is usable.
    std::vector<std::string> validate() const;
};

// First-run defaults: a 180 degree turn on mouse button 4 (X1), with the
// toggle on Ctrl+Alt+M.
AppConfig make_default_config();

// Clamps and repairs out-of-range values. Called after every load, so a
// hand-edited file cannot put the application into a bad state.
void sanitize(AppConfig& config);

// Serialization. to_json produces pretty-printed, stable key order.
std::string to_json(const AppConfig& config);

// Returns nullopt and fills `error` on malformed input. Unknown keys are
// ignored so a config written by a newer version still loads.
std::optional<AppConfig> from_json(std::string_view text, std::string& error);

}  // namespace rm
