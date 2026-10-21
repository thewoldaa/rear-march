// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/session.h"

#include <windows.h>
#include <wtsapi32.h>

namespace rm {
namespace session {
namespace {

SessionCallback g_callback = nullptr;

}  // namespace

bool subscribe(void* window_handle, SessionCallback callback) {
    g_callback = callback;
    return WTSRegisterSessionNotification(static_cast<HWND>(window_handle),
                                          NOTIFY_FOR_THIS_SESSION) != FALSE;
}

void unsubscribe(void* window_handle) {
    WTSUnRegisterSessionNotification(static_cast<HWND>(window_handle));
    g_callback = nullptr;
}

void handle_session_change(unsigned long event) {
    if (!g_callback) {
        return;
    }

    switch (event) {
        case WTS_SESSION_LOCK:
        case WTS_SESSION_LOGOFF:
            g_callback(SessionEvent::Locked);
            break;
        case WTS_SESSION_UNLOCK:
            g_callback(SessionEvent::Unlocked);
            break;
        default:
            g_callback(SessionEvent::Other);
            break;
    }
}

}  // namespace session
}  // namespace rm
