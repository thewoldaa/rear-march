// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace rm {
namespace platform {

// The UI language Windows is set to, as a code this application ships
// ("en" or "id"). Anything else reports "en".
std::string system_locale();

// The two-letter language code for a locale string, or empty when it is not
// one of the supported codes.
std::string normalize_locale(std::string_view code);

}  // namespace platform
}  // namespace rm
