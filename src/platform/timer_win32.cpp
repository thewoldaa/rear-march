// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/timer.h"

#include <thread>

#include <windows.h>
#include <timeapi.h>  // timeBeginPeriod: not pulled in by WIN32_LEAN_AND_MEAN

namespace rm {
namespace platform {
namespace {

// A waitable timer's due time is expressed in 100 ns units.
constexpr long long kNanosecondsPerUnit = 100;

class ThreadTimer {
public:
    ThreadTimer() {
        timer_ = CreateWaitableTimerExW(nullptr, nullptr,
                                        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                        TIMER_ALL_ACCESS);
        if (timer_ == nullptr) {
            // Pre-1803 Windows, or a restricted token. Raising the system timer
            // resolution is a machine-wide setting, so it is only done when the
            // precise timer is not available at all.
            timeBeginPeriod(1);
            legacy_ = true;
        }
    }

    ~ThreadTimer() {
        if (timer_ != nullptr) {
            CloseHandle(timer_);
        }
        if (legacy_) {
            timeEndPeriod(1);
        }
    }

    ThreadTimer(const ThreadTimer&) = delete;
    ThreadTimer& operator=(const ThreadTimer&) = delete;

    bool precise() const { return timer_ != nullptr; }

    void wait(std::chrono::nanoseconds duration) {
        long long units = duration.count() / kNanosecondsPerUnit;
        if (units <= 0) {
            // A zero due time would disable the timer, so clamp to the smallest
            // interval the API accepts.
            units = 1;
        }

        LARGE_INTEGER due{};
        due.QuadPart = -units;  // negative: relative to now

        if (SetWaitableTimer(timer_, &due, 0, nullptr, nullptr, FALSE) == FALSE) {
            std::this_thread::sleep_for(duration);
            return;
        }
        WaitForSingleObject(timer_, INFINITE);
    }

private:
    HANDLE timer_ = nullptr;
    bool legacy_ = false;
};

ThreadTimer& thread_timer() {
    thread_local ThreadTimer timer;
    return timer;
}

}  // namespace

void sleep_for(std::chrono::nanoseconds duration) {
    if (duration <= std::chrono::nanoseconds::zero()) {
        return;
    }
    ThreadTimer& timer = thread_timer();
    if (timer.precise()) {
        timer.wait(duration);
    } else {
        std::this_thread::sleep_for(duration);
    }
}

int64_t monotonic_us() {
    static const int64_t frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value.QuadPart > 0 ? value.QuadPart : 1;
    }();

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    // Split the division so a long uptime cannot overflow the multiplication.
    const int64_t seconds = counter.QuadPart / frequency;
    const int64_t remainder = counter.QuadPart % frequency;
    return seconds * 1000000 + remainder * 1000000 / frequency;
}

}  // namespace platform
}  // namespace rm
