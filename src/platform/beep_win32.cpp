// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/beep.h"

#include <windows.h>

namespace rm {
namespace platform {

void beep() {
    // MessageBeep returns immediately and respects the user's sound scheme,
    // including "no sounds". Blocking Beep() would stall the render loop.
    MessageBeep(MB_OK);
}

}  // namespace platform
}  // namespace rm
