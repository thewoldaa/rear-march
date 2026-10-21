// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace rm {

// Canonical, layout independent key identifiers.
//
// Keys are stored by physical position, never by virtual-key code. A user who
// switches their keyboard layout keeps the same physical keys; VK codes change
// underneath them, canonical ids do not.
//
// Naming follows the W3C UI Events `KeyboardEvent.code` convention where a
// sensible mapping exists, so the config format is familiar to anyone who has
// written a web keyboard handler.
using KeyId = uint16_t;

namespace key {
inline constexpr KeyId Invalid = 0;

// Letters: KeyA..KeyZ are contiguous.
inline constexpr KeyId KeyA = 4;
inline constexpr KeyId KeyB = KeyA + 1;
inline constexpr KeyId KeyC = KeyA + 2;
inline constexpr KeyId KeyD = KeyA + 3;
inline constexpr KeyId KeyE = KeyA + 4;
inline constexpr KeyId KeyF = KeyA + 5;
inline constexpr KeyId KeyG = KeyA + 6;
inline constexpr KeyId KeyH = KeyA + 7;
inline constexpr KeyId KeyI = KeyA + 8;
inline constexpr KeyId KeyJ = KeyA + 9;
inline constexpr KeyId KeyK = KeyA + 10;
inline constexpr KeyId KeyL = KeyA + 11;
inline constexpr KeyId KeyM = KeyA + 12;
inline constexpr KeyId KeyN = KeyA + 13;
inline constexpr KeyId KeyO = KeyA + 14;
inline constexpr KeyId KeyP = KeyA + 15;
inline constexpr KeyId KeyQ = KeyA + 16;
inline constexpr KeyId KeyR = KeyA + 17;
inline constexpr KeyId KeyS = KeyA + 18;
inline constexpr KeyId KeyT = KeyA + 19;
inline constexpr KeyId KeyU = KeyA + 20;
inline constexpr KeyId KeyV = KeyA + 21;
inline constexpr KeyId KeyW = KeyA + 22;
inline constexpr KeyId KeyX = KeyA + 23;
inline constexpr KeyId KeyY = KeyA + 24;
inline constexpr KeyId KeyZ = KeyA + 25;

// Digits: Digit1..Digit9 then Digit0.
inline constexpr KeyId Key1 = 30;
inline constexpr KeyId Key0 = 39;

// Editing and punctuation
inline constexpr KeyId Enter = 40;
inline constexpr KeyId Escape = 41;
inline constexpr KeyId Backspace = 42;
inline constexpr KeyId Tab = 43;
inline constexpr KeyId Space = 44;
inline constexpr KeyId Minus = 45;
inline constexpr KeyId Equal = 46;
inline constexpr KeyId BracketLeft = 47;
inline constexpr KeyId BracketRight = 48;
inline constexpr KeyId Backslash = 49;
inline constexpr KeyId Semicolon = 50;
inline constexpr KeyId Quote = 51;
inline constexpr KeyId Backquote = 52;
inline constexpr KeyId Comma = 53;
inline constexpr KeyId Period = 54;
inline constexpr KeyId Slash = 55;
inline constexpr KeyId CapsLock = 56;

// Function keys
inline constexpr KeyId F1 = 58;
inline constexpr KeyId F2 = 59;
inline constexpr KeyId F3 = 60;
inline constexpr KeyId F4 = 61;
inline constexpr KeyId F5 = 62;
inline constexpr KeyId F6 = 63;
inline constexpr KeyId F7 = 64;
inline constexpr KeyId F8 = 65;
inline constexpr KeyId F9 = 66;
inline constexpr KeyId F10 = 67;
inline constexpr KeyId F11 = 68;
inline constexpr KeyId F12 = 69;

// Navigation
inline constexpr KeyId PrintScreen = 70;
inline constexpr KeyId ScrollLock = 71;
inline constexpr KeyId Pause = 72;
inline constexpr KeyId Insert = 73;
inline constexpr KeyId Home = 74;
inline constexpr KeyId PageUp = 75;
inline constexpr KeyId Delete = 76;
inline constexpr KeyId End = 77;
inline constexpr KeyId PageDown = 78;
inline constexpr KeyId ArrowRight = 79;
inline constexpr KeyId ArrowLeft = 80;
inline constexpr KeyId ArrowDown = 81;
inline constexpr KeyId ArrowUp = 82;

// Numpad
inline constexpr KeyId NumLock = 83;
inline constexpr KeyId NumpadDivide = 84;
inline constexpr KeyId NumpadMultiply = 85;
inline constexpr KeyId NumpadSubtract = 86;
inline constexpr KeyId NumpadAdd = 87;
inline constexpr KeyId NumpadEnter = 88;
inline constexpr KeyId Numpad1 = 89;
inline constexpr KeyId Numpad2 = 90;
inline constexpr KeyId Numpad3 = 91;
inline constexpr KeyId Numpad4 = 92;
inline constexpr KeyId Numpad5 = 93;
inline constexpr KeyId Numpad6 = 94;
inline constexpr KeyId Numpad7 = 95;
inline constexpr KeyId Numpad8 = 96;
inline constexpr KeyId Numpad9 = 97;
inline constexpr KeyId Numpad0 = 98;
inline constexpr KeyId NumpadDecimal = 99;

// Modifiers
inline constexpr KeyId ControlLeft = 100;
inline constexpr KeyId ShiftLeft = 101;
inline constexpr KeyId AltLeft = 102;
inline constexpr KeyId MetaLeft = 103;
inline constexpr KeyId ControlRight = 104;
inline constexpr KeyId ShiftRight = 105;
inline constexpr KeyId AltRight = 106;
inline constexpr KeyId MetaRight = 107;

// Miscellaneous
inline constexpr KeyId Menu = 108;  // the "context menu" / Apps key
inline constexpr KeyId IntlBackslash = 109;
inline constexpr KeyId IntlRo = 110;
inline constexpr KeyId IntlYen = 111;
inline constexpr KeyId KanaMode = 112;
inline constexpr KeyId Convert = 113;
inline constexpr KeyId NonConvert = 114;
inline constexpr KeyId Lang1 = 115;
inline constexpr KeyId Lang2 = 116;

// Extended function keys
inline constexpr KeyId F13 = 117;
inline constexpr KeyId F24 = 128;  // F13..F24 contiguous

// Media and system keys (best effort; not present on all keyboards)
inline constexpr KeyId MediaPlayPause = 129;
inline constexpr KeyId MediaStop = 130;
inline constexpr KeyId MediaTrackNext = 131;
inline constexpr KeyId MediaTrackPrev = 132;
inline constexpr KeyId VolumeMute = 133;
inline constexpr KeyId VolumeDown = 134;
inline constexpr KeyId VolumeUp = 135;

inline constexpr KeyId Count = 136;
}  // namespace key

// A single physical key description.
struct KeyInfo {
    KeyId id;
    std::string_view name;   // canonical name, e.g. "Numpad0"
    std::string_view label;  // short display label, e.g. "Num 0"
    bool is_modifier;
};

// Returns nullptr for unknown ids.
const KeyInfo* key_info(KeyId id);

// Lookup by canonical name (case sensitive, matches KeyInfo::name).
std::optional<KeyId> key_from_name(std::string_view name);

// Canonical name for an id. Returns "Invalid" for unknown ids.
std::string_view key_name(KeyId id);

// Human readable label for an id.
std::string_view key_label(KeyId id);

bool key_is_modifier(KeyId id);

// All keys, ordered by id, for populating UI lists.
const std::vector<KeyInfo>& all_keys();

}  // namespace rm
