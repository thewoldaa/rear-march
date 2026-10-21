// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/backend_win32.h"

#include <windows.h>

#include <random>

#include "core/action_mailbox.h"
#include "core/runtime_config.h"
#include "core/turn.h"
#include "input/injection.h"
#include "input/keycodes_win32.h"
#include "input/turn_worker.h"

namespace rm {
namespace {

InputBackend* g_backend = nullptr;

// Reads modifier state directly from the OS. Sampling rather than tracking
// means the mask is right even if the user pressed Ctrl before the app
// started, and it costs four GetAsyncKeyState calls that never allocate.
ModMask current_modifiers() {
    ModMask mods = ModNone;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= ModCtrl;
    if (GetAsyncKeyState(VK_MENU) & 0x8000) mods |= ModAlt;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) mods |= ModShift;
    if (GetAsyncKeyState(VK_LWIN) & 0x8000) mods |= ModWin;
    if (GetAsyncKeyState(VK_RWIN) & 0x8000) mods |= ModWin;
    return mods;
}

// Maps a virtual-key code to a canonical key for comparison against the
// configured trigger and control chords. The hook reports a VK, which is
// layout dependent; that is acceptable here because the control chords are
// physical keys on the user's own layout and the config stores the same
// canonical name the picker recorded.
KeyId key_from_vk(uint32_t vk) {
    return win32::from_virtual_key(vk);
}

}  // namespace

InputBackend* active_backend() {
    return g_backend;
}

InputBackend::InputBackend(RuntimeStore& store, ActionMailbox& actions,
                           TurnWorker& worker)
    : store_(store), actions_(actions), worker_(worker) {
    std::random_device device;
    rng_.seed((static_cast<uint64_t>(device()) << 32) ^ device());
}

InputBackend::~InputBackend() {
    stop();
}

bool InputBackend::start() {
    if (thread_.joinable()) {
        return running_.load();
    }

    stop_requested_.store(false);
    hook_ready_.store(false);
    thread_ = std::thread([this] { hook_thread_main(); });

    // Wait for the hook to report success or failure, so the caller knows
    // whether the application is actually usable.
    for (int i = 0; i < 200 && !hook_ready_.load(); ++i) {
        Sleep(5);
    }
    return running_.load();
}

void InputBackend::stop() {
    if (!thread_.joinable()) {
        return;
    }

    stop_requested_.store(true);
    const DWORD id = thread_id_.load();
    if (id != 0) {
        PostThreadMessageW(id, WM_QUIT, 0, 0);
    }
    thread_.join();

    running_.store(false);
    hook_ready_.store(false);
    g_backend = nullptr;
}

void InputBackend::hook_thread_main() {
    thread_id_.store(GetCurrentThreadId());
    g_backend = this;

    // A message queue must exist before SetWindowsHookEx can post to it.
    MSG msg{};
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    keyboard_hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, &InputBackend::keyboard_hook_proc,
                                       GetModuleHandleW(nullptr), 0);
    mouse_hook_ = SetWindowsHookExW(WH_MOUSE_LL, &InputBackend::mouse_hook_proc,
                                    GetModuleHandleW(nullptr), 0);

    // The mouse hook is optional: a keyboard trigger still works without it.
    // The keyboard hook is not: without it nothing fires at all.
    const bool ok = keyboard_hook_ != nullptr;
    running_.store(ok);
    hook_ready_.store(true);
    if (!ok) {
        thread_id_.store(0);
        return;
    }

    while (!stop_requested_.load()) {
        // A timed wait rather than a blocking GetMessage: stop() joins this
        // thread, and a WM_QUIT posted before the message queue existed is
        // lost. Waking a few times a second guarantees the stop request is
        // observed. The thread is idle otherwise; the hook callbacks run on
        // Windows' hook dispatch.
        const DWORD wait = MsgWaitForMultipleObjectsEx(
            0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait != WAIT_OBJECT_0) {
            continue;
        }
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                stop_requested_.store(true);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (mouse_hook_) {
        UnhookWindowsHookEx(static_cast<HHOOK>(mouse_hook_));
        mouse_hook_ = nullptr;
    }
    if (keyboard_hook_) {
        UnhookWindowsHookEx(static_cast<HHOOK>(keyboard_hook_));
        keyboard_hook_ = nullptr;
    }
    thread_id_.store(0);
}

bool InputBackend::chord_matches(const Chord& chord, uint32_t vk, ModMask mods) const {
    return chord.valid() && chord.mods == mods && key_from_vk(vk) == chord.key;
}

