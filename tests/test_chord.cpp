// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include "core/chord.h"

using namespace rm;

TEST_CASE("default toggle chord is Ctrl+Alt+M") {
    const Chord chord{ModCtrl | ModAlt, key::KeyM};
    CHECK(chord.valid());
    CHECK(format_chord(chord) == "Ctrl + Alt + KeyM");
}

TEST_CASE("parse_chord accepts the canonical spelling") {
    const auto chord = parse_chord("Ctrl+Alt+KeyM");
    REQUIRE(chord.has_value());
    CHECK(*chord == Chord{ModCtrl | ModAlt, key::KeyM});
}

TEST_CASE("parse_chord is case insensitive and tolerates spacing") {
    const auto chord = parse_chord(" ctrl + ALT + keym ");
    REQUIRE(chord.has_value());
    CHECK(*chord == Chord{ModCtrl | ModAlt, key::KeyM});
}

TEST_CASE("parse_chord accepts a bare key") {
    const auto chord = parse_chord("KeyQ");
    REQUIRE(chord.has_value());
    CHECK(*chord == Chord{ModNone, key::KeyQ});
}

TEST_CASE("parse_chord rejects duplicate modifiers") {
    CHECK_FALSE(parse_chord("Ctrl+Ctrl+KeyM").has_value());
}

TEST_CASE("parse_chord rejects two base keys") {
    CHECK_FALSE(parse_chord("KeyA+KeyB").has_value());
}

TEST_CASE("parse_chord rejects a modifier as the base key") {
    CHECK_FALSE(parse_chord("Ctrl+Shift").has_value());
}

TEST_CASE("parse_chord rejects unknown names") {
    CHECK_FALSE(parse_chord("Ctrl+Banana").has_value());
}

TEST_CASE("format_modifiers keeps the canonical order") {
    CHECK(format_modifiers(ModWin | ModShift | ModCtrl | ModAlt) ==
          "Ctrl + Alt + Shift + Win");
    CHECK(format_modifiers(ModNone).empty());
}

TEST_CASE("format_chord of an invalid chord is empty") {
    CHECK(format_chord(Chord{}).empty());
    CHECK(format_chord(Chord{ModNone, key::Invalid}).empty());
}

TEST_CASE("modifier_bit collapses left and right") {
    CHECK(modifier_bit(key::ControlLeft) == ModCtrl);
    CHECK(modifier_bit(key::ControlRight) == ModCtrl);
    CHECK(modifier_bit(key::AltLeft) == ModAlt);
    CHECK(modifier_bit(key::MetaRight) == ModWin);
    CHECK(modifier_bit(key::KeyA) == ModNone);
}

TEST_CASE("is_modifier_key agrees with modifier_bit") {
    CHECK(is_modifier_key(key::ShiftLeft));
    CHECK_FALSE(is_modifier_key(key::ShiftRight + 100));
}
