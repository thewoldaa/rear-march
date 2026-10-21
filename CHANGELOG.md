# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-10-21

The first release. RearMarch turns a game camera by a configured angle with one
key press, and lets the exact count be calibrated in game.

### Added

- **One-press turn.** A configurable 5-360 degree turn, left, right, or random,
  with 90/180/270 degree presets. The default is 180 degrees to the right.
- **Keyboard or mouse trigger.** Any key or a mouse side button (X1/X2),
  recorded by pressing it. The default is X1.
- **Calibration mode.** The trigger keeps working in game while the dashboard
  shows the live count. `Ctrl+Alt+Up` and `Ctrl+Alt+Down` trim the count by a
  configurable percentage and beep in different pitches; `Ctrl+Alt+Enter` saves
  and leaves the mode.
- **DPI multiplier** from 1000 to 16000 on top of the calibrated count.
- **Optional ramp** of 0 to 200 ms, spreading the turn over several events.
- **Enable toggle** on `Ctrl+Alt+M`, available from the dashboard, the tray menu,
  and the hotkey. Switching it plays a short beep.
- **Tray icon** with the current state, a startup toggle, and a quit command.
  Closing the dashboard hides it instead of exiting.
- **Start with Windows**, written to the current user's `Run` key and mirrored
  in the configuration.
- **English and Indonesian**, detected from Windows on first run and changeable
  from the dashboard.
- **Safe mode and portable use.** `--safe-mode`, `--config`, `--minimized`,
  `--no-autostart`, `--locale`, `--help`, and `--version`.
- **Portable zip and per-user installer.** Neither needs administrator rights.
- **Test suite.** Unit coverage of the turn calculation, configuration,
  keycodes, chords, runtime snapshot, and command line, plus a synthetic UAT
  script that drives the real application.

### Fixed during development

- The Calibrate button could be overwritten by the per-frame state refresh and
  never enter calibration.
- Toggling "Start with Windows" did not write the registry until the next
  launch.
- An open trigger recorder did not suppress the trigger, so pressing the button
  being bound could fire a turn.
- The DPI input field clipped its last digit.
- Calibration trim beeps played outside calibration mode.
- A `--config` path in a missing directory failed to save.
- The status line could be pushed out of the window at 125% scaling.

[Unreleased]: https://github.com/thewoldaa/rear-march/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/thewoldaa/rear-march/releases/tag/v0.1.0
