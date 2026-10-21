// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/config.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "core/keycodes.h"

namespace rm {
namespace {

using json = nlohmann::json;

constexpr std::array<std::string_view, 4> kModNames{"ctrl", "alt", "shift", "win"};
constexpr std::array<ModMask, 4> kModBits{ModCtrl, ModAlt, ModShift, ModWin};

json chord_to_json(const Chord& chord) {
    json out;
    out["mods"] = json::array();
    for (size_t i = 0; i < kModNames.size(); ++i) {
        if ((chord.mods & kModBits[i]) != 0) {
            out["mods"].push_back(std::string(kModNames[i]));
        }
    }
    out["key"] = std::string(key_name(chord.key));
    return out;
}

std::optional<Chord> chord_from_json(const json& value) {
    if (!value.is_object()) {
        return std::nullopt;
    }

    Chord chord;
    if (const auto mods = value.find("mods"); mods != value.end() && mods->is_array()) {
        for (const auto& entry : *mods) {
            if (!entry.is_string()) {
                return std::nullopt;
            }
            const std::string name = entry.get<std::string>();
            bool matched = false;
            for (size_t i = 0; i < kModNames.size(); ++i) {
                if (name == kModNames[i]) {
                    chord.mods |= kModBits[i];
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return std::nullopt;
            }
        }
    }

    if (const auto key = value.find("key"); key != value.end() && key->is_string()) {
        const std::string name = key->get<std::string>();
        if (const auto id = key_from_name(name); id.has_value()) {
            chord.key = *id;
        } else {
            return std::nullopt;
        }
    }

    if (!chord.valid()) {
        return std::nullopt;
    }
    return chord;
}

std::optional<double> number_field(const json& value, const char* key) {
    if (!value.is_object()) {
        return std::nullopt;
    }
    const auto it = value.find(key);
    if (it == value.end() || !it->is_number()) {
        return std::nullopt;
    }
    return it->get<double>();
}

std::optional<int> int_field(const json& value, const char* key) {
    if (!value.is_object()) {
        return std::nullopt;
    }
    const auto it = value.find(key);
    if (it == value.end() || !it->is_number_integer()) {
        return std::nullopt;
    }
    return it->get<int>();
}

std::optional<std::string> string_field(const json& value, const char* key) {
    if (!value.is_object()) {
        return std::nullopt;
    }
    const auto it = value.find(key);
    if (it == value.end() || !it->is_string()) {
        return std::nullopt;
    }
    return it->get<std::string>();
}

std::optional<bool> bool_field(const json& value, const char* key) {
    if (!value.is_object()) {
        return std::nullopt;
    }
    const auto it = value.find(key);
    if (it == value.end() || !it->is_boolean()) {
        return std::nullopt;
    }
    return it->get<bool>();
}

std::optional<Trigger> trigger_from_json_key(const json& value, const char* key) {
    const auto text = string_field(value, key);
    if (!text) {
        return std::nullopt;
    }
    return trigger_from_string(*text);
}

}  // namespace

std::vector<std::string> AppConfig::validate() const {
    std::vector<std::string> problems;

    if (!turn.trigger.valid()) {
        problems.emplace_back("The turn trigger is empty.");
    }
    if (!toggle.valid()) {
        problems.emplace_back("The toggle chord is not a valid chord.");
    }
    if (!calibration.up.valid() || !calibration.down.valid() ||
        !calibration.save.valid()) {
        problems.emplace_back("A calibration chord is not a valid chord.");
    }

    if (toggle == calibration.up) {
        problems.emplace_back("The toggle chord is the same as the calibration-up chord.");
    }
    if (toggle == calibration.down) {
        problems.emplace_back("The toggle chord is the same as the calibration-down chord.");
    }
    if (toggle == calibration.save) {
        problems.emplace_back("The toggle chord is the same as the calibration-save chord.");
    }
    if (calibration.up == calibration.down) {
        problems.emplace_back("The calibration-up and calibration-down chords are the same.");
    }

    // A keyboard trigger must not be the base key of an always-active chord:
    // the toggle would fire a turn as a side effect, and vice versa.
    if (turn.trigger.kind == TriggerKind::Keyboard) {
        const KeyId key = turn.trigger.key;
        const auto collides = [key](const Chord& chord) {
            return chord.key == key && chord.mods == ModNone;
        };
        if (collides(toggle) || collides(calibration.up) ||
            collides(calibration.down) || collides(calibration.save)) {
            problems.emplace_back(
                "The turn trigger is a plain key that is also used by a control chord.");
        }
    }

    return problems;
}

AppConfig make_default_config() {
    return AppConfig{};
}

void sanitize(AppConfig& config) {
    config.schema = kConfigSchemaVersion;

    config.turn.angle_deg =
        std::clamp(config.turn.angle_deg, kMinAngleDeg, kMaxAngleDeg);
    config.turn.dpi = std::clamp(config.turn.dpi, kMinDpi, kMaxDpi);
    config.turn.counts_per_360 =
        std::clamp(config.turn.counts_per_360, kMinCountsPer360, kMaxCountsPer360);
    config.turn.ramp_ms = std::clamp(config.turn.ramp_ms, kMinRampMs, kMaxRampMs);
    if (!config.turn.trigger.valid()) {
        config.turn.trigger = Trigger{TriggerKind::MouseX1, key::Invalid};
    }

    if (!config.toggle.valid()) {
        config.toggle = Chord{ModCtrl | ModAlt, key::KeyM};
    }
    if (!config.calibration.up.valid()) {
        config.calibration.up = Chord{ModCtrl | ModAlt, key::ArrowUp};
    }
    if (!config.calibration.down.valid()) {
        config.calibration.down = Chord{ModCtrl | ModAlt, key::ArrowDown};
    }
    if (!config.calibration.save.valid()) {
        config.calibration.save = Chord{ModCtrl | ModAlt, key::Enter};
    }
    config.calibration.step_percent =
        std::clamp(config.calibration.step_percent, 0.5, 20.0);

    if (config.general.locale != "id" && config.general.locale != "en") {
        config.general.locale = "en";
    }

    // Invalid direction text was dropped by the parser; make sure the stored
    // value is also one of the three known strings.
    Direction direction = Direction::Right;
    direction_from_name(direction_name(config.turn.direction), direction);
    config.turn.direction = direction;
}

std::string to_json(const AppConfig& config) {
    json root;
    root["schema"] = config.schema;
    root["enabled"] = config.enabled;

    json turn;
    turn["trigger"] = trigger_to_string(config.turn.trigger);
    turn["angle_deg"] = config.turn.angle_deg;
    turn["direction"] = std::string(direction_name(config.turn.direction));
    turn["dpi"] = config.turn.dpi;
    turn["counts_per_360"] = config.turn.counts_per_360;
    turn["ramp_ms"] = config.turn.ramp_ms;
    root["turn"] = std::move(turn);

    root["toggle"] = chord_to_json(config.toggle);

    json calibration;
    calibration["step_percent"] = config.calibration.step_percent;
    calibration["up"] = chord_to_json(config.calibration.up);
    calibration["down"] = chord_to_json(config.calibration.down);
    calibration["save"] = chord_to_json(config.calibration.save);
    root["calibration"] = std::move(calibration);

    json feedback;
    feedback["beep"] = config.feedback.beep;
    root["feedback"] = std::move(feedback);

    json general;
    general["start_with_windows"] = config.general.start_with_windows;
    general["start_minimized"] = config.general.start_minimized;
    general["locale"] = config.general.locale;
    root["general"] = std::move(general);

    return root.dump(2) + "\n";
}

std::optional<AppConfig> from_json(std::string_view text, std::string& error) {
    json root;
    try {
        root = json::parse(text.begin(), text.end());
    } catch (const std::exception& exception) {
        error = std::string("Invalid JSON: ") + exception.what();
        return std::nullopt;
    }
    if (!root.is_object()) {
        error = "The configuration file is not a JSON object.";
        return std::nullopt;
    }

    // A schema from a newer build may mean fields this build cannot see; a
    // second file would then silently drop them.
    if (const auto schema = int_field(root, "schema"); schema.has_value()) {
        if (*schema > kConfigSchemaVersion) {
            error = "This configuration was written by a newer version (schema " +
                    std::to_string(*schema) + ").";
            return std::nullopt;
        }
    }

    AppConfig config = make_default_config();

    if (const auto enabled = bool_field(root, "enabled"); enabled.has_value()) {
        config.enabled = *enabled;
    }

    if (const auto turn = root.find("turn"); turn != root.end()) {
        if (const auto trigger = trigger_from_json_key(*turn, "trigger");
            trigger.has_value()) {
            config.turn.trigger = *trigger;
        }
        if (const auto value = number_field(*turn, "angle_deg"); value.has_value()) {
            config.turn.angle_deg = *value;
        }
        if (const auto value = string_field(*turn, "direction"); value.has_value()) {
            Direction direction = config.turn.direction;
            if (direction_from_name(*value, direction)) {
                config.turn.direction = direction;
            }
        }
        if (const auto value = int_field(*turn, "dpi"); value.has_value()) {
            config.turn.dpi = *value;
        }
        if (const auto value = number_field(*turn, "counts_per_360");
            value.has_value()) {
            config.turn.counts_per_360 = *value;
        }
        if (const auto value = int_field(*turn, "ramp_ms"); value.has_value()) {
            config.turn.ramp_ms = *value;
        }
    }

    if (const auto toggle = root.find("toggle"); toggle != root.end()) {
        if (const auto chord = chord_from_json(*toggle); chord.has_value()) {
            config.toggle = *chord;
        }
    }

    if (const auto calibration = root.find("calibration");
        calibration != root.end()) {
        if (const auto value = number_field(*calibration, "step_percent");
            value.has_value()) {
            config.calibration.step_percent = *value;
        }
        for (const auto& [name, target] :
             std::initializer_list<std::pair<const char*, Chord*>>{
                 {"up", &config.calibration.up},
                 {"down", &config.calibration.down},
                 {"save", &config.calibration.save}}) {
            const auto chord = calibration->find(name);
            if (chord == calibration->end()) {
                continue;
            }
            if (const auto parsed = chord_from_json(*chord); parsed.has_value()) {
                *target = *parsed;
            }
        }
    }

    if (const auto feedback = root.find("feedback"); feedback != root.end()) {
        if (const auto value = bool_field(*feedback, "beep"); value.has_value()) {
            config.feedback.beep = *value;
        }
    }

    if (const auto general = root.find("general"); general != root.end()) {
        if (const auto value = bool_field(*general, "start_with_windows");
            value.has_value()) {
            config.general.start_with_windows = *value;
        }
        if (const auto value = bool_field(*general, "start_minimized");
            value.has_value()) {
            config.general.start_minimized = *value;
        }
        if (const auto value = string_field(*general, "locale");
            value.has_value()) {
            config.general.locale = *value;
        }
    }

    sanitize(config);
    return config;
}

}  // namespace rm
