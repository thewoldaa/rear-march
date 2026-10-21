// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include <cmath>
#include <numeric>
#include <random>

#include "core/turn.h"

using namespace rm;

namespace {

TurnSettings base_settings() {
    TurnSettings settings;
    settings.angle_deg = 180.0;
    settings.dpi = 1000;
    settings.counts_per_360 = 6000.0;
    settings.direction = Direction::Right;
    return settings;
}

int64_t total(const std::vector<int32_t>& values) {
    return std::accumulate(values.begin(), values.end(), int64_t{0});
}

}  // namespace

TEST_CASE("a 180 degree turn at 1000 DPI sends half of counts_per_360") {
    const TurnSettings settings = base_settings();
    CHECK(turn_counts(settings) == 3000);
    CHECK(effective_counts_per_360(settings) == doctest::Approx(6000.0));
}

TEST_CASE("DPI multiplies the counts") {
    TurnSettings settings = base_settings();
    settings.dpi = 8000;
    CHECK(turn_counts(settings) == 24000);

    settings.dpi = 16000;
    CHECK(turn_counts(settings) == 48000);
}

TEST_CASE("the angle scales linearly and rounds to the nearest count") {
    TurnSettings settings = base_settings();
    settings.angle_deg = 90.0;
    CHECK(turn_counts(settings) == 1500);

    settings.angle_deg = 270.0;
    CHECK(turn_counts(settings) == 4500);

    settings.angle_deg = 5.0;
    CHECK(turn_counts(settings) == 83);  // 83.33 rounds down

    settings.angle_deg = 360.0;
    CHECK(turn_counts(settings) == 6000);
}

TEST_CASE("turn_counts is clamped and never zero") {
    TurnSettings settings = base_settings();
    settings.counts_per_360 = kMinCountsPer360;
    settings.angle_deg = kMinAngleDeg;
    CHECK(turn_counts(settings) >= 1);

    // Out-of-range values are clamped rather than trusted.
    settings.counts_per_360 = 6000.0;
    settings.angle_deg = 10000.0;
    CHECK(turn_counts(settings) == 6000);

    settings.angle_deg = -50.0;
    CHECK(turn_counts(settings) >= 1);
}

TEST_CASE("turn_dx applies the sign") {
    const TurnSettings settings = base_settings();
    CHECK(turn_dx(settings, 1) == 3000);
    CHECK(turn_dx(settings, -1) == -3000);
}

TEST_CASE("resolve_direction passes fixed directions through") {
    std::mt19937 rng(1234);
    for (int i = 0; i < 100; ++i) {
        CHECK(resolve_direction(Direction::Left, rng) == Direction::Left);
        CHECK(resolve_direction(Direction::Right, rng) == Direction::Right);
    }
}

TEST_CASE("resolve_direction for random produces both directions") {
    std::mt19937 rng(1234);
    int left = 0;
    int right = 0;
    for (int i = 0; i < 1000; ++i) {
        if (resolve_direction(Direction::Random, rng) == Direction::Left) {
            ++left;
        } else {
            ++right;
        }
    }
    // A fair coin over 1000 flips lands far from an all-one-side result; this
    // is a smoke test that both values occur, not a statistical one.
    CHECK(left > 300);
    CHECK(right > 300);
}

TEST_CASE("resolve_direction is deterministic for a given seed") {
    std::mt19937 a(42);
    std::mt19937 b(42);
    for (int i = 0; i < 50; ++i) {
        CHECK(resolve_direction(Direction::Random, a) ==
              resolve_direction(Direction::Random, b));
    }
}

TEST_CASE("split_counts sums exactly to the total") {
    CHECK(total(split_counts(3000)) == 3000);
    CHECK(total(split_counts(-3000)) == -3000);
    CHECK(total(split_counts(1)) == 1);
    CHECK(total(split_counts(255)) == 255);
    CHECK(total(split_counts(256)) == 256);
    CHECK(total(split_counts(257)) == 257);
    CHECK(total(split_counts(48000)) == 48000);
    CHECK(total(split_counts(-48000)) == -48000);
}

