// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <vector>

namespace rm {
namespace input {

// Attached to every input event RearMarch injects through SendInput.
//
// Without a tag the mouse hook would see the turn it just produced and could
// mistake it for a trigger press when the trigger is a mouse button. The tag is
// a precise filter; LLMHF_INJECTED is too broad (it would also hide a second
// tool's output, which this application does not need to hide).
//
// It fits in 32 bits on purpose. Windows hands low-level hook procedures the
// value truncated to a DWORD, so a 64-bit tag silently never matches. "KMO2" is
// deliberately different from KeyMouse's tag so the two applications never
// mistake each other's injections for their own.
inline constexpr uint32_t kInjectionTag = 0x4B4D4F32u;

// Sends a batch of relative horizontal deltas as mouse events.
//
// All deltas go out in one SendInput call, which is both faster and more
// compatible than one call per event: some games read only the first event of
// a burst or clamp each one, and a single call keeps the turn inside one
// sampling window of the game loop. `dy` is applied to the first event only
// and is normally zero.
//
// Returns the number of events Windows accepted (0 on failure).
uint32_t send_move_batch(const std::vector<int32_t>& dxs, int32_t dy = 0);

// Injects one relative mouse movement. Used by the ramp, which spreads the
// events over time instead of sending them together.
void send_move(int32_t dx, int32_t dy = 0);

// Whether a physical mouse side button is currently down (1 = X1, 2 = X2).
bool physical_button_down(int xbutton);

}  // namespace input
}  // namespace rm
