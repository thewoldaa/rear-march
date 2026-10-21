// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/single_instance.h"

#include <atomic>
#include <thread>

#include <windows.h>

namespace rm {
namespace single_instance {
namespace {

// Deliberately different from KeyMouse's names: the two applications can run
// side by side, and a shared name would make each start think the other was
// itself.
constexpr wchar_t kMutexName[] = L"Local\\RearMarch.Instance.v1";
constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\RearMarch.Command.v1";

HANDLE g_mutex = nullptr;
std::thread g_listener;
std::atomic<bool> g_stop{false};
void (*g_handler)(const std::string&) = nullptr;

void listener_main() {
    while (!g_stop.load()) {
        HANDLE pipe = CreateNamedPipeW(
            kPipeName,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 512, 512, 0, nullptr);

        if (pipe == INVALID_HANDLE_VALUE) {
            Sleep(250);
            continue;
        }

        // Wait for a client without blocking shutdown. ConnectNamedPipe with a
        // short timeout would be cleaner, but overlapped I/O complicates
        // shutdown for no benefit at this scale.
        const BOOL connected = ConnectNamedPipe(pipe, nullptr);
        const DWORD error = connected ? ERROR_SUCCESS : GetLastError();

        if (g_stop.load()) {
            CloseHandle(pipe);
            break;
        }

        if (connected || error == ERROR_PIPE_CONNECTED) {
            char buffer[128]{};
            DWORD read = 0;
            if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &read, nullptr) && read > 0) {
                std::string command(buffer, read);
                while (!command.empty() &&
                       (command.back() == '\0' || command.back() == '\n' ||
                        command.back() == '\r')) {
                    command.pop_back();
                }
                if (g_handler && !command.empty()) {
                    g_handler(command);
                }
            }
            FlushFileBuffers(pipe);
            DisconnectNamedPipe(pipe);
        }
        CloseHandle(pipe);
    }
}

}  // namespace

bool acquire() {
    g_mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!g_mutex) {
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_mutex);
        g_mutex = nullptr;
        return false;
    }

    g_stop.store(false);
    g_listener = std::thread(listener_main);
    return true;
}

bool send_command(const std::string& command) {
    // Wait briefly for the pipe: the first instance may still be starting.
    if (!WaitNamedPipeW(kPipeName, 2000)) {
        return false;
    }

    HANDLE pipe = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const bool ok = WriteFile(pipe, command.data(),
                              static_cast<DWORD>(command.size()), &written, nullptr) != FALSE;
    CloseHandle(pipe);
    return ok && written == command.size();
}

void set_command_handler(void (*handler)(const std::string& command)) {
    g_handler = handler;
}

void release() {
    g_stop.store(true);

    // Unblock ConnectNamedPipe by connecting to ourselves.
    HANDLE pipe = CreateFileW(kPipeName, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
    }

    if (g_listener.joinable()) {
        g_listener.join();
    }

    if (g_mutex) {
        ReleaseMutex(g_mutex);
        CloseHandle(g_mutex);
        g_mutex = nullptr;
    }
}

}  // namespace single_instance
}  // namespace rm
