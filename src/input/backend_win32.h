// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>
#include <random>
#include <thread>

#include <windows.h>

#include "core/chord.h"

namespace rm {

class ActionMailbox;
class RuntimeStore;
class TurnWorker;

// Installs WH_KEYBOARD_LL and WH_MOUSE_LL on a dedicated thread that pumps
// messages, and classifies every event against the current runtime snapshot.
//
// The hooks are observers: they never swallow input, so the game keeps seeing
// every key and button exactly as before. Classification is limited to the
// trigger, the toggle chord, and the calibration chords; everything else falls
// straight through. Nothing in a callback allocates or locks — see
// RuntimeStore and ActionMailbox.
class InputBackend {
public:
    InputBackend(RuntimeStore& store, ActionMailbox& actions, TurnWorker& worker);
    ~InputBackend();

    InputBackend(const InputBackend&) = delete;
    InputBackend& operator=(const InputBackend&) = delete;

    // Installs the hooks and waits until they are ready (or failed).
    bool start();
    void stop();
    bool running() const { return running_.load(); }
    bool x1_down() const { return x1_down_.load(); }
    bool x2_down() const { return x2_down_.load(); }
    uint64_t turns_requested() const { return turns_requested_.load(); }

private:
    void hook_thread_main();

    // Returns true when the event was consumed by a control chord; the hook
    // still passes it through, so the return value only affects bookkeeping.
    bool on_key(uint32_t vk, bool down);
    bool on_mouse_button(int xbutton, bool down);

    void fire_trigger();
    bool chord_matches(const Chord& chord, uint32_t vk, ModMask mods) const;

    static LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam);

    RuntimeStore& store_;
    ActionMailbox& actions_;
    TurnWorker& worker_;

    std::thread thread_;
    std::atomic<DWORD> thread_id_{0};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> hook_ready_{false};

    void* keyboard_hook_ = nullptr;
    void* mouse_hook_ = nullptr;

    // Auto-repeat and held-state suppression. One slot per control chord, plus
    // one for the trigger key and one per mouse button.
    std::atomic<bool> toggle_down_{false};
    std::atomic<bool> calib_up_down_{false};
    std::atomic<bool> calib_down_down_{false};
    std::atomic<bool> calib_save_down_{false};
    std::atomic<bool> key_trigger_down_{false};
    std::atomic<bool> x1_down_{false};
    std::atomic<bool> x2_down_{false};

    // Random direction. Owned by the hook thread; seeded once at start.
    std::mt19937 rng_;

    std::atomic<uint64_t> turns_requested_{0};
};

}  // namespace rm
