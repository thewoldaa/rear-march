// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/chord.h"

#include <array>
#include <cctype>
#include <vector>

namespace rm {
namespace {

struct ModifierName {
    ModMask bit;
    std::string_view name;
    std::string_view canonical;
};

// Order here defines the canonical formatting order.
constexpr std::array<ModifierName, 4> kModifierNames{{
    {ModCtrl, "ctrl", "Ctrl"},
    {ModAlt, "alt", "Alt"},
    {ModShift, "shift", "Shift"},
    {ModWin, "win", "Win"},
}};

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

bool equals_ignore_case(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

std::vector<std::string_view> split_on_plus(std::string_view text) {
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t plus = text.find('+', start);
        const size_t end = plus == std::string_view::npos ? text.size() : plus;
        parts.push_back(trim(text.substr(start, end - start)));
        if (plus == std::string_view::npos) {
            break;
        }
        start = plus + 1;
    }
    return parts;
}

}  // namespace

std::string format_modifiers(ModMask mods) {
    std::string out;
    for (const auto& mod : kModifierNames) {
        if ((mods & mod.bit) != 0) {
            if (!out.empty()) {
                out += " + ";
            }
            out += mod.canonical;
        }
    }
    return out;
}

std::string format_chord(const Chord& chord) {
    if (!chord.valid()) {
        return {};
    }

    std::string out = format_modifiers(chord.mods);
    if (!out.empty()) {
        out += " + ";
    }
    out += key_name(chord.key);
    return out;
}

std::optional<Chord> parse_chord(std::string_view text) {
    text = trim(text);
    if (text.empty()) {
        return std::nullopt;
    }

    Chord chord;
    int modifier_count = 0;

    for (std::string_view part : split_on_plus(text)) {
        if (part.empty()) {
            return std::nullopt;
        }

        bool matched_modifier = false;
        for (const auto& mod : kModifierNames) {
            if (equals_ignore_case(part, mod.name)) {
                if ((chord.mods & mod.bit) != 0) {
                    return std::nullopt;  // duplicate modifier
                }
                chord.mods |= mod.bit;
                ++modifier_count;
                matched_modifier = true;
                break;
            }
        }
        if (matched_modifier) {
            continue;
        }

        // Not a modifier, so it must be the base key. Only one is allowed.
        if (chord.key != key::Invalid) {
            return std::nullopt;
        }

        // Key names are matched case insensitively so a hand-edited config
        // with "keym" still loads, while the canonical spelling stays "KeyM"
        // for output.
        std::optional<KeyId> id;
        for (const auto& info : all_keys()) {
            if (equals_ignore_case(part, info.name)) {
                id = info.id;
                break;
            }
        }
        if (!id) {
            return std::nullopt;
        }
        chord.key = *id;
    }

    if (modifier_count > kMaxChordModifiers) {
        return std::nullopt;
    }
    if (!chord.valid()) {
        return std::nullopt;
    }
    return chord;
}

ModMask modifier_bit(KeyId key) {
    switch (key) {
        case key::ControlLeft:
        case key::ControlRight:
            return ModCtrl;
        case key::AltLeft:
        case key::AltRight:
            return ModAlt;
        case key::ShiftLeft:
        case key::ShiftRight:
            return ModShift;
        case key::MetaLeft:
        case key::MetaRight:
            return ModWin;
        default:
            return ModNone;
    }
}

bool is_modifier_key(KeyId key) {
    return modifier_bit(key) != ModNone;
}

}  // namespace rm
