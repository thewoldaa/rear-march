// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

#include <imgui.h>

#include "core/chord.h"
#include "core/i18n.h"
#include "core/turn.h"

namespace rm {
namespace ui {

// Result of one frame of a recorder.
struct CaptureResult {
    enum class State {
        Idle,       // nothing happened this frame
        Capturing,  // this recorder is open and waiting for input
        Accepted,   // something was recorded this frame
        Cancelled,  // the recorder was closed without recording
    };

    State state = State::Idle;
    Trigger trigger;  // set for trigger_capture
    Chord chord;      // set for chord_capture
};

// A button that turns into a recorder for one physical input: a keyboard key
// or a mouse side button. ImGui has no key identifiers for X1/X2, so the
// picker polls the physical button state in addition to ImGui's key state.
//
// The recorder's state lives in ImGui's per-window storage, keyed by `id` and
// the current ID stack. Escape cancels.
//
// While a recorder is open the caller must publish capture_active through the
// runtime snapshot, otherwise a key that is already bound would fire before
// the recorder can see it.
CaptureResult trigger_capture(const char* id, const Trigger& current, const I18n& tr);

// A button that turns into a recorder for a chord (modifiers plus one key).
// Modifiers are read from ImGui's IO state; the Fn layer does not exist here.
CaptureResult chord_capture(const char* id, const Chord& current, const I18n& tr);

// Renders a chord as a readable label, "(unset)" when invalid.
std::string chord_label(const Chord& chord, const I18n& tr);

// Renders a trigger as a readable label.
std::string trigger_text(const Trigger& trigger, const I18n& tr);

}  // namespace ui
}  // namespace rm
