// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/keycodes_win32.h"

#include <optional>

#include <windows.h>

namespace rm {
namespace win32 {
namespace {

// Scan code set 1, which is what Windows reports from the low-level hook.
struct ScanMapping {
    uint32_t scan;
    bool extended;
    KeyId key;
};

// clang-format off
constexpr ScanMapping kScanMappings[] = {
    // PrintScreen is E0 37 and is listed first so to_scan_code() picks the
    // normal key rather than the Alt+PrintScreen form further down.
    {0x37, true, key::PrintScreen},

    {0x01, false, key::Escape},
    {0x02, false, key::Key1},
    {0x03, false, key::Key1 + 1},
    {0x04, false, key::Key1 + 2},
    {0x05, false, key::Key1 + 3},
    {0x06, false, key::Key1 + 4},
    {0x07, false, key::Key1 + 5},
    {0x08, false, key::Key1 + 6},
    {0x09, false, key::Key1 + 7},
    {0x0A, false, key::Key1 + 8},
    {0x0B, false, key::Key0},
    {0x0C, false, key::Minus},
    {0x0D, false, key::Equal},
    {0x0E, false, key::Backspace},
    {0x0F, false, key::Tab},
    {0x10, false, key::KeyA + 16},  // Q
    {0x11, false, key::KeyA + 22},  // W
    {0x12, false, key::KeyA + 4},   // E
    {0x13, false, key::KeyA + 17},  // R
    {0x14, false, key::KeyA + 19},  // T
    {0x15, false, key::KeyA + 24},  // Y
    {0x16, false, key::KeyA + 20},  // U
    {0x17, false, key::KeyA + 8},   // I
    {0x18, false, key::KeyA + 14},  // O
    {0x19, false, key::KeyA + 15},  // P
    {0x1A, false, key::BracketLeft},
    {0x1B, false, key::BracketRight},
    {0x1C, false, key::Enter},
    {0x1D, false, key::ControlLeft},
    {0x1E, false, key::KeyA},       // A
    {0x1F, false, key::KeyA + 18},  // S
    {0x20, false, key::KeyA + 3},   // D
    {0x21, false, key::KeyA + 5},   // F
    {0x22, false, key::KeyA + 6},   // G
    {0x23, false, key::KeyA + 7},   // H
    {0x24, false, key::KeyA + 9},   // J
    {0x25, false, key::KeyA + 10},  // K
    {0x26, false, key::KeyA + 11},  // L
    {0x27, false, key::Semicolon},
    {0x28, false, key::Quote},
    {0x29, false, key::Backquote},
    {0x2A, false, key::ShiftLeft},
    {0x2B, false, key::Backslash},
    {0x2C, false, key::KeyA + 25},  // Z
    {0x2D, false, key::KeyA + 23},  // X
    {0x2E, false, key::KeyA + 2},   // C
    {0x2F, false, key::KeyA + 21},  // V
    {0x30, false, key::KeyA + 1},   // B
    {0x31, false, key::KeyA + 13},  // N
    {0x32, false, key::KeyA + 12},  // M
    {0x33, false, key::Comma},
    {0x34, false, key::Period},
    {0x35, false, key::Slash},
    {0x36, false, key::ShiftRight},
    {0x37, false, key::NumpadMultiply},
    {0x38, false, key::AltLeft},
    {0x39, false, key::Space},
    {0x3A, false, key::CapsLock},
    {0x3B, false, key::F1},
    {0x3C, false, key::F1 + 1},
    {0x3D, false, key::F1 + 2},
    {0x3E, false, key::F1 + 3},
    {0x3F, false, key::F1 + 4},
    {0x40, false, key::F1 + 5},
    {0x41, false, key::F1 + 6},
    {0x42, false, key::F1 + 7},
    {0x43, false, key::F1 + 8},
    {0x44, false, key::F1 + 9},
    {0x45, false, key::NumLock},  // shares 0x45 with Pause; see from_scan_code
    {0x46, false, key::ScrollLock},
    {0x47, false, key::Numpad1 + 6},  // Numpad7
    {0x48, false, key::Numpad1 + 7},  // Numpad8
    {0x49, false, key::Numpad1 + 8},  // Numpad9
    {0x4A, false, key::NumpadSubtract},
    {0x4B, false, key::Numpad1 + 3},  // Numpad4
    {0x4C, false, key::Numpad1 + 4},  // Numpad5
    {0x4D, false, key::Numpad1 + 5},  // Numpad6
    {0x4E, false, key::NumpadAdd},
    {0x4F, false, key::Numpad1},
    {0x50, false, key::Numpad1 + 1},
    {0x51, false, key::Numpad1 + 2},
    {0x52, false, key::Numpad0},
    {0x53, false, key::NumpadDecimal},
    {0x54, false, key::PrintScreen},  // Alt+PrintScreen (SysRq)
    {0x56, false, key::IntlBackslash},
    {0x57, false, key::F1 + 10},
    {0x58, false, key::F1 + 11},
    // F13..F23 are non-extended 0x64..0x6E in scan code set 1; F24 is 0x76.
    {0x64, false, key::F13},
    {0x65, false, key::F13 + 1},
    {0x66, false, key::F13 + 2},
    {0x67, false, key::F13 + 3},
    {0x68, false, key::F13 + 4},
    {0x69, false, key::F13 + 5},
    {0x6A, false, key::F13 + 6},
    {0x6B, false, key::F13 + 7},
    {0x6C, false, key::F13 + 8},
    {0x6D, false, key::F13 + 9},
    {0x6E, false, key::F13 + 10},
    {0x76, false, key::F24},
    {0x73, false, key::IntlBackslash},  // Brazilian ABNT2

    // Extended keys
    {0x1C, true, key::NumpadEnter},
    {0x1D, true, key::ControlRight},
    {0x35, true, key::NumpadDivide},
    {0x38, true, key::AltRight},
    {0x46, true, key::Pause},  // Ctrl+Break; Pause itself is E1-prefixed
    {0x47, true, key::Home},
    {0x48, true, key::ArrowUp},
    {0x49, true, key::PageUp},
    {0x4B, true, key::ArrowLeft},
    {0x4D, true, key::ArrowRight},
    {0x4F, true, key::End},
    {0x50, true, key::ArrowDown},
    {0x51, true, key::PageDown},
    {0x52, true, key::Insert},
    {0x53, true, key::Delete},
    {0x5B, true, key::MetaLeft},
    {0x5C, true, key::MetaRight},
    {0x5D, true, key::Menu},
    {0x70, false, key::KanaMode},
    {0x79, false, key::Convert},
    {0x7B, false, key::NonConvert},
    {0x7D, false, key::IntlYen},
};
// clang-format on

KeyId from_vk_switch(uint32_t vk) {
    // Letters and digits are contiguous in the VK space.
    if (vk >= 'A' && vk <= 'Z') {
        return static_cast<KeyId>(key::KeyA + (vk - 'A'));
    }
    if (vk >= '0' && vk <= '9') {
        if (vk == '0') {
            return key::Key0;
        }
        return static_cast<KeyId>(key::Key1 + (vk - '1'));
    }
    if (vk >= VK_F1 && vk <= VK_F12) {
        return static_cast<KeyId>(key::F1 + (vk - VK_F1));
    }
    if (vk >= VK_F13 && vk <= VK_F24) {
        return static_cast<KeyId>(key::F13 + (vk - VK_F13));
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        if (vk == VK_NUMPAD0) {
            return key::Numpad0;
        }
        return static_cast<KeyId>(key::Numpad1 + (vk - VK_NUMPAD1));
    }

    switch (vk) {
        case VK_CANCEL: return key::Pause;
        case VK_BACK: return key::Backspace;
        case VK_TAB: return key::Tab;
        case VK_CLEAR: return key::Numpad1 + 4;
        case VK_RETURN: return key::Enter;
        case VK_SHIFT: return key::ShiftLeft;
        case VK_CONTROL: return key::ControlLeft;
        case VK_MENU: return key::AltLeft;
        case VK_PAUSE: return key::Pause;
        case VK_CAPITAL: return key::CapsLock;
        case VK_ESCAPE: return key::Escape;
        case VK_SPACE: return key::Space;
        case VK_PRIOR: return key::PageUp;
        case VK_NEXT: return key::PageDown;
        case VK_END: return key::End;
        case VK_HOME: return key::Home;
        case VK_LEFT: return key::ArrowLeft;
        case VK_UP: return key::ArrowUp;
        case VK_RIGHT: return key::ArrowRight;
        case VK_DOWN: return key::ArrowDown;
        case VK_SNAPSHOT: return key::PrintScreen;
        case VK_INSERT: return key::Insert;
        case VK_DELETE: return key::Delete;
        case VK_LWIN: return key::MetaLeft;
        case VK_RWIN: return key::MetaRight;
        case VK_APPS: return key::Menu;
        case VK_MULTIPLY: return key::NumpadMultiply;
        case VK_ADD: return key::NumpadAdd;
        case VK_SUBTRACT: return key::NumpadSubtract;
        case VK_DECIMAL: return key::NumpadDecimal;
        case VK_DIVIDE: return key::NumpadDivide;
        case VK_NUMLOCK: return key::NumLock;
        case VK_SCROLL: return key::ScrollLock;
        case VK_LSHIFT: return key::ShiftLeft;
        case VK_RSHIFT: return key::ShiftRight;
        case VK_LCONTROL: return key::ControlLeft;
        case VK_RCONTROL: return key::ControlRight;
        case VK_LMENU: return key::AltLeft;
        case VK_RMENU: return key::AltRight;
        case VK_OEM_1: return key::Semicolon;
        case VK_OEM_PLUS: return key::Equal;
        case VK_OEM_COMMA: return key::Comma;
        case VK_OEM_MINUS: return key::Minus;
        case VK_OEM_PERIOD: return key::Period;
        case VK_OEM_2: return key::Slash;
        case VK_OEM_3: return key::Backquote;
        case VK_OEM_4: return key::BracketLeft;
        case VK_OEM_5: return key::Backslash;
        case VK_OEM_6: return key::BracketRight;
        case VK_OEM_7: return key::Quote;
        case VK_OEM_102: return key::IntlBackslash;
        case VK_VOLUME_MUTE: return key::VolumeMute;
        case VK_VOLUME_DOWN: return key::VolumeDown;
        case VK_VOLUME_UP: return key::VolumeUp;
        case VK_MEDIA_NEXT_TRACK: return key::MediaTrackNext;
        case VK_MEDIA_PREV_TRACK: return key::MediaTrackPrev;
        case VK_MEDIA_STOP: return key::MediaStop;
        case VK_MEDIA_PLAY_PAUSE: return key::MediaPlayPause;
        case VK_KANA: return key::KanaMode;
        case VK_HANJA: return key::Lang2;
        case VK_CONVERT: return key::Convert;
        case VK_NONCONVERT: return key::NonConvert;
        default: return key::Invalid;
    }
}

}  // namespace

KeyTranslation from_scan_code(uint32_t scan_code, bool extended, uint32_t vk) {
    const uint32_t scan = scan_code & 0xFFu;

    // Pause and NumLock share scan code 0x45 and neither sets the extended
    // flag, so the scan table cannot tell them apart on its own. The virtual
    // key can.
    if (scan == 0x45 && !extended && vk == VK_PAUSE) {
        return {key::Pause, false, scan};
    }

    for (const auto& mapping : kScanMappings) {
        if (mapping.scan == scan && mapping.extended == extended) {
            return {mapping.key, extended, mapping.scan};
        }
    }

    // Some remappers and virtual keyboards report a VK but a bogus scan code.
    // Falling back to the VK keeps those users working; layout dependent
    // bindings are the price, and only for these unusual devices.
    return {from_vk_switch(vk), extended, scan};
}

KeyId from_virtual_key(uint32_t vk) {
    return from_vk_switch(vk);
}

KeyTranslation to_scan_code(KeyId target) {
    // Pause is sent as a virtual key. Its own scan code (E1 1D 45) is not a
    // plain set-1 sequence, and 0xE0 0x46 is Ctrl+Break, a different key.
    if (target == key::Pause) {
        return {};
    }
    for (const auto& mapping : kScanMappings) {
        if (mapping.key == target) {
            return {mapping.key, mapping.extended, mapping.scan};
        }
    }
    return {};
}

std::optional<uint32_t> to_virtual_key(KeyId target) {
    if (target >= key::KeyA && target <= key::KeyZ) {
        return static_cast<uint32_t>('A' + (target - key::KeyA));
    }
    if (target >= key::Key1 && target <= key::Key1 + 8) {
        return static_cast<uint32_t>('1' + (target - key::Key1));
    }
    if (target == key::Key0) {
        return static_cast<uint32_t>('0');
    }
    if (target >= key::F1 && target <= key::F1 + 11) {
        return VK_F1 + (target - key::F1);
    }
    if (target >= key::F13 && target <= key::F24) {
        return VK_F13 + (target - key::F13);
    }
    if (target >= key::Numpad1 && target <= key::Numpad9) {
        return VK_NUMPAD1 + (target - key::Numpad1);
    }
    if (target == key::Numpad0) {
        return VK_NUMPAD0;
    }

    switch (target) {
        case key::Backspace: return VK_BACK;
        case key::Tab: return VK_TAB;
        case key::Enter: return VK_RETURN;
        case key::ShiftLeft: return VK_LSHIFT;
        case key::ShiftRight: return VK_RSHIFT;
        case key::ControlLeft: return VK_LCONTROL;
        case key::ControlRight: return VK_RCONTROL;
        case key::AltLeft: return VK_LMENU;
        case key::AltRight: return VK_RMENU;
        case key::Pause: return VK_PAUSE;
        case key::CapsLock: return VK_CAPITAL;
        case key::Escape: return VK_ESCAPE;
        case key::Space: return VK_SPACE;
        case key::PageUp: return VK_PRIOR;
        case key::PageDown: return VK_NEXT;
        case key::End: return VK_END;
        case key::Home: return VK_HOME;
        case key::ArrowLeft: return VK_LEFT;
        case key::ArrowUp: return VK_UP;
        case key::ArrowRight: return VK_RIGHT;
        case key::ArrowDown: return VK_DOWN;
        case key::PrintScreen: return VK_SNAPSHOT;
        case key::Insert: return VK_INSERT;
        case key::Delete: return VK_DELETE;
        case key::MetaLeft: return VK_LWIN;
        case key::MetaRight: return VK_RWIN;
        case key::Menu: return VK_APPS;
        case key::NumpadMultiply: return VK_MULTIPLY;
        case key::NumpadAdd: return VK_ADD;
        case key::NumpadSubtract: return VK_SUBTRACT;
        case key::NumpadDecimal: return VK_DECIMAL;
        case key::NumpadDivide: return VK_DIVIDE;
        case key::NumLock: return VK_NUMLOCK;
        case key::ScrollLock: return VK_SCROLL;
        case key::Semicolon: return VK_OEM_1;
        case key::Equal: return VK_OEM_PLUS;
        case key::Comma: return VK_OEM_COMMA;
        case key::Minus: return VK_OEM_MINUS;
        case key::Period: return VK_OEM_PERIOD;
        case key::Slash: return VK_OEM_2;
        case key::Backquote: return VK_OEM_3;
        case key::BracketLeft: return VK_OEM_4;
        case key::Backslash: return VK_OEM_5;
        case key::BracketRight: return VK_OEM_6;
        case key::Quote: return VK_OEM_7;
        case key::IntlBackslash: return VK_OEM_102;
        case key::VolumeMute: return VK_VOLUME_MUTE;
        case key::VolumeDown: return VK_VOLUME_DOWN;
        case key::VolumeUp: return VK_VOLUME_UP;
        case key::MediaTrackNext: return VK_MEDIA_NEXT_TRACK;
        case key::MediaTrackPrev: return VK_MEDIA_PREV_TRACK;
        case key::MediaStop: return VK_MEDIA_STOP;
        case key::MediaPlayPause: return VK_MEDIA_PLAY_PAUSE;
        case key::KanaMode: return VK_KANA;
        case key::Lang2: return VK_HANJA;
        case key::Convert: return VK_CONVERT;
        case key::NonConvert: return VK_NONCONVERT;
        default: return std::nullopt;
    }
}

}  // namespace win32
}  // namespace rm
