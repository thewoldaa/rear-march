// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/dashboard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include <imgui.h>

#include "core/keycodes.h"
#include "core/version.h"
#include "ui/capture.h"
#include "ui/theme.h"

namespace rm {
namespace ui {
namespace {

float em(float value) {
    return ImGui::GetFontSize() * value;
}

// A label in a fixed left column, so every row's control starts at the same x.
void label(const char* text) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
    ImGui::SameLine(em(10.0f));
}

// A dimmed hint on its own line under a control. Wrapped, so it never widens
// the window.
void hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

std::string angle_text(double angle) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.0f°", angle);
    return buffer;
}

std::string counts_text(int64_t counts) {
    const bool negative = counts < 0;
    std::string digits = std::to_string(negative ? -counts : counts);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) {
        digits.insert(static_cast<size_t>(i), ",");
    }
    return negative ? "-" + digits : digits;
}

void draw_header(DashboardContext& context) {
    AppConfig& config = *context.config;
    const I18n& tr = *context.i18n;

    ImGui::TextUnformatted(tr.tr("app.name").data());
    ImGui::SameLine();
    ImGui::TextDisabled("| %s", version::kVersion);

    const char* enabled_label = tr.tr("common.enabled").data();
    const float switch_width = ImGui::GetFrameHeight() +
                               ImGui::CalcTextSize(enabled_label).x +
                               ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::SameLine(ImGui::GetWindowWidth() - switch_width -
                    ImGui::GetStyle().WindowPadding.x);
    bool enabled = config.enabled;
    if (ImGui::Checkbox(enabled_label, &enabled)) {
        config.enabled = enabled;
        *context.config_dirty = true;
    }

    ImGui::Separator();
}

void draw_turn_section(DashboardContext& context) {
    AppConfig& config = *context.config;
    const I18n& tr = *context.i18n;

    ImGui::SeparatorText(tr.tr("section.turn").data());

    label(tr.tr("turn.trigger").data());
    const CaptureResult capture = trigger_capture("trigger", config.turn.trigger, tr);
    if (capture.state == CaptureResult::State::Accepted) {
        config.turn.trigger = capture.trigger;
        *context.config_dirty = true;
    }
    if (context.capture_active) {
        *context.capture_active =
            *context.capture_active ||
            capture.state == CaptureResult::State::Capturing;
    }
    hint(tr.tr("turn.trigger_hint").data());

    label(tr.tr("turn.angle").data());
    float angle = static_cast<float>(config.turn.angle_deg);
    ImGui::SetNextItemWidth(em(7.0f));
    if (ImGui::SliderFloat("##angle", &angle, static_cast<float>(kMinAngleDeg),
                           static_cast<float>(kMaxAngleDeg), "%.0f°")) {
        config.turn.angle_deg = std::clamp(static_cast<double>(angle), kMinAngleDeg,
                                           kMaxAngleDeg);
        *context.config_dirty = true;
    }
    for (const double preset : {90.0, 180.0, 270.0}) {
        ImGui::SameLine();
        const std::string caption = angle_text(preset);
        if (ImGui::SmallButton(caption.c_str())) {
            config.turn.angle_deg = preset;
            *context.config_dirty = true;
        }
    }
    hint(tr.tr("turn.angle_hint").data());

    label(tr.tr("turn.direction").data());
    const std::pair<const char*, Direction> directions[] = {
        {tr.tr("turn.direction_left").data(), Direction::Left},
        {tr.tr("turn.direction_right").data(), Direction::Right},
        {tr.tr("turn.direction_random").data(), Direction::Random},
    };
    for (size_t i = 0; i < std::size(directions); ++i) {
        if (i > 0) {
            ImGui::SameLine();
        }
        if (ImGui::RadioButton(directions[i].first,
                               config.turn.direction == directions[i].second)) {
            config.turn.direction = directions[i].second;
            *context.config_dirty = true;
        }
    }
}

