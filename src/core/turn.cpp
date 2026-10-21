// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace rm {
namespace {

bool equals_ignore_case(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        const char ca = a[i];
        const char cb = b[i];
        const char la = (ca >= 'A' && ca <= 'Z') ? static_cast<char>(ca - 'A' + 'a') : ca;
        const char lb = (cb >= 'A' && cb <= 'Z') ? static_cast<char>(cb - 'A' + 'a') : cb;
        if (la != lb) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::string_view direction_name(Direction direction) {
    switch (direction) {
        case Direction::Left: return "left";
        case Direction::Right: return "right";
        case Direction::Random: return "random";
    }
    return "right";
}

bool direction_from_name(std::string_view name, Direction& out) {
    if (equals_ignore_case(name, "left")) {
        out = Direction::Left;
        return true;
    }
    if (equals_ignore_case(name, "right")) {
        out = Direction::Right;
        return true;
    }
    if (equals_ignore_case(name, "random")) {
        out = Direction::Random;
        return true;
    }
    return false;
}

std::string trigger_to_string(const Trigger& trigger) {
    switch (trigger.kind) {
        case TriggerKind::MouseX1: return "X1";
        case TriggerKind::MouseX2: return "X2";
        case TriggerKind::Keyboard: return std::string(key_name(trigger.key));
    }
    return "X1";
}

std::optional<Trigger> trigger_from_string(std::string_view text) {
    if (equals_ignore_case(text, "x1") || equals_ignore_case(text, "mousex1") ||
        equals_ignore_case(text, "xbutton1")) {
        return Trigger{TriggerKind::MouseX1, key::Invalid};
    }
    if (equals_ignore_case(text, "x2") || equals_ignore_case(text, "mousex2") ||
        equals_ignore_case(text, "xbutton2")) {
        return Trigger{TriggerKind::MouseX2, key::Invalid};
    }
    for (const auto& info : all_keys()) {
        if (equals_ignore_case(text, info.name)) {
            if (info.is_modifier || info.id == key::Invalid) {
                return std::nullopt;
            }
            return Trigger{TriggerKind::Keyboard, info.id};
        }
    }
    return std::nullopt;
}

std::string_view trigger_label(const Trigger& trigger) {
    switch (trigger.kind) {
        case TriggerKind::MouseX1: return "X1";
        case TriggerKind::MouseX2: return "X2";
        case TriggerKind::Keyboard: return key_label(trigger.key);
    }
    return "?";
}

double effective_counts_per_360(const TurnSettings& settings) {
    const int dpi = std::clamp(settings.dpi, kMinDpi, kMaxDpi);
    return settings.counts_per_360 * (static_cast<double>(dpi) / 1000.0);
}

int64_t turn_counts(const TurnSettings& settings) {
    const double angle = std::clamp(settings.angle_deg, kMinAngleDeg, kMaxAngleDeg);
    const double counts = effective_counts_per_360(settings) * angle / 360.0;
    // Rounding to the nearest count keeps 90 and 270 degree quarter turns
    // symmetric instead of always shaving half a count off.
    const int64_t value = static_cast<int64_t>(std::llround(counts));
    return std::max<int64_t>(value, 1);
}

int64_t turn_dx(const TurnSettings& settings, int sign) {
    const int64_t counts = turn_counts(settings);
    return sign >= 0 ? counts : -counts;
}

Direction resolve_direction(Direction direction, std::mt19937& rng) {
    if (direction != Direction::Random) {
        return direction;
    }
    std::uniform_int_distribution<int> pick(0, 1);
    return pick(rng) == 0 ? Direction::Left : Direction::Right;
}

std::vector<int32_t> split_counts(int64_t total, int64_t per_event, int max_events) {
    std::vector<int32_t> out;
    if (total == 0) {
        return out;
    }
    if (per_event < 1) {
        per_event = 1;
    }
    if (max_events < 1) {
        max_events = 1;
    }

    const int64_t magnitude = std::llabs(total);
    const int64_t sign = total < 0 ? -1 : 1;

    // ceil(magnitude / per_event), clamped. When max_events is the binding
    // constraint each event simply carries more than per_event; there is no
    // way to inject a turn wider than that without more events.
    int64_t events = (magnitude + per_event - 1) / per_event;
    events = std::clamp<int64_t>(events, 1, max_events);

    const int64_t base = magnitude / events;
    const int64_t remainder = magnitude % events;

    out.reserve(static_cast<size_t>(events));
    for (int64_t i = 0; i < events; ++i) {
        const int64_t value = base + (i < remainder ? 1 : 0);
        out.push_back(static_cast<int32_t>(sign * value));
    }
    return out;
}

TurnPlan plan_turn(const TurnSettings& settings, int sign) {
    TurnPlan plan;
    plan.dxs = split_counts(turn_dx(settings, sign));
    plan.ramp_ms = std::clamp(settings.ramp_ms, kMinRampMs, kMaxRampMs);
    return plan;
}

double trimmed_counts(double counts_per_360, double step_percent, bool up) {
    const double factor = 1.0 + (up ? step_percent : -step_percent) / 100.0;
    const double result = counts_per_360 * factor;
    return std::clamp(result, kMinCountsPer360, kMaxCountsPer360);
}

}  // namespace rm
