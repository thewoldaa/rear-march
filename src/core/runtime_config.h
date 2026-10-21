// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "core/chord.h"
#include "core/turn.h"

namespace rm {

// The part of the configuration the hook thread and the turn worker need.
//
// It is published as an immutable snapshot: the UI thread builds a new one
// after every change, and the hook thread reads a raw pointer to the current
// one. Snapshots are retained for the lifetime of the process (see
// RuntimeStore), so a hook callback never touches a reference count, a mutex,
// or an allocation.
struct RuntimeConfig {
    bool enabled = true;
    bool locked = false;      // the workstation is locked; do nothing
    bool calibrating = false;
    // A picker in the dashboard is recording input; the trigger must not fire
    // while the user is trying to press the key they want to bind.
    bool capture_active = false;

    TurnSettings turn;

    Chord toggle{ModCtrl | ModAlt, key::KeyM};
    Chord calib_up{ModCtrl | ModAlt, key::ArrowUp};
    Chord calib_down{ModCtrl | ModAlt, key::ArrowDown};
    Chord calib_save{ModCtrl | ModAlt, key::Enter};
    double calib_step_percent = 2.0;
    bool beep = true;
};

// Keeps every published snapshot alive for the lifetime of the process.
//
// This is the trade the hook thread makes everywhere: a few hundred bytes per
// configuration change (a handful of small structs) in exchange for a load
// that is a single atomic read. The store must outlive the hooks; the
// application stops them before destroying it.
class RuntimeStore {
public:
    RuntimeStore();
    ~RuntimeStore();

    RuntimeStore(const RuntimeStore&) = delete;
    RuntimeStore& operator=(const RuntimeStore&) = delete;

    // Publishes a snapshot and makes it the one returned by current().
    void publish(RuntimeConfig config);

    // The live snapshot. Never null: the store publishes a default in its
    // constructor.
    const RuntimeConfig* current() const {
        return current_.load(std::memory_order_acquire);
    }

private:
    std::atomic<const RuntimeConfig*> current_{nullptr};
    std::vector<RuntimeConfig*> retained_;
    std::mutex publish_mutex_;
};

}  // namespace rm