void draw_sensitivity_section(DashboardContext& context) {
    AppConfig& config = *context.config;
    const I18n& tr = *context.i18n;

    ImGui::SeparatorText(tr.tr("section.sensitivity").data());

    label(tr.tr("sensitivity.dpi").data());
    int dpi = config.turn.dpi;
    ImGui::SetNextItemWidth(em(9.0f));
    if (ImGui::SliderInt("##dpi", &dpi, kSliderMinDpi, kSliderMaxDpi, "%d",
                         ImGuiSliderFlags_AlwaysClamp)) {
        config.turn.dpi = std::clamp(dpi, kMinDpi, kMaxDpi);
        *context.config_dirty = true;
    }
    ImGui::SameLine();
    // Wider than the number alone: InputInt reserves room for its two step
    // buttons inside this width, and a clipped last digit looks like a broken
    // value.
    ImGui::SetNextItemWidth(em(8.0f));
    if (ImGui::InputInt("##dpi_custom", &dpi, 0, 0)) {
        config.turn.dpi = std::clamp(dpi, kMinDpi, kMaxDpi);
        *context.config_dirty = true;
    }
    hint(tr.tr("sensitivity.dpi_hint").data());

    label(tr.tr("sensitivity.counts_per_360").data());
    int counts = static_cast<int>(std::llround(config.turn.counts_per_360));
    ImGui::SetNextItemWidth(em(9.0f));
    if (ImGui::InputInt("##counts", &counts, 100, 1000)) {
        config.turn.counts_per_360 = std::clamp(static_cast<double>(counts),
                                                kMinCountsPer360, kMaxCountsPer360);
        *context.config_dirty = true;
    }
    hint(tr.tr("sensitivity.counts_hint").data());

    // The number the user actually needs: what one press sends right now.
    const int64_t effective = turn_counts(config.turn);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", tr.tr("sensitivity.effective").data());
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kAccent), "%s counts",
                       counts_text(effective).c_str());

    ImGui::Spacing();
    if (context.state->calibrating) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::ColorConvertU32ToFloat4(colors::kAccent));
        if (ImGui::Button(tr.tr("sensitivity.stop").data(), ImVec2(em(11.0f), 0.0f))) {
            if (context.set_calibrating) {
                context.set_calibrating(false);
            } else {
                context.state->calibrating = false;
            }
            *context.config_dirty = true;
        }
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::TextUnformatted(tr.tr("sensitivity.calibrating").data());
        hint(tr.tr("sensitivity.calibrate_keys").data());

        // The number being trimmed, large enough to read from a game running
        // behind the dashboard. ImGui's window font scale is restored before
        // the next widget so the surrounding controls keep their size.
        ImGui::Spacing();
        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.45f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kAccent),
                           "%.0f", config.turn.counts_per_360);
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", tr.tr("sensitivity.counts_per_360").data());

        label(tr.tr("sensitivity.trim_up").data());
        const CaptureResult up_result =
            chord_capture("calib_up", config.calibration.up, tr);
        if (up_result.state == CaptureResult::State::Accepted) {
            config.calibration.up = up_result.chord;
            *context.config_dirty = true;
        }
        if (context.capture_active) {
            *context.capture_active =
                *context.capture_active ||
                up_result.state == CaptureResult::State::Capturing;
        }

        label(tr.tr("sensitivity.trim_down").data());
        const CaptureResult down_result =
            chord_capture("calib_down", config.calibration.down, tr);
        if (down_result.state == CaptureResult::State::Accepted) {
            config.calibration.down = down_result.chord;
            *context.config_dirty = true;
        }
        if (context.capture_active) {
            *context.capture_active =
                *context.capture_active ||
                down_result.state == CaptureResult::State::Capturing;
        }

        label(tr.tr("sensitivity.trim_save").data());
        const CaptureResult save_result =
            chord_capture("calib_save", config.calibration.save, tr);
        if (save_result.state == CaptureResult::State::Accepted) {
            config.calibration.save = save_result.chord;
            *context.config_dirty = true;
        }
        if (context.capture_active) {
            *context.capture_active =
                *context.capture_active ||
                save_result.state == CaptureResult::State::Capturing;
        }

        label(tr.tr("sensitivity.trim_step").data());
        ImGui::SetNextItemWidth(em(9.0f));
        int step = static_cast<int>(std::llround(config.calibration.step_percent));
        if (ImGui::SliderInt("##step", &step, 1, 20, "%d%%",
                             ImGuiSliderFlags_AlwaysClamp)) {
            config.calibration.step_percent = std::clamp<double>(step, 1.0, 20.0);
            *context.config_dirty = true;
        }
    } else if (ImGui::Button(tr.tr("sensitivity.calibrate").data(),
                             ImVec2(em(11.0f), 0.0f))) {
        if (context.set_calibrating) {
            context.set_calibrating(true);
        } else {
            context.state->calibrating = true;
        }
        *context.config_dirty = true;
    }
}

