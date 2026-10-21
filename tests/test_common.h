// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// doctest wants to print its assertion values; this header gives it an ostream
// operator for the project's small structs so a failed CHECK shows the actual
// chord instead of a byte count.
#include <ostream>
#include <string>

#include <doctest/doctest.h>

#include "core/chord.h"
#include "core/turn.h"

namespace rm {

inline std::ostream& operator<<(std::ostream& stream, const Chord& chord) {
    const std::string text = format_chord(chord);
    return stream << (text.empty() ? "(invalid)" : text);
}

inline std::ostream& operator<<(std::ostream& stream, const Trigger& trigger) {
    return stream << trigger_to_string(trigger);
}

inline std::ostream& operator<<(std::ostream& stream, Direction direction) {
    return stream << direction_name(direction);
}

}  // namespace rm
