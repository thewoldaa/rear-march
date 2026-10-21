// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/tray.h"

#include <windows.h>
#include <shellapi.h>

#include "core/i18n.h"

namespace rm {
namespace ui {
namespace {

constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kMenuToggle = 1001;
constexpr UINT kMenuShow = 1002;
constexpr UINT kMenuStartup = 1003;
constexpr UINT kMenuExit = 1004;

// Status colours drawn as a small dot, since the .ico files are not shipped
// per-state.
HICON make_status_icon(COLORREF color) {
    const int size = GetSystemMetrics(SM_CXSMICON);
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, size, size);
    HGDIOBJ previous = SelectObject(memory, bitmap);

    RECT rect{0, 0, size, size};
    HBRUSH background = CreateSolidBrush(RGB(0x15, 0x18, 0x1D));
    FillRect(memory, &rect, background);
    DeleteObject(background);

    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old_brush = SelectObject(memory, brush);
    HPEN pen = CreatePen(PS_NULL, 0, color);
    HGDIOBJ old_pen = SelectObject(memory, pen);

    const int inset = size / 8;
    Ellipse(memory, inset, inset, size - inset, size - inset);

    SelectObject(memory, old_pen);
    SelectObject(memory, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);

    SelectObject(memory, previous);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO info{};
    info.hbmMask = mask;
    info.hbmColor = bitmap;

    HICON icon = CreateIconIndirect(&info);
    DeleteObject(bitmap);
    DeleteObject(mask);
    return icon;
}

COLORREF color_for(TrayStatus status) {
    switch (status) {
        case TrayStatus::Active: return RGB(0x4C, 0xC2, 0xA6);
        case TrayStatus::Disabled: return RGB(0x8A, 0x92, 0xA0);
        case TrayStatus::Calibrating: return RGB(0x5A, 0xA9, 0xFF);
    }
    return RGB(0x8A, 0x92, 0xA0);
}

// Win32 menus are UTF-16; the translation table is UTF-8.
std::wstring widen(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                         static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                        out.data(), size);
    return out;
}

}  // namespace

TrayIcon::~TrayIcon() {
    destroy();
}

UINT TrayIcon::message_id() {
    return kTrayMessage;
}

bool TrayIcon::create(HWND window, HICON icon) {
    window_ = window;
    if (icon) {
        icon_ = icon;
    } else {
        icon_ = make_status_icon(color_for(status_));
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayMessage;
    data.hIcon = icon_;
    wcsncpy_s(data.szTip, tooltip_.c_str(), _TRUNCATE);

    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        return false;
    }

    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
    return true;
}

void TrayIcon::destroy() {
    if (window_) {
        NOTIFYICONDATAW data{};
        data.cbSize = sizeof(data);
        data.hWnd = window_;
        data.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &data);
        window_ = nullptr;
    }
    if (icon_) {
        DestroyIcon(icon_);
        icon_ = nullptr;
    }
}

void TrayIcon::set_status(TrayStatus status) {
    if (status == status_) {
        return;
    }
    status_ = status;

    if (icon_) {
        DestroyIcon(icon_);
    }
    icon_ = make_status_icon(color_for(status_));

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = 1;
    data.uFlags = NIF_ICON;
    data.hIcon = icon_;
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void TrayIcon::set_tooltip(const std::wstring& text) {
    tooltip_ = text;
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = window_;
    data.uID = 1;
    data.uFlags = NIF_TIP;
    wcsncpy_s(data.szTip, tooltip_.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void TrayIcon::handle_message(WPARAM wparam, LPARAM lparam) {
    (void)wparam;
    const UINT event = LOWORD(lparam);

    switch (event) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            if (callbacks_.on_show) {
                callbacks_.on_show();
            }
            break;

        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            show_menu();
            break;

        default:
            break;
    }
}

std::wstring TrayIcon::text(const char* key) const {
    if (!i18n_) {
        return widen(key);
    }
    const std::string_view value = i18n_->tr(key);
    // A missing translation returns the key itself; showing "tray.exit" would
    // be worse than showing the tail.
    if (value == key) {
        const size_t dot = value.find('.');
        return widen(std::string(dot == std::string_view::npos ? value
                                                              : value.substr(dot + 1)));
    }
    return widen(std::string(value));
}

void TrayIcon::show_menu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    const bool enabled = callbacks_.get_enabled ? callbacks_.get_enabled() : true;
    const bool startup = callbacks_.get_startup ? callbacks_.get_startup() : false;

    const std::wstring open_label = text("tray.open");
    const std::wstring enabled_label = text("tray.enabled");
    const std::wstring startup_label = text("tray.start_with_windows");
    const std::wstring exit_label = text("tray.exit");

    AppendMenuW(menu, MF_STRING, kMenuShow, open_label.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (enabled ? MF_CHECKED : 0), kMenuToggle,
                enabled_label.c_str());
    AppendMenuW(menu, MF_STRING | (startup ? MF_CHECKED : 0), kMenuStartup,
                startup_label.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuExit, exit_label.c_str());

    POINT cursor{};
    GetCursorPos(&cursor);

    // Required so the menu closes when the user clicks elsewhere, which is not
    // automatic for tray menus.
    SetForegroundWindow(window_);

    const UINT command = static_cast<UINT>(
        TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0,
                       window_, nullptr));

    PostMessageW(window_, WM_NULL, 0, 0);
    DestroyMenu(menu);

    switch (command) {
        case kMenuShow:
            if (callbacks_.on_show) callbacks_.on_show();
            break;
        case kMenuToggle:
            if (callbacks_.on_toggle) callbacks_.on_toggle();
            break;
        case kMenuStartup:
            if (callbacks_.on_startup_changed) callbacks_.on_startup_changed(!startup);
            break;
        case kMenuExit:
            if (callbacks_.on_exit) callbacks_.on_exit();
            break;
        default:
            break;
    }
}

}  // namespace ui
}  // namespace rm