void draw_behavior_section(DashboardContext& context) {
    AppConfig& config = *context.config;
    I18n& tr = *context.i18n;

    ImGui::SeparatorText(tr.tr("section.behavior").data());

    label(tr.tr("behavior.ramp").data());
    int ramp = config.turn.ramp_ms;
    ImGui::SetNextItemWidth(em(9.0f));
    if (ImGui::SliderInt("##ramp", &ramp, kMinRampMs, kMaxRampMs, "%d ms",
                         ImGuiSliderFlags_AlwaysClamp)) {
        config.turn.ramp_ms = std::clamp(ramp, kMinRampMs, kMaxRampMs);
        *context.config_dirty = true;
    }
    hint(tr.tr("behavior.ramp_hint").data());

    label("");
    bool beep = config.feedback.beep;
    if (ImGui::Checkbox(tr.tr("behavior.beep").data(), &beep)) {
        config.feedback.beep = beep;
        *context.config_dirty = true;
    }

    label("");
    bool startup = config.general.start_with_windows;
    if (ImGui::Checkbox(tr.tr("behavior.start_with_windows").data(), &startup)) {
        config.general.start_with_windows = startup;
        // The registry entry is the real setting; writing only the config file
        // would make the checkbox look applied until the next launch, when the
        // startup probe overwrites it.
        if (context.set_start_with_windows) {
            context.set_start_with_windows(startup);
        }
        *context.config_dirty = true;
    }
    ImGui::SameLine();
    bool minimized = config.general.start_minimized;
    if (ImGui::Checkbox(tr.tr("behavior.start_minimized").data(), &minimized)) {
        config.general.start_minimized = minimized;
        *context.config_dirty = true;
    }

    label(tr.tr("behavior.language").data());
    const std::pair<const char*, const char*> languages[] = {
        {"English", "en"},
        {"Bahasa Indonesia", "id"},
    };
    ImGui::SetNextItemWidth(em(12.0f));
    const char* current_language =
        config.general.locale == "id" ? languages[1].first : languages[0].first;
    if (ImGui::BeginCombo("##language", current_language)) {
        for (const auto& [name, code] : languages) {
            const bool selected = config.general.locale == code;
            if (ImGui::Selectable(name, selected) && !selected) {
                config.general.locale = code;
                *context.request_locale_reload = true;
                *context.config_dirty = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void draw_status_drawer(DashboardContext& context) {
    const I18n& tr = *context.i18n;
    const DashboardState& state = *context.state;

    ImGui::Separator();

    // The drawer is its own child so long messages are clipped instead of
    // growing the window, and it is drawn after the scrolling body so it can
    // never be pushed below the visible area.
    ImGui::BeginChild("##status", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_None);
    if (!state.hook_running) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kDanger), "%s",
                           tr.tr("status.hook_failed").data());
    } else if (state.locked) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kWarning), "%s",
                           tr.tr("status.locked").data());
    } else if (!context.config->enabled) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kMuted), "%s",
                           tr.tr("status.disabled").data());
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kAccent), "%s",
                           tr.tr("status.enabled").data());
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");

    ImGui::SameLine();
    if (state.has_turned) {
        ImGui::Text("%s: %s (%s)", tr.tr("status.last_turn").data(),
                    counts_text(state.last_turn_counts).c_str(),
                    state.last_turn_sign < 0 ? tr.tr("turn.direction_left").data()
                                             : tr.tr("turn.direction_right").data());
    } else {
        ImGui::TextDisabled("%s", tr.tr("status.no_turns").data());
    }

    if (!state.message.empty()) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kWarning), "%s",
                           state.message.c_str());
    }

    // Configuration problems that would make the controls collide or silently
    // do nothing. The first one is enough for the footer; the full list lives
    // in AppConfig::validate() for tests and future diagnostics.
    const std::vector<std::string> problems = context.config->validate();
    if (!problems.empty()) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(colors::kWarning), "%s",
                           problems.front().c_str());
    }
    ImGui::EndChild();
}

}  // namespace

void draw_dashboard(DashboardContext& context) {
    // Rebuilt every frame from the recorders that are actually open; see the
    // capture results below. Done before drawing so a closed recorder cannot
    // leave the trigger suppressed.
    if (context.capture_active) {
        *context.capture_active = false;
    }

    draw_header(context);

    // Fixed header and footer, scrolling body: at 125% DPI on a small screen
    // the body would otherwise push the status line out of view. The status
    // child asks for whatever is left after the body, so the two always add up
    // to the window height.
    const float footer_height =
        ImGui::GetTextLineHeightWithSpacing() * 2.0f + ImGui::GetStyle().ItemSpacing.y;
    const float body_height = ImGui::GetContentRegionAvail().y - footer_height;

    ImGui::BeginChild("##body", ImVec2(0.0f, body_height), ImGuiChildFlags_None,
                      ImGuiWindowFlags_None);

    draw_turn_section(context);
    ImGui::Spacing();
    draw_sensitivity_section(context);
    ImGui::Spacing();
    draw_behavior_section(context);

    ImGui::EndChild();
    draw_status_drawer(context);
}

}  // namespace ui
}  // namespace rm
