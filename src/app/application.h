// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <windows.h>

#include "app/cli.h"
#include "core/action_mailbox.h"
#include "core/config.h"
#include "core/i18n.h"
#include "core/runtime_config.h"
#include "input/turn_worker.h"
#include "platform/session.h"
#include "ui/dashboard.h"
#include "ui/tray.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

namespace rm {

class InputBackend;

// Owns the application lifetime: config, input hooks, turn worker, tray icon,
// and the DirectX 11 dashboard window.
class Application {
public:
    Application();
    ~Application();

    int run(int argc, wchar_t** argv);

    // The window procedure is a static C callback, so it must be reachable
    // from the file-scope WNDCLASSEXW registration helper.
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
    bool initialize(int argc, wchar_t** argv);
    void shutdown();

    bool create_window();
    bool create_device();
    void destroy_device();
    void render_frame();

    void load_config();
    void save_config();

    // Builds the hook-thread snapshot from the current config and publishes
    // it. Called after every change, before the file is written.
    void publish_runtime_config();
    void apply_action(AppAction action);
    void drain_actions();
    void reload_locale();

    void on_session_event(session::SessionEvent event);
    static void session_callback(session::SessionEvent event);

    void show_dashboard();
    void hide_dashboard();
    void set_enabled(bool enabled);
    void toggle_calibration();
    void update_tray();

    void on_command(const std::string& command);
    void sync_capture_state();

    HWND window_ = nullptr;
    bool window_visible_ = false;
    bool tray_visible_ = false;

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swap_chain_ = nullptr;
    ID3D11RenderTargetView* render_target_ = nullptr;

    AppConfig config_;
    CliOptions options_{};
    I18n i18n_;

    RuntimeStore store_;
    ActionMailbox actions_;
    std::unique_ptr<TurnWorker> worker_;
    std::unique_ptr<InputBackend> backend_;
    std::unique_ptr<ui::TrayIcon> tray_;

    ui::DashboardState state_{};
    ui::DashboardContext dashboard_{};

    bool config_dirty_ = false;
    bool request_hide_ = false;
    bool request_exit_ = false;
    bool request_locale_reload_ = false;
    bool capture_active_ = false;
    bool calibrating_ = false;
    bool locked_ = false;

    bool initialized_ = false;
    bool first_run_ = false;
    bool hook_failed_ = false;
    float dpi_scale_ = 1.0f;

    // Calibration trims are batched per UI iteration: a key held on the trim
    // chord would otherwise throw away a hundred slider updates per second.
    int pending_trims_ = 0;
};

}  // namespace rm
