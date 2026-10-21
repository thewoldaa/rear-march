// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace rm {
namespace single_instance {

// Acquires the single-instance mutex and starts the command pipe listener.
// Returns false when another instance already owns it.
bool acquire();

// Sends a command ("show") to the running instance.
bool send_command(const std::string& command);

// Called on the pipe thread when a command arrives.
void set_command_handler(void (*handler)(const std::string& command));

void release();

}  // namespace single_instance
}  // namespace rm
