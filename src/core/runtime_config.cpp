// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/runtime_config.h"

namespace rm {

RuntimeStore::RuntimeStore() {
    publish(RuntimeConfig{});
}

RuntimeStore::~RuntimeStore() {
    for (RuntimeConfig* config : retained_) {
        delete config;
    }
}

void RuntimeStore::publish(RuntimeConfig config) {
    auto* copy = new RuntimeConfig(std::move(config));

    // Only one thread publishes (the UI thread), but the lock also covers the
    // retainer vector, and the cost is irrelevant at configuration-change
    // frequency.
    std::lock_guard<std::mutex> lock(publish_mutex_);
    retained_.push_back(copy);
    current_.store(copy, std::memory_order_release);
}

}  // namespace rm
