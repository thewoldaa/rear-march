// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <windows.h>

#include <functional>
#include <string>

namespace rm {

class I18n;

namespace ui {

enum class TrayStatus { Active, Disabled, Calibrating };

// Owns the notification area icon and its context menu.
//
// The tray icon lives on the dashboard's window, which is hidden rather than
// destroyed when the user closes it, so the icon stays clickable.
class TrayIcon {
public:
    struct Callbacks {
        std::function<void()> on_show;
        std::function<void()> on_toggle;
        std::function<void()> on_exit;
        std::function<void(bool)> on_startup_changed;
        std::function<bool()> get_startup;
        std::function<bool()> get_enabled;
    };

    TrayIcon() = default;
    ~TrayIcon();

    bool create(HWND window, HICON icon);
    void destroy();

    // Menu labels come from the loaded locale. Not owned; must outlive the
    // icon.
    void set_i18n(const I18n* i18n) { i18n_ = i18n; }

    void set_status(TrayStatus status);
    void set_tooltip(const std::wstring& text);

    void set_callbacks(Callbacks callbacks) { callbacks_ = std::move(callbacks); }

    // Handles WM_TRAYICON messages forwarded by the window procedure.
    void handle_message(WPARAM wparam, LPARAM lparam);

    // The message id the window should forward.
    static UINT message_id();

private:
    void show_menu();
    std::wstring text(const char* key) const;

    HWND window_ = nullptr;
    HICON icon_ = nullptr;
    TrayStatus status_ = TrayStatus::Active;
    std::wstring tooltip_ = L"RearMarch";
    Callbacks callbacks_;
    const I18n* i18n_ = nullptr;
};

}  // namespace ui
}  // namespace rm
