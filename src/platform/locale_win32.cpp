// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/locale.h"

#include <windows.h>

namespace rm {
namespace platform {

std::string system_locale() {
    // The primary language id is the low ten bits of the LANGID, so a regional
    // variant still reports its language.
    switch (GetUserDefaultUILanguage() & 0x3FF) {
        case LANG_INDONESIAN: return "id";
        default: return "en";
    }
}

std::string normalize_locale(std::string_view code) {
    if (code == "id") {
        return "id";
    }
    if (code == "en") {
        return "en";
    }
    return {};
}

}  // namespace platform
}  // namespace rm
