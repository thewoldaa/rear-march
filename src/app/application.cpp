// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/application.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <d3d11.h>
#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <shellapi.h>
#include <wtsapi32.h>

#include "app/cli.h"
#include "app/single_instance.h"
#include "core/version.h"
#include "input/backend_win32.h"
#include "platform/dpi.h"
#include "platform/beep.h"
#include "platform/locale.h"
#include "platform/paths.h"
#include "platform/startup.h"
#include "ui/fonts.h"
#include "ui/theme.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg,
                                                             WPARAM wparam, LPARAM lparam);

namespace rm {
namespace {

constexpr wchar_t kWindowClass[] = L"RearMarchDashboard";
constexpr wchar_t kWindowTitle[] = L"RearMarch";

// Icon resource id, defined in resources/rearmarch.rc.
constexpr int kIconResourceId = 101;

Application* g_app = nullptr;

bool register_window_class(HINSTANCE instance) {
    HICON icon = LoadIconW(instance, MAKEINTRESOURCEW(kIconResourceId));
    if (!icon) {
        icon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Application::window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClass;
    wc.hIcon = icon;
    wc.hIconSm = icon;

    if (RegisterClassExW(&wc) == 0) {
        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    return true;
}

}  // namespace

Application::Application() = default;

Application::~Application() {
    shutdown();
}

int Application::run(int argc, wchar_t** argv) {
    if (!initialize(argc, argv)) {
        return 1;
    }

    // Render loop. When the dashboard is hidden the loop still runs so the
    // tray keeps responding and a hotkey can bring the window back; it just
    // renders nothing and sleeps longer.
    MSG msg{};
    bool running = true;

    while (running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running || request_exit_) {
            break;
        }
        if (request_hide_) {
            hide_dashboard();
            request_hide_ = false;
        }

        // Hook-thread actions (toggle, calibration trims) land here.
        drain_actions();

        if (window_visible_) {
            render_frame();
            Sleep(16);
        } else {
            Sleep(50);
        }

        if (request_locale_reload_) {
            request_locale_reload_ = false;
            reload_locale();
        }

        // Persist at most once per iteration, never per widget edit.
        if (config_dirty_) {
            config_dirty_ = false;
            publish_runtime_config();
            save_config();
        }
    }

    shutdown();
    return 0;
}

bool Application::initialize(int argc, wchar_t** argv) {
    g_app = this;
    dpi::enable_awareness();

    std::string error;
    if (!CliOptions::parse(argc, argv, options_, error)) {
        MessageBoxA(nullptr, error.c_str(), "RearMarch", MB_ICONERROR);
        return false;
    }
    if (options_.show_help) {
        MessageBoxA(nullptr, usage_text().c_str(), "RearMarch", MB_OK);
        return false;
    }
    if (options_.show_version) {
        const std::string text = std::string("RearMarch ") + version::kVersion;
        MessageBoxA(nullptr, text.c_str(), "RearMarch", MB_OK);
        return false;
    }

    // A second instance must not install a second pair of hooks: the trigger
    // would fire twice per press. Signal the first instance and exit instead.
    if (!single_instance::acquire()) {
        single_instance::send_command("show");
        return false;
    }
    single_instance::set_command_handler([](const std::string& command) {
        if (g_app) {
            g_app->on_command(command);
        }
    });

    // A custom --config path may live anywhere; creating the default data
    // directory in that case would leave an empty %LOCALAPPDATA%\RearMarch
    // behind on a portable or test run. save_config() creates the parent of
    // the path it is given.
    if (options_.config_path.empty()) {
        paths::ensure_data_dir();
    }
    load_config();

    if (options_.safe_mode) {
        // Safe mode ignores the saved configuration entirely. A turn trigger
        // the user cannot control is exactly what safe mode is for.
        config_ = make_default_config();
        config_.enabled = false;
    }

    if (first_run_) {
        // Pick up the language the machine is set to, or the one the installer
        // wizard ran in. Only on a first run: a language chosen in the
        // dashboard is never overridden.
        const std::string detected = options_.locale.empty()
                                         ? platform::system_locale()
                                         : platform::normalize_locale(options_.locale);
        config_.general.locale = detected.empty() ? "en" : detected;
    }

    if (options_.no_autostart) {
        config_.general.start_with_windows = false;
        config_dirty_ = true;
    } else if (first_run_) {
        // The default configuration asks to run at login, so register that
        // once, now. Afterwards the registry is the source of truth and the
        // config only mirrors it.
        startup::set_enabled(config_.general.start_with_windows);
        config_dirty_ = true;
    } else {
        // The registry is the source of truth; the file only mirrors it. Keep
        // the mirror accurate so a config copied to another machine is not
        // mistaken for the actual state there.
        const bool registered = startup::is_enabled();
        if (config_.general.start_with_windows != registered) {
            config_.general.start_with_windows = registered;
            config_dirty_ = true;
        }
    }

    i18n_.load(language_from_code(config_.general.locale),
               paths::locale_dir().string());

    publish_runtime_config();

    worker_ = std::make_unique<TurnWorker>(store_, actions_);
    if (!worker_->start()) {
        MessageBoxW(nullptr, L"RearMarch could not start its turn worker.",
                    L"RearMarch", MB_ICONWARNING);
    }

    backend_ = std::make_unique<InputBackend>(store_, actions_, *worker_);
    if (!options_.safe_mode && !backend_->start()) {
        hook_failed_ = true;
        MessageBoxW(nullptr,
                    L"RearMarch could not install its input hooks.\n"
                    L"Another application may be blocking them, or the desktop is locked.",
                    L"RearMarch", MB_ICONWARNING);
    }

    if (!create_window()) {
        return false;
    }

    session::subscribe(window_, &Application::session_callback);

    tray_ = std::make_unique<ui::TrayIcon>();
    ui::TrayIcon::Callbacks callbacks;
    callbacks.on_show = [this] { show_dashboard(); };
    callbacks.on_toggle = [this] { set_enabled(!config_.enabled); };
    callbacks.on_exit = [this] {
        request_exit_ = true;
        PostQuitMessage(0);
    };
    callbacks.on_startup_changed = [this](bool enabled) {
        startup::set_enabled(enabled);
        config_.general.start_with_windows = enabled;
        config_dirty_ = true;
    };
    callbacks.get_startup = [] { return startup::is_enabled(); };
    callbacks.get_enabled = [this] { return config_.enabled; };
    tray_->set_callbacks(callbacks);
    tray_->set_i18n(&i18n_);
    tray_visible_ = tray_->create(window_, nullptr);
    update_tray();

    dashboard_.config = &config_;
    dashboard_.i18n = &i18n_;
    dashboard_.state = &state_;
    dashboard_.config_dirty = &config_dirty_;
    dashboard_.request_hide = &request_hide_;
    dashboard_.request_exit = &request_exit_;
    dashboard_.request_locale_reload = &request_locale_reload_;
    dashboard_.capture_active = &capture_active_;
    dashboard_.set_calibrating = [this](bool enabled) {
        calibrating_ = enabled;
        publish_runtime_config();
        update_tray();
    };
    dashboard_.set_start_with_windows = [this](bool enabled) {
        startup::set_enabled(enabled);
        // Read back what the registry now says instead of trusting the click:
        // a failed write shows up in the checkbox immediately.
        config_.general.start_with_windows = startup::is_enabled();
    };

    // A first launch that vanished straight into the tray would look like a
    // failed install. Show the dashboard once; from the second launch on, the
    // saved preference wins.
    if (options_.minimized || (config_.general.start_minimized && !first_run_)) {
        hide_dashboard();
    } else {
        show_dashboard();
    }

    initialized_ = true;
    return true;
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }
    initialized_ = false;

