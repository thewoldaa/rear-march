// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "core/keycodes.h"

namespace rm {

// Modifier set. Left and right variants collapse to the same bit: a user who
// binds "Ctrl + Alt + M" does not want it to stop working because they pressed
// the right-hand Ctrl.
enum Modifier : uint8_t {
    ModNone = 0,
    ModCtrl = 1u << 0,
    ModAlt = 1u << 1,
    ModShift = 1u << 2,
    ModWin = 1u << 3,
};

using ModMask = uint8_t;

inline constexpr ModMask kAllModifiers = ModCtrl | ModAlt | ModShift | ModWin;

// Maximum number of modifiers in a chord: every modifier bit we define.
inline constexpr int kMaxChordModifiers = 4;

// A key combination: zero or more modifiers plus exactly one base key.
//
// `key` is a canonical KeyId. A chord whose base key is itself a modifier is
// invalid; "Ctrl + Ctrl" is not a meaningful binding.
struct Chord {
    ModMask mods = ModNone;
    KeyId key = key::Invalid;

    bool valid() const {
        return key != key::Invalid && !key_is_modifier(key);
    }

    bool operator==(const Chord&) const = default;
};

// Formats a chord as "Ctrl + Alt + KeyM", in a fixed modifier order:
// Ctrl, Alt, Shift, Win. Returns an empty string for invalid chords.
std::string format_chord(const Chord& chord);

// Formats just the modifiers, for example "Ctrl + Alt". Returns an empty
// string when no modifier is set.
std::string format_modifiers(ModMask mods);

// Parses "Ctrl+Alt+KeyM" (whitespace around separators is ignored, modifier
// names are case insensitive). Returns nullopt when the string does not
// describe a valid chord.
std::optional<Chord> parse_chord(std::string_view text);

// Returns the modifier mask bit for a modifier key, or 0 for non-modifiers.
ModMask modifier_bit(KeyId key);

// True when `key` is one of the physical keys that maps to a modifier bit.
bool is_modifier_key(KeyId key);

}  // namespace rm
