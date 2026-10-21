// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

namespace rm {

class RuntimeStore;
class ActionMailbox;

// The turn worker runs on its own thread. It waits for a request, reads the
// runtime snapshot, plans the turn, and injects it. Injection never happens
// inside the low-level hook callback: Windows removes a hook whose callback
// takes too long, and blocking there would stall the user's keyboard.
class TurnWorker {
public:
    TurnWorker(RuntimeStore& store, ActionMailbox& actions);
    ~TurnWorker();

    TurnWorker(const TurnWorker&) = delete;
    TurnWorker& operator=(const TurnWorker&) = delete;

    bool start();
    void stop();

    // Called from the hook thread. Never blocks: the sign is stored in an
    // atomic slot and the worker is woken with SetEvent. A second press that
    // arrives before the worker wakes up replaces the first, which is correct
    // for a single-shot trigger.
    void request_turn(int sign);

    // Plays the calibration feedback tone. Called from the hook thread for the
    // same reason: the game has focus, and the tone must not come from a UI
    // callback that only runs at frame rate.
    void request_feedback(bool up);

    // Diagnostic counters, read by the UI thread.
    uint64_t turns_done() const { return turns_done_.load(); }
    int64_t last_counts() const { return last_counts_.load(); }
    int last_sign() const { return last_sign_.load(); }

private:
    void loop();
    void perform_turn(int sign);
    void perform_feedback(bool up);

    RuntimeStore& store_;
    ActionMailbox& actions_;
    std::thread thread_;
    std::atomic<bool> stop_requested_{false};
    void* wake_event_ = nullptr;  // HANDLE, kept as void* so the header stays clean

    std::atomic<bool> has_turn_{false};
    std::atomic<int> turn_sign_{1};

    std::atomic<bool> has_feedback_{false};
    std::atomic<bool> feedback_up_{false};

    std::atomic<uint64_t> turns_done_{0};
    std::atomic<int64_t> last_counts_{0};
    std::atomic<int> last_sign_{1};
};

}  // namespace rm