    // Hooks first: they read the runtime snapshot and the worker, and must be
    // gone before either is destroyed.
    if (backend_) {
        backend_->stop();
        backend_.reset();
    }
    if (worker_) {
        worker_->stop();
        worker_.reset();
    }

    if (config_dirty_) {
        save_config();
    }

    if (window_) {
        session::unsubscribe(window_);
    }

    if (tray_) {
        tray_->destroy();
        tray_.reset();
        tray_visible_ = false;
    }

    destroy_device();
    if (window_) {
        DestroyWindow(window_);
        window_ = nullptr;
    }

    // The pipe thread and the mutex belong to this process; release them only
    // after everything that could still post a command is gone.
    single_instance::release();
    g_app = nullptr;
}

bool Application::create_window() {
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!register_window_class(instance)) {
        return false;
    }

    // Start hidden; show_dashboard() positions and reveals it.
    // Size the window in physical pixels for the monitor's DPI. The client
    // area has to be large enough for the ImGui layout, which is also scaled:
    // a fixed 720x900 physical window is 576x720 logical at 125% and clips the
    // right-hand column.
    const float initial_scale = dpi::system_scale();
    window_ = CreateWindowExW(WS_EX_APPWINDOW, kWindowClass, kWindowTitle,
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              static_cast<int>(720.0f * initial_scale),
                              static_cast<int>(900.0f * initial_scale),
                              nullptr, nullptr, instance, this);
    if (!window_) {
        return false;
    }

