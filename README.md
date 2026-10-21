# RearMarch

One key. Half a turn.

RearMarch is a small Windows tray utility that turns a game camera around by
injecting relative mouse movement. Press a mouse side button or a key and the
view swings 180 degrees - or whatever angle you configure - without moving your
hand off the controls.

It was built for players who want a quick rear check, and for anyone who finds a
180 degree flick uncomfortable to repeat. It is not a macro engine, a
clicker, or an aim aid: it sends one known number of counts in one direction
when you press one button.

[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey.svg)](#requirements)

## Status

Version 0.1.0. Windows only. The calculation and configuration layers are
platform independent; the input and UI layers are Win32.

## Features

- **Adjustable turn.** 5 to 360 degrees, left, right, or random. Presets for
  90, 180, and 270 degrees.
- **Keyboard or mouse trigger.** Bind a key or a mouse side button (X1/X2) by
  pressing it. The default is X1.
- **Calibration mode.** Turn in game, trim the count up or down with a hotkey
  until the turn is exact, then save. No need to leave the game.
- **DPI multiplier** from 1000 to 16000 on top of the calibrated value.
- **Optional ramp** that spreads the movement over up to 200 ms.
- **Two languages.** English and Indonesian, detected from Windows on first
  run and changeable at any time.
- **Tray first.** Closing the window keeps it running; the tray menu shows the
  state and can disable or quit it.
- **Tiny and self-contained.** One statically linked executable, no runtime
  dependencies, no network access.
- **Portable or installed.** A portable zip and a per-user installer, neither of
  which needs administrator rights.

## Requirements

- Windows 10 or later, 64-bit.
- A mouse with X1/X2 side buttons, or any keyboard for a key trigger.

No administrator rights are needed. Input cannot reach an elevated window such
as Task Manager unless RearMarch itself is run elevated.

## Install

Each release is published in two forms:

- **`RearMarch-0.1.0-setup.exe`** - per-user install, no administrator prompt
  and no `Program Files`. The wizard speaks English and Indonesian, copies the
  documentation next to the application, and offers to start RearMarch at
  login. Uninstalling asks whether to keep your settings.
- **`RearMarch-0.1.0-win64.zip`** - portable. Unpack it anywhere and run
  `RearMarch.exe`; nothing is written outside `%LOCALAPPDATA%\RearMarch`.

Both are unsigned, so Windows SmartScreen may warn the first time.

## Usage

Run `RearMarch.exe`.

The first start opens the dashboard, picks the language Windows is set to, and
registers RearMarch to start with Windows. Afterwards it starts in the tray with
the dashboard closed: double-click the tray icon, or run the executable again,
to open it. Closing the window hides it - use **Quit** in the tray menu to exit.

Defaults:

| Action | Input |
|---|---|
| Turn | Mouse X1 (the rear thumb button) |
| Enable or disable | `Ctrl+Alt+M` |
| Calibrate up | `Ctrl+Alt+Up` |
| Calibrate down | `Ctrl+Alt+Down` |
| Save calibration and leave | `Ctrl+Alt+Enter` |

The trigger and all three calibration chords are rebindable by pressing the
button next to them. The toggle is fixed at `Ctrl+Alt+M` in 0.1.0.

### Calibrating

Every game applies its own sensitivity, so the count that produces a perfect
180 degrees differs between games. Calibration measures the one number that
matters: how many mouse counts a full circle takes at 1000 DPI.

1. Open RearMarch and click **Calibrate**. The tray icon turns blue and the
   dashboard shows the live count.
2. Switch back to the game. The trigger still works.
3. Turn once. If the view falls short, press `Ctrl+Alt+Up`; if it overshoots,
   press `Ctrl+Alt+Down`. Each press changes the count by the configured trim
   step (2% by default) and plays a higher or lower beep, so you can calibrate
   without looking at the dashboard.
4. Press `Ctrl+Alt+Enter` to save and leave calibration. The value is written
   to the configuration immediately.

Calibration changes `counts_per_360`; the DPI field stays a separate multiplier.
A value calibrated at one game sensitivity is only correct for that game, so
recalibrate when you change the game's sensitivity.

### Command line

| Option | Effect |
|---|---|
| `--minimized` | Start hidden in the tray |
| `--safe-mode` | Load defaults with the trigger disabled, to escape a bad binding |
| `--no-autostart` | Never create or change the Windows startup entry |
| `--locale CODE` | Start in `en` or `id` (first run only) |
| `--config PATH` | Use an alternative configuration file |
| `-h`, `--help` | Show the usage text |
| `-v`, `--version` | Show the version |

## Configuration

Settings live in `%LOCALAPPDATA%\RearMarch\config.json`. The file is
human-readable and hand-editable; RearMarch validates and repairs it on load.
A malformed file is copied to `config.json.broken` and defaults are used, so the
application always starts.

```json
{
  "schema": 1,
  "enabled": true,
  "general": {
    "start_with_windows": true,
    "start_minimized": true,
    "locale": "en"
  },
  "turn": {
    "trigger": "X1",
    "angle_deg": 180.0,
    "direction": "right",
    "dpi": 1000,
    "counts_per_360": 6000.0,
    "ramp_ms": 0
  },
  "toggle": {
    "mods": ["ctrl", "alt"],
    "key": "KeyM"
  },
  "calibration": {
    "step_percent": 2.0,
    "up": { "mods": ["ctrl", "alt"], "key": "ArrowUp" },
    "down": { "mods": ["ctrl", "alt"], "key": "ArrowDown" },
    "save": { "mods": ["ctrl", "alt"], "key": "Enter" }
  },
  "feedback": {
    "beep": true
  }
}
```

`trigger` accepts `X1`, `X2`, or a canonical key name such as `KeyQ` or `F7`.
The direction is `left`, `right`, or `random`.

### The calculation

One press sends:

```
counts = counts_per_360 x (angle_deg / 360) x (dpi / 1000)
```

The result is rounded to the nearest count and split into mouse events of about
256 counts, with a maximum of 128 events; if 128 events would not be enough,
each event carries proportionally more. Splitting matters because many games
clamp a single mouse event; the remainder is added to the first events so the
total is exact. With the defaults that is 3000 counts in 12 events.

`counts_per_360` is the calibrated value at 1000 DPI. `dpi` is a convenience
multiplier, not a measurement of your physical mouse: setting it to 1600 sends
1.6 times as many counts.

## Building

Prerequisites: Visual Studio 2022 (Desktop C++ workload) and CMake 3.24+.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The binary lands in `build/Release/RearMarch.exe`. Third-party dependencies
(Dear ImGui, nlohmann/json, doctest) are downloaded and checksum-verified at
configure time; there are no submodules to initialise. Building the installer
and the zip, and the build options, are covered in
[docs/BUILDING.md](docs/BUILDING.md).

## How it works

RearMarch installs two low-level hooks - one keyboard, one mouse - on a
dedicated thread. The hooks only observe: they recognise the trigger, toggle,
and calibration chords, post an action, and always pass the original input
through to the foreground application. A separate worker thread performs the
injection, so no `SendInput` call ever runs inside a hook callback. The UI is a
Dear ImGui dashboard on DirectX 11 and does not sit on the input path.

The full design is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Known limitations

| Limitation | Reason |
|---|---|
| Some games ignore injected movement | They read Raw Input or filter injected events; see [docs/GAME-COMPAT.md](docs/GAME-COMPAT.md) |
| Kernel anti-cheat may block or flag injection | Out of scope; RearMarch does not attempt to evade detection |
| Elevated games do not receive input | Windows UIPI. Run RearMarch elevated if you need this |
| The lock screen and UAC prompts are unreachable | Windows secure desktop, by design |
| Pointer acceleration affects the physical distance | The count is exact, but Windows' pointer settings change how far the cursor travels |
| A keyboard trigger ignores modified presses | `Ctrl+Q` does not fire a trigger bound to `Q`, so crouching cannot turn the camera |
| The toggle chord is fixed | It is not rebindable in 0.1.0 |
| Mouse buttons other than X1/X2 cannot be triggers | Left, right, and middle click belong to the game |
| Windows SmartScreen warns on first run | The binaries are unsigned. Build from source if you prefer |

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

Third-party components are MIT licensed and GPL-compatible: Dear ImGui,
nlohmann/json, doctest.