bool InputBackend::on_key(uint32_t vk, bool down) {
    const RuntimeConfig* config = store_.current();
    if (!config) {
        return false;
    }

    // While a picker in the dashboard is recording, nothing is interpreted:
    // the key being pressed may be the one the user wants to bind, and firing
    // a turn for it would be a surprise.
    if (config->capture_active) {
        return false;
    }

    const ModMask mods = current_modifiers();

    // The control chords are always active, even while the turn itself is
    // disabled or the workstation is locked, so the user can always turn the
    // application back on (or finish calibration) without a keyboard that is
    // still wired to the game.
    if (chord_matches(config->toggle, vk, mods)) {
        if (down) {
            if (!toggle_down_.exchange(true, std::memory_order_relaxed)) {
                actions_.post(AppAction::Toggle);
            }
        } else {
            toggle_down_.store(false, std::memory_order_relaxed);
        }
        return true;
    }

    // Auto-repeat of the toggle key while held: consume it, so releasing the
    // modifiers does not leak an auto-repeat to the foreground application.
    if (down && toggle_down_.load(std::memory_order_relaxed) &&
        key_from_vk(vk) == config->toggle.key) {
        return true;
    }

    if (chord_matches(config->calib_up, vk, mods)) {
        if (down && !calib_up_down_.exchange(true, std::memory_order_relaxed)) {
            actions_.post(AppAction::CalibrateUp);
        }
        if (!down) calib_up_down_.store(false, std::memory_order_relaxed);
        return true;
    }
    if (chord_matches(config->calib_down, vk, mods)) {
        if (down && !calib_down_down_.exchange(true, std::memory_order_relaxed)) {
            actions_.post(AppAction::CalibrateDown);
        }
        if (!down) calib_down_down_.store(false, std::memory_order_relaxed);
        return true;
    }
    if (chord_matches(config->calib_save, vk, mods)) {
        if (down && !calib_save_down_.exchange(true, std::memory_order_relaxed)) {
            actions_.post(AppAction::CalibrateSave);
        }
        if (!down) calib_save_down_.store(false, std::memory_order_relaxed);
        return true;
    }

    // The trigger. Only a keyboard trigger is handled here; mouse buttons are
    // reported by the mouse hook. The modifier set must be empty: a trigger
    // that fired while Ctrl was held would turn the camera every time the
    // player crouched.
    if (config->turn.trigger.kind == TriggerKind::Keyboard &&
        key_from_vk(vk) == config->turn.trigger.key) {
        if (mods != ModNone) {
            return false;
        }
        if (down) {
            if (!key_trigger_down_.exchange(true, std::memory_order_relaxed)) {
                fire_trigger();
            }
        } else {
            key_trigger_down_.store(false, std::memory_order_relaxed);
        }
        return true;
    }

    return false;
}

bool InputBackend::on_mouse_button(int xbutton, bool down) {
    const RuntimeConfig* config = store_.current();
    if (!config || !config->enabled || config->locked || config->capture_active) {
        return false;
    }
    if (config->turn.trigger.kind == TriggerKind::Keyboard) {
        return false;
    }

    const bool is_x1 = xbutton == XBUTTON1;
    const bool matches = (is_x1 && config->turn.trigger.kind == TriggerKind::MouseX1) ||
                         (!is_x1 && config->turn.trigger.kind == TriggerKind::MouseX2);
    if (!matches) {
        return false;
    }

    if (down) {
        if (!(is_x1 ? x1_down_ : x2_down_).exchange(true, std::memory_order_relaxed)) {
            fire_trigger();
        }
    } else {
        (is_x1 ? x1_down_ : x2_down_).store(false, std::memory_order_relaxed);
    }
    return true;
}

void InputBackend::fire_trigger() {
    const RuntimeConfig* config = store_.current();
    if (!config || !config->enabled || config->locked || config->capture_active) {
        return;
    }

    const Direction direction = resolve_direction(config->turn.direction, rng_);
    const int sign = direction == Direction::Left ? -1 : 1;
    worker_.request_turn(sign);
    turns_requested_.fetch_add(1);
}

LRESULT CALLBACK InputBackend::keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam) {
    if (code != HC_ACTION) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    InputBackend* backend = g_backend;
    if (!backend) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    const auto* info = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lparam);

    // RearMarch's own injected keyboard events (there are none today, but a
    // future feature may add them) carry the tag and are ignored, so they can
    // never re-enter the trigger logic.
    if (info->dwExtraInfo == input::kInjectionTag) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    // Ctrl+Alt+Del and friends are delivered on the secure desktop and never
    // routed through this hook, but Win+L, Win+D, and Win+Tab are. Let the OS
    // keep them.
    const bool win_down =
        (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 ||
        (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
    if (win_down && (info->vkCode == VK_TAB || info->vkCode == 'L' ||
                     info->vkCode == 'D')) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    const bool is_down = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
    backend->on_key(info->vkCode, is_down);

    // Never swallow: the game must see every key exactly as pressed.
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

LRESULT CALLBACK InputBackend::mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam) {
    if (code != HC_ACTION) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    InputBackend* backend = g_backend;
    if (!backend) {
        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    switch (wparam) {
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP: {
            const auto* info = reinterpret_cast<const MSLLHOOKSTRUCT*>(lparam);
            if (info->dwExtraInfo == input::kInjectionTag) {
                break;
            }
            const WORD button = HIWORD(info->mouseData);
            const int xbutton = button == XBUTTON1 ? XBUTTON1 : XBUTTON2;
            backend->on_mouse_button(xbutton, wparam == WM_XBUTTONDOWN);
            break;
        }
        default:
            break;
    }

    // Never swallow: the game must see every button exactly as pressed.
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

}  // namespace rm