    dpi_scale_ = dpi::scale_for_window(window_);
    RECT work{};
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0)) {
        const int max_width = work.right - work.left - 32;
        const int max_height = work.bottom - work.top - 32;
        const int width = std::clamp(
            static_cast<int>(720.0f * dpi_scale_), 400, std::max(max_width, 400));
        const int height = std::clamp(
            static_cast<int>(900.0f * dpi_scale_), 460, std::max(max_height, 460));
        const int x = work.left + (work.right - work.left - width) / 2;
        const int y = work.top + (work.bottom - work.top - height) / 2;
        SetWindowPos(window_, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Dark title bar, which is otherwise white and jarring next to the theme.
    const BOOL dark = TRUE;
    DwmSetWindowAttribute(window_, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark,
                          sizeof(dark));

    return create_device();
}

bool Application::create_device() {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window_;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained{};

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                      levels, static_cast<UINT>(std::size(levels)),
                                      D3D11_SDK_VERSION, &desc, &swap_chain_, &device_,
                                      &obtained, &context_) != S_OK) {
        return false;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    if (swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)) != S_OK) {
        return false;
    }
    const HRESULT hr = device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_);
    back_buffer->Release();
    if (hr != S_OK) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;  // no imgui.ini next to the exe
    ImGui::GetIO().LogFilename = nullptr;

    ui::apply_theme(dpi_scale_);
    ui::load_fonts(dpi_scale_);

    ImGui_ImplWin32_Init(window_);
    ImGui_ImplDX11_Init(device_, context_);
    return true;
}

void Application::destroy_device() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    if (render_target_) {
        render_target_->Release();
        render_target_ = nullptr;
    }
    if (swap_chain_) {
        swap_chain_->Release();
        swap_chain_ = nullptr;
    }
    if (context_) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
}

