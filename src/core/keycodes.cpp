// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/keycodes.h"

#include <algorithm>

namespace rm {
namespace {

// The table is written out in full rather than generated from ranges. It is
// long, but every entry is visible and greppable, and the string_views point
// at string literals with static storage duration.
constexpr KeyInfo kKeys[] = {
    {key::Invalid, "Invalid", "-", false},

    // Letters
    {key::KeyA, "KeyA", "A", false},
    {key::KeyA + 1, "KeyB", "B", false},
    {key::KeyA + 2, "KeyC", "C", false},
    {key::KeyA + 3, "KeyD", "D", false},
    {key::KeyA + 4, "KeyE", "E", false},
    {key::KeyA + 5, "KeyF", "F", false},
    {key::KeyA + 6, "KeyG", "G", false},
    {key::KeyA + 7, "KeyH", "H", false},
    {key::KeyA + 8, "KeyI", "I", false},
    {key::KeyA + 9, "KeyJ", "J", false},
    {key::KeyA + 10, "KeyK", "K", false},
    {key::KeyA + 11, "KeyL", "L", false},
    {key::KeyA + 12, "KeyM", "M", false},
    {key::KeyA + 13, "KeyN", "N", false},
    {key::KeyA + 14, "KeyO", "O", false},
    {key::KeyA + 15, "KeyP", "P", false},
    {key::KeyA + 16, "KeyQ", "Q", false},
    {key::KeyA + 17, "KeyR", "R", false},
    {key::KeyA + 18, "KeyS", "S", false},
    {key::KeyA + 19, "KeyT", "T", false},
    {key::KeyA + 20, "KeyU", "U", false},
    {key::KeyA + 21, "KeyV", "V", false},
    {key::KeyA + 22, "KeyW", "W", false},
    {key::KeyA + 23, "KeyX", "X", false},
    {key::KeyA + 24, "KeyY", "Y", false},
    {key::KeyA + 25, "KeyZ", "Z", false},

    // Digits
    {key::Key1, "Digit1", "1", false},
    {key::Key1 + 1, "Digit2", "2", false},
    {key::Key1 + 2, "Digit3", "3", false},
    {key::Key1 + 3, "Digit4", "4", false},
    {key::Key1 + 4, "Digit5", "5", false},
    {key::Key1 + 5, "Digit6", "6", false},
    {key::Key1 + 6, "Digit7", "7", false},
    {key::Key1 + 7, "Digit8", "8", false},
    {key::Key1 + 8, "Digit9", "9", false},
    {key::Key0, "Digit0", "0", false},

    // Editing
    {key::Enter, "Enter", "Enter", false},
    {key::Escape, "Escape", "Esc", false},
    {key::Backspace, "Backspace", "Backspace", false},
    {key::Tab, "Tab", "Tab", false},
    {key::Space, "Space", "Space", false},
    {key::Minus, "Minus", "-", false},
    {key::Equal, "Equal", "=", false},
    {key::BracketLeft, "BracketLeft", "[", false},
    {key::BracketRight, "BracketRight", "]", false},
    {key::Backslash, "Backslash", "\\", false},
    {key::Semicolon, "Semicolon", ";", false},
    {key::Quote, "Quote", "'", false},
    {key::Backquote, "Backquote", "`", false},
    {key::Comma, "Comma", ",", false},
    {key::Period, "Period", ".", false},
    {key::Slash, "Slash", "/", false},
    {key::CapsLock, "CapsLock", "Caps", false},

    // Function keys
    {key::F1, "F1", "F1", false},
    {key::F1 + 1, "F2", "F2", false},
    {key::F1 + 2, "F3", "F3", false},
    {key::F1 + 3, "F4", "F4", false},
    {key::F1 + 4, "F5", "F5", false},
    {key::F1 + 5, "F6", "F6", false},
    {key::F1 + 6, "F7", "F7", false},
    {key::F1 + 7, "F8", "F8", false},
    {key::F1 + 8, "F9", "F9", false},
    {key::F1 + 9, "F10", "F10", false},
    {key::F1 + 10, "F11", "F11", false},
    {key::F1 + 11, "F12", "F12", false},

    // Navigation
    {key::PrintScreen, "PrintScreen", "PrtSc", false},
    {key::ScrollLock, "ScrollLock", "ScrLk", false},
    {key::Pause, "Pause", "Pause", false},
    {key::Insert, "Insert", "Ins", false},
    {key::Home, "Home", "Home", false},
    {key::PageUp, "PageUp", "PgUp", false},
    {key::Delete, "Delete", "Del", false},
    {key::End, "End", "End", false},
    {key::PageDown, "PageDown", "PgDn", false},
    {key::ArrowRight, "ArrowRight", "Right", false},
    {key::ArrowLeft, "ArrowLeft", "Left", false},
    {key::ArrowDown, "ArrowDown", "Down", false},
    {key::ArrowUp, "ArrowUp", "Up", false},

    // Numpad
    {key::NumLock, "NumLock", "NumLk", false},
    {key::NumpadDivide, "NumpadDivide", "Num /", false},
    {key::NumpadMultiply, "NumpadMultiply", "Num *", false},
    {key::NumpadSubtract, "NumpadSubtract", "Num -", false},
    {key::NumpadAdd, "NumpadAdd", "Num +", false},
    {key::NumpadEnter, "NumpadEnter", "Num Enter", false},
    {key::Numpad1, "Numpad1", "Num 1", false},
    {key::Numpad1 + 1, "Numpad2", "Num 2", false},
    {key::Numpad1 + 2, "Numpad3", "Num 3", false},
    {key::Numpad1 + 3, "Numpad4", "Num 4", false},
    {key::Numpad1 + 4, "Numpad5", "Num 5", false},
    {key::Numpad1 + 5, "Numpad6", "Num 6", false},
    {key::Numpad1 + 6, "Numpad7", "Num 7", false},
    {key::Numpad1 + 7, "Numpad8", "Num 8", false},
    {key::Numpad1 + 8, "Numpad9", "Num 9", false},
    {key::Numpad0, "Numpad0", "Num 0", false},
    {key::NumpadDecimal, "NumpadDecimal", "Num .", false},

    // Modifiers
    {key::ControlLeft, "ControlLeft", "L-Ctrl", true},
    {key::ShiftLeft, "ShiftLeft", "L-Shift", true},
    {key::AltLeft, "AltLeft", "L-Alt", true},
    {key::MetaLeft, "MetaLeft", "L-Win", true},
    {key::ControlRight, "ControlRight", "R-Ctrl", true},
    {key::ShiftRight, "ShiftRight", "R-Shift", true},
    {key::AltRight, "AltRight", "R-Alt", true},
    {key::MetaRight, "MetaRight", "R-Win", true},

    // Miscellaneous
    {key::Menu, "Menu", "Menu", false},
    {key::IntlBackslash, "IntlBackslash", "Intl \\", false},
    {key::IntlRo, "IntlRo", "Ro", false},
    {key::IntlYen, "IntlYen", "Yen", false},
    {key::KanaMode, "KanaMode", "Kana", false},
    {key::Convert, "Convert", "Conv", false},
    {key::NonConvert, "NonConvert", "NoConv", false},
    {key::Lang1, "Lang1", "Lang1", false},
    {key::Lang2, "Lang2", "Lang2", false},

    // F13..F24 (present on some extended keyboards; useful as free bindings)
    {key::F13, "F13", "F13", false},
    {key::F13 + 1, "F14", "F14", false},
    {key::F13 + 2, "F15", "F15", false},
    {key::F13 + 3, "F16", "F16", false},
    {key::F13 + 4, "F17", "F17", false},
    {key::F13 + 5, "F18", "F18", false},
    {key::F13 + 6, "F19", "F19", false},
    {key::F13 + 7, "F20", "F20", false},
    {key::F13 + 8, "F21", "F21", false},
    {key::F13 + 9, "F22", "F22", false},
    {key::F13 + 10, "F23", "F23", false},
    {key::F13 + 11, "F24", "F24", false},

    // Media keys
    {key::MediaPlayPause, "MediaPlayPause", "Play", false},
    {key::MediaStop, "MediaStop", "Stop", false},
    {key::MediaTrackNext, "MediaTrackNext", "Next", false},
    {key::MediaTrackPrev, "MediaTrackPrev", "Prev", false},
    {key::VolumeMute, "VolumeMute", "Mute", false},
    {key::VolumeDown, "VolumeDown", "Vol-", false},
    {key::VolumeUp, "VolumeUp", "Vol+", false},
};

constexpr size_t kKeyCount = std::size(kKeys);

}  // namespace

const std::vector<KeyInfo>& all_keys() {
    static const std::vector<KeyInfo> table(std::begin(kKeys), std::end(kKeys));
    return table;
}

const KeyInfo* key_info(KeyId id) {
    const KeyInfo* begin = kKeys;
    const KeyInfo* end = kKeys + kKeyCount;
    const KeyInfo* it = std::lower_bound(begin, end, id,
        [](const KeyInfo& info, KeyId value) { return info.id < value; });
    if (it != end && it->id == id) {
        return it;
    }
    return nullptr;
}

std::optional<KeyId> key_from_name(std::string_view name) {
    for (const auto& info : kKeys) {
        if (info.name == name) {
            return info.id;
        }
    }
    return std::nullopt;
}

std::string_view key_name(KeyId id) {
    const KeyInfo* info = key_info(id);
    return info ? info->name : std::string_view{"Invalid"};
}

std::string_view key_label(KeyId id) {
    const KeyInfo* info = key_info(id);
    return info ? info->label : std::string_view{"-"};
}

bool key_is_modifier(KeyId id) {
    const KeyInfo* info = key_info(id);
    return info && info->is_modifier;
}

}  // namespace rm
