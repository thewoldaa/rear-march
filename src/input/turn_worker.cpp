// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/turn_worker.h"

#include <algorithm>
#include <chrono>

#include <windows.h>

#include "core/action_mailbox.h"
#include "core/runtime_config.h"
#include "core/turn.h"
#include "input/injection.h"
#include "platform/timer.h"

namespace rm {

TurnWorker::TurnWorker(RuntimeStore& store, ActionMailbox& actions)
    : store_(store), actions_(actions) {}

TurnWorker::~TurnWorker() {
    stop();
}

bool TurnWorker::start() {
    if (thread_.joinable()) {
        return true;
    }

    wake_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!wake_event_) {
        return false;
    }

    stop_requested_.store(false);
    thread_ = std::thread([this] { loop(); });
    return true;
}

void TurnWorker::stop() {
    if (!thread_.joinable()) {
        return;
    }

    stop_requested_.store(true);
    if (wake_event_) {
        SetEvent(static_cast<HANDLE>(wake_event_));
    }
    thread_.join();

    if (wake_event_) {
        CloseHandle(static_cast<HANDLE>(wake_event_));
        wake_event_ = nullptr;
    }
}

void TurnWorker::request_turn(int sign) {
    turn_sign_.store(sign >= 0 ? 1 : -1, std::memory_order_relaxed);
    has_turn_.store(true, std::memory_order_release);
    if (wake_event_) {
        SetEvent(static_cast<HANDLE>(wake_event_));
    }
}

void TurnWorker::request_feedback(bool up) {
    feedback_up_.store(up, std::memory_order_relaxed);
    has_feedback_.store(true, std::memory_order_release);
    if (wake_event_) {
        SetEvent(static_cast<HANDLE>(wake_event_));
    }
}

void TurnWorker::loop() {
    while (!stop_requested_.load()) {
        WaitForSingleObject(static_cast<HANDLE>(wake_event_), 250);

        if (stop_requested_.load()) {
            break;
        }

        if (has_feedback_.exchange(false, std::memory_order_acq_rel)) {
            perform_feedback(feedback_up_.load(std::memory_order_relaxed));
        }

        if (has_turn_.exchange(false, std::memory_order_acq_rel)) {
            perform_turn(turn_sign_.load(std::memory_order_relaxed));
        }
    }
}

void TurnWorker::perform_turn(int sign) {
    const RuntimeConfig* config = store_.current();
    if (!config || !config->enabled || config->locked) {
        return;
    }

    const TurnPlan plan = plan_turn(config->turn, sign);
    if (plan.dxs.empty()) {
        return;
    }

    int64_t total = 0;
    for (const int32_t dx : plan.dxs) {
        total += dx;
    }

    if (plan.ramp_ms <= 0 || plan.dxs.size() == 1) {
        input::send_move_batch(plan.dxs);
    } else {
        // Spread the events evenly over the ramp. The platform timer is the
        // high-resolution waitable timer; the last event goes out immediately
        // rather than after a pointless final sleep.
        const auto total_time =
            std::chrono::microseconds(static_cast<long long>(plan.ramp_ms) * 1000);
        const auto step =
            total_time / static_cast<long long>(plan.dxs.size());
        for (size_t i = 0; i < plan.dxs.size(); ++i) {
            if (stop_requested_.load()) {
                return;
            }
            input::send_move(plan.dxs[i]);
            if (i + 1 < plan.dxs.size()) {
                platform::sleep_for(step);
            }
        }
    }

    last_counts_.store(total);
    last_sign_.store(sign >= 0 ? 1 : -1);
    turns_done_.fetch_add(1);
}

void TurnWorker::perform_feedback(bool up) {
    const RuntimeConfig* config = store_.current();
    if (!config || !config->beep) {
        return;
    }
    // Two tones distinguish "more counts" from "fewer counts" without looking
    // at the dashboard, which is the point of calibrating inside the game.
    Beep(up ? 1400 : 900, 40);
}

}  // namespace rm