TEST_CASE("split_counts respects the per-event budget when possible") {
    const auto events = split_counts(3000);
    // 3000 / 256 = 11.7 -> 12 events, none above 256.
    CHECK(events.size() == 12);
    for (const int32_t dx : events) {
        CHECK(dx > 0);
        CHECK(dx <= kCountsPerEvent);
    }
}

TEST_CASE("split_counts caps the event count and keeps the total") {
    const auto events = split_counts(1000000);
    CHECK(events.size() == static_cast<size_t>(kMaxEventsPerTurn));
    CHECK(total(events) == 1000000);
}

TEST_CASE("split_counts spreads the remainder over the first events") {
    const auto events = split_counts(10, 3);
    REQUIRE(events.size() == 4);
    // 10 = 3 + 3 + 2 + 2: the remainder is distributed, not dropped.
    CHECK(total(events) == 10);
    for (const int32_t dx : events) {
        CHECK(dx >= 2);
        CHECK(dx <= 3);
    }
}

TEST_CASE("split_counts handles zero and negatives") {
    CHECK(split_counts(0).empty());

    const auto negative = split_counts(-10, 3);
    CHECK(total(negative) == -10);
    for (const int32_t dx : negative) {
        CHECK(dx <= -2);
    }
}

TEST_CASE("plan_turn carries the ramp and the sign") {
    TurnSettings settings = base_settings();
    settings.ramp_ms = 40;
    const TurnPlan plan = plan_turn(settings, -1);
    CHECK(plan.ramp_ms == 40);
    CHECK(total(plan.dxs) == -3000);
}

TEST_CASE("plan_turn clamps an out-of-range ramp") {
    TurnSettings settings = base_settings();
    settings.ramp_ms = 5000;
    CHECK(plan_turn(settings, 1).ramp_ms == kMaxRampMs);

    settings.ramp_ms = -10;
    CHECK(plan_turn(settings, 1).ramp_ms == kMinRampMs);
}

TEST_CASE("calibration trims by percentage and clamps") {
    CHECK(trimmed_counts(6000.0, 2.0, true) == doctest::Approx(6120.0));
    CHECK(trimmed_counts(6000.0, 2.0, false) == doctest::Approx(5880.0));

    CHECK(trimmed_counts(kMaxCountsPer360, 20.0, true) ==
          doctest::Approx(kMaxCountsPer360));
    CHECK(trimmed_counts(kMinCountsPer360, 20.0, false) ==
          doctest::Approx(kMinCountsPer360));
}

TEST_CASE("trigger strings round-trip") {
    CHECK(trigger_to_string(Trigger{TriggerKind::MouseX1, key::Invalid}) == "X1");
    CHECK(trigger_to_string(Trigger{TriggerKind::MouseX2, key::Invalid}) == "X2");
    CHECK(trigger_to_string(Trigger{TriggerKind::Keyboard, key::F7}) == "F7");

    const auto x1 = trigger_from_string("X1");
    REQUIRE(x1.has_value());
    CHECK(x1->kind == TriggerKind::MouseX1);

    const auto x2 = trigger_from_string("x2");
    REQUIRE(x2.has_value());
    CHECK(x2->kind == TriggerKind::MouseX2);

    const auto key = trigger_from_string("KeyQ");
    REQUIRE(key.has_value());
    CHECK(key->kind == TriggerKind::Keyboard);
    CHECK(key->key == key::KeyQ);
}

TEST_CASE("trigger parsing rejects modifiers and nonsense") {
    CHECK_FALSE(trigger_from_string("ControlLeft").has_value());
    CHECK_FALSE(trigger_from_string("ShiftRight").has_value());
    CHECK_FALSE(trigger_from_string("Banana").has_value());
    CHECK_FALSE(trigger_from_string("").has_value());
}

TEST_CASE("direction names round-trip") {
    for (const Direction direction :
         {Direction::Left, Direction::Right, Direction::Random}) {
        Direction parsed = Direction::Right;
        CHECK(direction_from_name(direction_name(direction), parsed));
        CHECK(parsed == direction);
    }
    Direction ignored = Direction::Right;
    CHECK_FALSE(direction_from_name("up", ignored));
}
