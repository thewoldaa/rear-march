// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include "core/config.h"

using namespace rm;

TEST_CASE("the default configuration is usable") {
    const AppConfig config = make_default_config();
    CHECK(config.schema == kConfigSchemaVersion);
    CHECK(config.turn.trigger.kind == TriggerKind::MouseX1);
    CHECK(config.turn.angle_deg == 180.0);
    CHECK(config.turn.dpi == 1000);
    CHECK(config.turn.direction == Direction::Right);
    CHECK(config.toggle.valid());
    CHECK(config.validate().empty());
}

TEST_CASE("config round-trips through JSON") {
    AppConfig config = make_default_config();
    config.turn.trigger = Trigger{TriggerKind::Keyboard, key::F7};
    config.turn.angle_deg = 270.0;
    config.turn.dpi = 8000;
    config.turn.direction = Direction::Random;
    config.turn.counts_per_360 = 7200.0;
    config.turn.ramp_ms = 25;
    config.general.locale = "id";
    config.feedback.beep = false;
    config.toggle = Chord{ModCtrl | ModShift, key::KeyQ};

    const std::string text = to_json(config);
    std::string error;
    const auto parsed = from_json(text, error);
    REQUIRE(parsed.has_value());
    CHECK(error.empty());

    CHECK(parsed->turn.trigger == config.turn.trigger);
    CHECK(parsed->turn.angle_deg == doctest::Approx(config.turn.angle_deg));
    CHECK(parsed->turn.dpi == config.turn.dpi);
    CHECK(parsed->turn.direction == config.turn.direction);
    CHECK(parsed->turn.counts_per_360 ==
          doctest::Approx(config.turn.counts_per_360));
    CHECK(parsed->turn.ramp_ms == config.turn.ramp_ms);
    CHECK(parsed->general.locale == "id");
    CHECK_FALSE(parsed->feedback.beep);
    CHECK(parsed->toggle == config.toggle);
}

TEST_CASE("an empty object loads as defaults") {
    std::string error;
    const auto config = from_json("{}", error);
    REQUIRE(config.has_value());
    CHECK(config->turn.angle_deg == 180.0);
    CHECK(config->turn.trigger.kind == TriggerKind::MouseX1);
}

TEST_CASE("malformed JSON is rejected with an error") {
    std::string error;
    CHECK_FALSE(from_json("{ not json", error).has_value());
    CHECK_FALSE(error.empty());

    CHECK_FALSE(from_json("[]", error).has_value());
    CHECK_FALSE(error.empty());
}

TEST_CASE("a newer schema is rejected rather than half-read") {
    std::string error;
    CHECK_FALSE(from_json(R"({"schema": 99})", error).has_value());
    CHECK(error.find("newer") != std::string::npos);
}

TEST_CASE("unknown keys are ignored") {
    std::string error;
    const auto config = from_json(
        R"({"schema":1,"turn":{"angle_deg":90},"future_field":{"x":1}})", error);
    REQUIRE(config.has_value());
    CHECK(config->turn.angle_deg == 90.0);
}

TEST_CASE("out-of-range numbers are clamped on load") {
    std::string error;
    const auto config = from_json(
        R"({"turn":{"angle_deg":9000,"dpi":1,"counts_per_360":-5,"ramp_ms":9999}})",
        error);
    REQUIRE(config.has_value());
    CHECK(config->turn.angle_deg == kMaxAngleDeg);
    CHECK(config->turn.dpi == kMinDpi);
    CHECK(config->turn.counts_per_360 == kMinCountsPer360);
    CHECK(config->turn.ramp_ms == kMaxRampMs);
}

TEST_CASE("an invalid trigger falls back to the default") {
    std::string error;
    const auto config = from_json(R"({"turn":{"trigger":"Banana"}})", error);
    REQUIRE(config.has_value());
    CHECK(config->turn.trigger.kind == TriggerKind::MouseX1);
}

TEST_CASE("an invalid chord falls back to the default") {
    std::string error;
    const auto config = from_json(
        R"({"toggle":{"mods":["hyper"],"key":"KeyM"}})", error);
    REQUIRE(config.has_value());
    CHECK(config->toggle == Chord{ModCtrl | ModAlt, key::KeyM});
}

TEST_CASE("an unknown locale is normalised to English") {
    std::string error;
    const auto config = from_json(R"({"general":{"locale":"fr"}})", error);
    REQUIRE(config.has_value());
    CHECK(config->general.locale == "en");
}

TEST_CASE("the serialized file has the documented top-level keys") {
    const std::string text = to_json(make_default_config());
    CHECK(text.find("\"schema\"") != std::string::npos);
    CHECK(text.find("\"turn\"") != std::string::npos);
    CHECK(text.find("\"toggle\"") != std::string::npos);
    CHECK(text.find("\"calibration\"") != std::string::npos);
    CHECK(text.find("\"feedback\"") != std::string::npos);
    CHECK(text.find("\"general\"") != std::string::npos);
    CHECK(text.find("\"trigger\": \"X1\"") != std::string::npos);
}

TEST_CASE("validate reports duplicate control chords") {
    AppConfig config = make_default_config();
    config.calibration.up = config.toggle;
    CHECK_FALSE(config.validate().empty());
}

TEST_CASE("validate reports a plain-key trigger that clashes with a chord") {
    AppConfig config = make_default_config();
    config.turn.trigger = Trigger{TriggerKind::Keyboard, key::KeyM};
    config.toggle = Chord{ModNone, key::KeyM};
    CHECK_FALSE(config.validate().empty());
}

TEST_CASE("sanitize repairs an invalid direction") {
    AppConfig config = make_default_config();
    config.turn.direction = static_cast<Direction>(99);
    sanitize(config);
    CHECK(config.turn.direction == Direction::Right);
}

TEST_CASE("sanitize never leaves an invalid trigger") {
    AppConfig config = make_default_config();
    config.turn.trigger = Trigger{TriggerKind::Keyboard, key::Invalid};
    sanitize(config);
    CHECK(config.turn.trigger.valid());
}
