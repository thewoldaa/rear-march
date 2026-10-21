// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include <string_view>

#include "core/keycodes.h"

using namespace rm;

TEST_CASE("the key table is sorted by id") {
    // key_info() binary-searches the table; an out-of-order entry would make
    // some keys silently unreachable.
    KeyId previous = 0;
    for (const auto& info : all_keys()) {
        CHECK(info.id >= previous);
        previous = info.id;
    }
}

TEST_CASE("names are unique") {
    for (const auto& a : all_keys()) {
        for (const auto& b : all_keys()) {
            if (a.id == b.id) {
                continue;
            }
            CHECK(a.name != b.name);
        }
    }
}

TEST_CASE("key_from_name finds canonical names") {
    const auto key_a = key_from_name("KeyA");
    REQUIRE(key_a.has_value());
    CHECK(*key_a == key::KeyA);

    const auto f7 = key_from_name("F7");
    REQUIRE(f7.has_value());
    CHECK(*f7 == key::F7);

    const auto numpad0 = key_from_name("Numpad0");
    REQUIRE(numpad0.has_value());
    CHECK(*numpad0 == key::Numpad0);
}

TEST_CASE("key_from_name is case sensitive on purpose") {
    // The config writer always emits the canonical spelling; accepting "keya"
    // would let a typo load and then silently bind the wrong key if a future
    // key name differed only by case.
    CHECK_FALSE(key_from_name("keya").has_value());
}

TEST_CASE("key_name of an unknown id is Invalid") {
    CHECK(key_name(static_cast<KeyId>(9999)) == "Invalid");
}

TEST_CASE("key_label gives a short form") {
    CHECK(key_label(key::KeyA) == "A");
    CHECK(key_label(key::Numpad0) == "Num 0");
    CHECK(key_label(key::ArrowUp) == "Up");
}

TEST_CASE("modifiers are flagged") {
    CHECK(key_is_modifier(key::ControlLeft));
    CHECK(key_is_modifier(key::ShiftRight));
    CHECK_FALSE(key_is_modifier(key::KeyA));
    CHECK_FALSE(key_is_modifier(key::Invalid));
}

TEST_CASE("the id ranges the code relies on stay contiguous") {
    CHECK(key::KeyZ - key::KeyA == 25);
    CHECK(key::Key0 - key::Key1 == 9);
    CHECK(key::F12 - key::F1 == 11);
    CHECK(key::F24 - key::F13 == 11);
    CHECK(key::Numpad0 - key::Numpad1 == 9);
}