void Application::render_frame() {
    if (!window_visible_ || !swap_chain_) {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    state_.hook_running = backend_ && backend_->running() && !hook_failed_;
    state_.locked = locked_;
    state_.calibrating = calibrating_;
    state_.turns_this_session = worker_ ? worker_->turns_done() : 0;
    state_.last_turn_counts = worker_ ? worker_->last_counts() : 0;
    state_.last_turn_sign = worker_ ? worker_->last_sign() : 1;
    state_.has_turned = state_.turns_this_session > 0;
    state_.message.clear();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ui::draw_dashboard(dashboard_);

    ImGui::End();

    // Report the recorder state before rendering, so the hook thread sees it
    // from the next event onward. The dashboard sets capture_active_ when a
    // picker opens; sync_capture_state() publishes the difference once.
    sync_capture_state();

    ImGui::Render();
    const float clear[4] = {0.082f, 0.094f, 0.114f, 1.0f};
    context_->OMSetRenderTargets(1, &render_target_, nullptr);
    context_->ClearRenderTargetView(render_target_, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swap_chain_->Present(1, 0);
}

void Application::load_config() {
    const std::filesystem::path path = options_.config_path.empty()
                                           ? paths::config_file()
                                           : std::filesystem::path(options_.config_path);

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        // No file yet: the defaults are what the user gets, and a few of them
        // (run at login, start minimized) are applied differently on a first
        // launch.
        first_run_ = true;
        config_ = make_default_config();
        return;
    }

    const std::string text((std::istreambuf_iterator<char>(stream)),
                           std::istreambuf_iterator<char>());

    std::string error;
    const auto parsed = from_json(text, error);
    if (!parsed) {
        // Keep the broken file for inspection instead of silently overwriting
        // it, and start from defaults so the app still runs. Treating this
        // like a first run means the dashboard comes up.
        std::error_code ec;
        std::filesystem::copy_file(path, path.string() + ".broken",
                                   std::filesystem::copy_options::overwrite_existing, ec);
        first_run_ = true;
        config_ = make_default_config();
        return;
    }
    config_ = *parsed;
}

void Application::save_config() {
    const std::filesystem::path path = options_.config_path.empty()
                                           ? paths::config_file()
                                           : std::filesystem::path(options_.config_path);

    // A --config path may point into a directory that does not exist yet (a
    // smoke test's temp directory, for example). Creating it here keeps the
    // write atomic instead of special-casing the first save.
    if (!path.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    // Atomic write: temp file then rename, so a crash mid-write cannot leave a
    // truncated config behind.
    const std::filesystem::path temp = path.string() + ".tmp";
    {
        std::ofstream stream(temp, std::ios::binary | std::ios::trunc);
        if (!stream) {
            return;
        }
        const std::string text = to_json(config_);
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    }

    std::error_code ec;
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
    }
}

void Application::publish_runtime_config() {
    RuntimeConfig runtime;
    runtime.enabled = config_.enabled;
    runtime.locked = locked_;
    runtime.calibrating = calibrating_;
    runtime.capture_active = capture_active_;
    runtime.turn = config_.turn;
    runtime.toggle = config_.toggle;
    runtime.calib_up = config_.calibration.up;
    runtime.calib_down = config_.calibration.down;
    runtime.calib_save = config_.calibration.save;
    runtime.calib_step_percent = config_.calibration.step_percent;
    runtime.beep = config_.feedback.beep;
    store_.publish(runtime);
}

void Application::drain_actions() {
    if (const uint32_t count = actions_.take(AppAction::Toggle); count % 2 == 1) {
        set_enabled(!config_.enabled);
    }

    const int up = static_cast<int>(actions_.take(AppAction::CalibrateUp));
    const int down = static_cast<int>(actions_.take(AppAction::CalibrateDown));
    if (up > 0 || down > 0) {
        // Trims only apply in calibration mode. Outside it the keys are still
        // swallowed (the hook returns before the game sees them) but nothing
        // changes, which keeps a stray press from silently rescaling.
        if (calibrating_) {
            for (int i = 0; i < up; ++i) {
                config_.turn.counts_per_360 = trimmed_counts(
                    config_.turn.counts_per_360, config_.calibration.step_percent, true);
            }
            for (int i = 0; i < down; ++i) {
                config_.turn.counts_per_360 = trimmed_counts(
                    config_.turn.counts_per_360, config_.calibration.step_percent, false);
            }
            config_dirty_ = true;
            // The next turn reads the trimmed value; the hook thread already
            // has the old snapshot, so republish now rather than at the end of
            // the frame.
            publish_runtime_config();
            if (worker_ && (up > 0 || down > 0)) {
                worker_->request_feedback(up > 0);
            }
        }
    }

    if (actions_.take(AppAction::CalibrateSave) > 0 && calibrating_) {
        calibrating_ = false;
        config_dirty_ = true;
        publish_runtime_config();
        update_tray();
        if (config_.feedback.beep) {
            Beep(1800, 60);
        }
    }

    if (actions_.take(AppAction::ShowDashboard) > 0) {
        show_dashboard();
    }
}

void Application::apply_action(AppAction action) {
    actions_.post(action);
}

void Application::reload_locale() {
    i18n_.load(language_from_code(config_.general.locale),
               paths::locale_dir().string());
}

void Application::on_session_event(session::SessionEvent event) {
    switch (event) {
        case session::SessionEvent::Locked:
            locked_ = true;
            publish_runtime_config();
            break;
        case session::SessionEvent::Unlocked:
            locked_ = false;
            publish_runtime_config();
            break;
        case session::SessionEvent::Other:
            break;
    }
}

void Application::session_callback(session::SessionEvent event) {
    if (g_app) {
        g_app->on_session_event(event);
    }
}

void Application::show_dashboard() {
    if (!window_) {
        return;
    }

    if (IsIconic(window_)) {
        ShowWindow(window_, SW_RESTORE);
    }
    ShowWindow(window_, SW_SHOW);
    SetForegroundWindow(window_);
    window_visible_ = true;
}

void Application::hide_dashboard() {
    if (!window_) {
        return;
    }
    ShowWindow(window_, SW_HIDE);
    window_visible_ = false;

    // The dashboard is no longer drawing, so a recorder cannot make progress.
    // Releasing the capture flag here prevents a closed window from leaving
    // the trigger disabled until it is opened again.
    if (capture_active_) {
        capture_active_ = false;
        publish_runtime_config();
    }
}

void Application::set_enabled(bool enabled) {
    config_.enabled = enabled;
    // Leaving calibration mode when the trigger is disabled would be
    // surprising; keep it, the dashboard shows the state either way.
    config_dirty_ = true;
    publish_runtime_config();
    update_tray();
    if (config_.feedback.beep) {
        platform::beep();
    }
}

void Application::toggle_calibration() {
    calibrating_ = !calibrating_;
    config_dirty_ = true;
    publish_runtime_config();
    update_tray();
}

void Application::update_tray() {
    if (!tray_) {
        return;
    }
    ui::TrayStatus status = ui::TrayStatus::Active;
    if (calibrating_) {
        status = ui::TrayStatus::Calibrating;
    } else if (!config_.enabled) {
        status = ui::TrayStatus::Disabled;
    }
    tray_->set_status(status);
}

void Application::sync_capture_state() {
    // capture_active_ is set by the capture widgets through the dashboard
    // pointer; publish when it changed so the hook thread stops interpreting.
    if (capture_active_ != store_.current()->capture_active) {
        publish_runtime_config();
    }
}

void Application::on_command(const std::string& command) {
    if (command == "show") {
        // Posted rather than called directly: the command arrives on the pipe
        // thread and window calls must happen on the UI thread.
        PostMessageW(window_, WM_APP + 10, 0, 0);
    }
}

LRESULT CALLBACK Application::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }

    switch (msg) {
        case WM_SIZE:
            if (g_app && g_app->swap_chain_ && wparam != SIZE_MINIMIZED &&
                LOWORD(lparam) > 0 && HIWORD(lparam) > 0) {
                if (g_app->render_target_) {
                    g_app->render_target_->Release();
                    g_app->render_target_ = nullptr;
                }
                g_app->swap_chain_->ResizeBuffers(0, LOWORD(lparam), HIWORD(lparam),
                                                  DXGI_FORMAT_UNKNOWN, 0);
                ID3D11Texture2D* back_buffer = nullptr;
                if (g_app->swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)) == S_OK) {
                    g_app->device_->CreateRenderTargetView(back_buffer, nullptr,
                                                           &g_app->render_target_);
                    back_buffer->Release();
                }
            }
            return 0;

        case WM_CLOSE:
            // Closing hides to the tray instead of exiting, which is what a
            // background utility should do. Without a tray icon there would be
            // no way back, so the window closes for real instead.
            if (g_app) {
                if (g_app->tray_visible_) {
                    g_app->request_hide_ = true;
                } else {
                    g_app->request_exit_ = true;
                }
            }
            return 0;

        case WM_APP + 10:
            if (g_app) {
                g_app->show_dashboard();
            }
            return 0;

        case WM_WTSSESSION_CHANGE:
            session::handle_session_change(static_cast<unsigned long>(wparam));
            return 0;

        case WM_DPICHANGED:
            if (g_app) {
                g_app->dpi_scale_ = static_cast<float>(HIWORD(wparam)) / 96.0f;
                const RECT* suggested = reinterpret_cast<const RECT*>(lparam);
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                if (ImGui::GetCurrentContext()) {
                    ui::apply_theme(g_app->dpi_scale_);
                    ui::rebuild_fonts(g_app->dpi_scale_);
                }
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    if (msg == ui::TrayIcon::message_id() && g_app && g_app->tray_) {
        g_app->tray_->handle_message(wparam, lparam);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace rm
