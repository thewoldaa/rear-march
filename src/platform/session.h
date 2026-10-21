// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace rm {

// Session change notifications.
//
// Locking the workstation switches to another desktop, where injected input
// must never reach (the lock screen is a secure desktop and the game is no
// longer in the foreground). The application marks the runtime snapshot
// locked so the trigger is ignored, and stops any pending ramp.
namespace session {

enum class SessionEvent {
    Locked,
    Unlocked,
    Other,
};

// Called on the UI thread, from the message loop.
using SessionCallback = void (*)(SessionEvent);

bool subscribe(void* window_handle, SessionCallback callback);

void unsubscribe(void* window_handle);

// Dispatches a WM_WTSSESSION_CHANGE payload. Called by the window procedure
// with the message's wParam.
void handle_session_change(unsigned long event);

}  // namespace session
}  // namespace rm
