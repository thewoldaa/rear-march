# Building RearMarch

RearMarch builds on Windows with Visual Studio and CMake. There is nothing to
install beyond the toolchain: the three third-party libraries are downloaded and
checksum-verified during the configure step.

## Prerequisites

| Requirement | Notes |
|---|---|
| Windows 10 or later, x64 | Development is on Windows 11 |
| Visual Studio 2022 | The "Desktop development with C++" workload |
| CMake 3.24 or newer | Any recent installer; it does not need to be on `PATH` if you use the VS-bundled copy |
| PowerShell 5.1 | Only for regenerating the icon |
| Inno Setup 6 | Only for the installer |

Python, NASM, vcpkg, and a network proxy are not required.

## Configure and build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable is `build/Release/RearMarch.exe`. The build copies `locales/`
next to it so a plain build tree behaves like an installed one.

To build a single target while you work:

```powershell
cmake --build build --config Release --target rearmarch_core
```

## Tests

```powershell
cmake --build build --config Release --target rearmarch_tests
.\build\tests\Release\rearmarch_tests.exe
```

or through CTest:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The suite covers the platform-independent core only (`rearmarch_core`): the turn
calculation and event splitting, configuration round-trips and repair, key
names, chords, the runtime snapshot, and command-line parsing. Anything that
needs a window station, a hook, or a display is verified by running the
application.

`--success` and `--test-case=<name>` are doctest options and work as expected.

## Build options

| Option | Default | Effect |
|---|---|---|
| `REARMARCH_BUILD_TESTS` | `ON` | Build the unit test suite |
| `REARMARCH_STATIC_RUNTIME` | `ON` | Link the MSVC runtime statically (`/MT`) |
| `REARMARCH_WARNINGS_AS_ERRORS` | `OFF` | Turn warnings into errors |

Static linking is the default because it makes the portable zip a single
executable with no VC++ redistributable to install.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
    -DREARMARCH_WARNINGS_AS_ERRORS=ON
```

## Dependencies

`cmake/Dependencies.cmake` fetches Dear ImGui, nlohmann/json, and doctest with
`FetchContent` using release **tarballs** pinned by SHA256, not git clones.
Downloads are cached under `build/_deps`, so reconfiguring is offline-friendly
after the first run.

To use a local copy of a dependency (for patching ImGui, say), point the
matching `FETCHCONTENT_SOURCE_DIR_*` variable at it:

```powershell
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_IMGUI=C:\src\imgui
```

The variable names follow the FetchContent convention:
`FETCHCONTENT_SOURCE_DIR_IMGUI`, `FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON`,
`FETCHCONTENT_SOURCE_DIR_DOCTEST`.

## The icon and version resource

`resources/rearmarch.ico` is generated, not hand-drawn:

```powershell
powershell -ExecutionPolicy Bypass -File tools/make_icon.ps1
```

It emits 16, 24, 32, 48, 64, 128, and 256 pixel frames; the two largest are PNG
compressed, the rest are 32-bit DIBs. `resources/rearmarch.rc.in` is filled in by
CMake at configure time so the version block cannot drift from
`project(VERSION ...)` - bump the version in `CMakeLists.txt` and nowhere else.

## Packaging

Portable zip (no installer):

```powershell
cmake --build build --config Release --target package
```

It lands in `build/RearMarch-<version>-win64.zip` and unpacks into a folder of
the same name: the executable, `locales/`, `docs/`, and the top-level markdown
files. CPack pulls the same `install()` rules the installer uses, so there is no
second file list to keep in sync.

Installer, which needs [Inno Setup 6](https://jrsoftware.org/isdl.php) - `iscc`
on `PATH`, or the default per-user location used below:

```powershell
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" installer\rearmarch.iss
```

The result is `installer/Output/RearMarch-<version>-setup.exe`, a per-user
install that never asks for elevation. The script takes the executable and the
translations from `build/Release/`, so this route expects a Release build too.
`installer/languages/Indonesian.isl` is the official *unofficial* translation
committed to the repository, because Inno Setup ships English but not
Indonesian.

The version appears in three places by design: `project(VERSION ...)` in
`CMakeLists.txt` drives the executable and the zip, `AppVersion` in
`installer/rearmarch.iss` drives the installer, and `CHANGELOG.md` records the
release. Bump all three together.

## The synthetic UAT script

`tools/smoke_test.ps1` launches the real executable with a temporary
configuration, presses the trigger with `keybd_event`, and measures what the
cursor does. It also exercises the toggle, persistence, calibration trimming,
saving, and recorder suppression.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\smoke_test.ps1 `
    -Config "$env:TEMP\rearmarch-smoke\config.json"
```

The script creates the configuration if it is missing and points the
application at it, so your own `%LOCALAPPDATA%\RearMarch\config.json` is
untouched. It requires an unlocked, interactive desktop: the trigger is a
synthetic key event and the calibration click uses real cursor coordinates.

`tools/screenshot.ps1` captures the dashboard for documentation, and
`tools/make_icon.ps1` regenerates the icon.

## Troubleshooting

**The build cannot find the resource compiler.** Install the "Desktop
development with C++" workload; `rc.exe` ships with the Windows SDK component.

**`defaultlib 'MSVCRT' conflicts` when linking.** Something is being built with
a different runtime than the rest. Check that `REARMARCH_STATIC_RUNTIME` is set
consistently and do a clean configure after deleting `build/`.

**The application starts with the trigger disabled.** It was launched with
`--safe-mode`, or the input hooks could not be installed. Check the status line
in the dashboard; another application may be blocking low-level hooks.

**The trigger fires but the game does not turn.** The game ignores injected
movement or clamps it. See [GAME-COMPAT.md](GAME-COMPAT.md).

**The smoke test fails to click Calibrate.** The dashboard must be foreground
and the display must not change between the screenshot and the click. Close
anything that steals focus, and make sure no other copy of RearMarch is running.
