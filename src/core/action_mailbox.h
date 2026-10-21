// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>

namespace rm {

// Actions that are detected on the hook thread but must be applied on the UI
// thread, where the configuration and the dashboard live.
enum class AppAction : uint8_t {
    Toggle,          // Ctrl+Alt+M: enable or disable
    CalibrateUp,     // more counts per turn
    CalibrateDown,   // fewer counts per turn
    CalibrateSave,   // leave calibration mode and persist
    ShowDashboard,   // a second launch asked for the window
    Count,
};

// A lock-free hand-off between the hook thread and the UI thread.
//
// Each action is a counter rather than a queue: the hook thread only ever
// increments, and the UI thread drains with an exchange, so several presses
// that land between two frames are all applied instead of overwriting each
// other. The hook callback must not allocate or lock, which rules out a
// std::deque here.
class ActionMailbox {
public:
    void post(AppAction action) {
        pending_[static_cast<size_t>(action)].fetch_add(1, std::memory_order_relaxed);
    }

    // Returns how many times the action was posted since the last drain.
    uint32_t take(AppAction action) {
        return pending_[static_cast<size_t>(action)].exchange(
            0, std::memory_order_acq_rel);
    }

    void clear() {
        for (auto& slot : pending_) {
            slot.store(0, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<uint32_t> pending_[static_cast<size_t>(AppAction::Count)]{};
};

}  // namespace rm
