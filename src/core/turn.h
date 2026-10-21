// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "core/keycodes.h"

namespace rm {

// Which way the camera turns.
enum class Direction { Left, Right, Random };

std::string_view direction_name(Direction direction);
bool direction_from_name(std::string_view name, Direction& out);

// What the user presses to turn.
//
// A trigger is one physical input, either a keyboard key or a mouse side
// button. Mouse buttons are identified by Windows' XBUTTON numbers, which is
// also how the config file spells them ("X1", "X2").
enum class TriggerKind { Keyboard, MouseX1, MouseX2 };

struct Trigger {
    TriggerKind kind = TriggerKind::MouseX1;
    KeyId key = key::Invalid;  // only meaningful for TriggerKind::Keyboard

    bool valid() const {
        return kind != TriggerKind::Keyboard || key != key::Invalid;
    }
    bool operator==(const Trigger&) const = default;
};

// "X1", "X2", or a canonical key name such as "KeyQ".
std::string trigger_to_string(const Trigger& trigger);

// Accepts the same spellings as trigger_to_string. Key names are matched case
// insensitively. Returns nullopt for anything that is not a usable trigger.
std::optional<Trigger> trigger_from_string(std::string_view text);

// Human readable label for the UI: "X1", "X2", or the key's short label.
std::string_view trigger_label(const Trigger& trigger);

// The user's turning preferences.
struct TurnSettings {
    Trigger trigger;
    double angle_deg = 180.0;
    Direction direction = Direction::Right;
    // Sensitivity multiplier. 1000 means "as calibrated"; 16000 sends sixteen
    // times the counts. Games apply their own sensitivity on top, which is why
    // the calibration value, not the DPI number, is the source of truth.
    int dpi = 1000;
    // Counts that rotate the view a full 360 degrees at DPI 1000. Set by the
    // calibration mode; the dashboard shows it as "counts per 360".
    double counts_per_360 = 6000.0;
    // Time to spread the injected counts over. Zero sends everything in one
    // call, which is the fastest turn the injection path allows.
    int ramp_ms = 0;
};

inline constexpr double kMinAngleDeg = 5.0;
inline constexpr double kMaxAngleDeg = 360.0;
// The slider and the custom input share the same bounds: DPI is a multiplier
// on top of the calibration, so values outside this band have no practical
// meaning and only make an accidental typo more destructive.
inline constexpr int kMinDpi = 1000;
inline constexpr int kMaxDpi = 16000;
inline constexpr int kSliderMinDpi = kMinDpi;
inline constexpr int kSliderMaxDpi = kMaxDpi;
inline constexpr double kMinCountsPer360 = 100.0;
inline constexpr double kMaxCountsPer360 = 1000000.0;
inline constexpr int kMinRampMs = 0;
inline constexpr int kMaxRampMs = 200;

// Per-event delta limit. Games commonly clamp a single mouse event, so a large
// turn is split; 256 counts per event is small enough for the common clamps and
// large enough that a 180 degree turn needs only a handful of events.
inline constexpr int64_t kCountsPerEvent = 256;

// More events than this delay the turn more than they help compatibility.
inline constexpr int kMaxEventsPerTurn = 128;

// Counts for a full 360 at the configured DPI.
double effective_counts_per_360(const TurnSettings& settings);

// Counts for one turn, always at least 1.
int64_t turn_counts(const TurnSettings& settings);

// The signed horizontal delta for one turn. `sign` is +1 for right, -1 for
// left. The magnitude is turn_counts().
int64_t turn_dx(const TurnSettings& settings, int sign);

// Picks left or right for a Random setting. Deterministic for a given engine
// state, which is what makes the random direction testable.
Direction resolve_direction(Direction direction, std::mt19937& rng);

// Splits a signed count into per-event deltas. The first events absorb the
// remainder so the sum is exactly `total`, and the event count is clamped so a
// huge setting cannot produce a million-event injection.
std::vector<int32_t> split_counts(int64_t total, int64_t per_event = kCountsPerEvent,
                                  int max_events = kMaxEventsPerTurn);

// A turn ready to inject.
struct TurnPlan {
    std::vector<int32_t> dxs;  // one entry per mouse event, in order
    int ramp_ms = 0;
};

// Builds the plan for a trigger press. `sign` is +1 right, -1 left.
TurnPlan plan_turn(const TurnSettings& settings, int sign);

// Applies one calibration trim step. `up` increases the sensitivity (more
// counts per turn). The result is clamped to the valid range.
double trimmed_counts(double counts_per_360, double step_percent, bool up);

}  // namespace rm
