# Architecture

## Goals

1. **Injection never runs in a hook callback.** Windows removes a low-level
   hook that takes too long, and `SendInput` is slow enough to matter. The
   callback recognises input and returns.
2. **The hooks never swallow input.** The game must see every key and button
   exactly as pressed, including the trigger itself.
3. **The idle cost is near zero.** A tray utility that polls or renders in the
   background is a bug.
4. **The logic is testable without Windows.** The turn calculation,
   configuration, key names, and chords live in a library that must not include
   `<windows.h>`.

## Process layout

```
RearMarch.exe
├── UI thread      message pump, ImGui, DirectX 11, tray icon, config
├── hook thread    WH_KEYBOARD_LL + WH_MOUSE_LL, one message loop
└── turn worker    injection and calibration feedback
```

The three threads share only atomics, a single-writer snapshot pointer, and
small fixed-size queues. There is no lock on the input path.

### The hook thread

`SetWindowsHookEx(WH_KEYBOARD_LL, ...)` and `WH_MOUSE_LL` deliver callbacks on
the thread that installed them, and that thread must pump messages. Sharing the
UI thread would tie input latency to frame time, and Windows silently removes a
hook whose callback exceeds the system timeout. A dedicated thread that only
pumps messages keeps the callback to a few comparisons.

The keyboard hook is required. The mouse hook is optional; a keyboard trigger
still works if it fails to install. Both are observer hooks and always return
`CallNextHookEx`.

### The turn worker

A press sets a pending sign and signals an event. The worker wakes, builds the
turn plan from the current runtime snapshot, and injects it. The event is
auto-reset and the pending turn is a single flag, so a burst of presses cannot
grow an unbounded queue: the worker turns once per wake and drains the latest
request.

Ramp mode spreads the same event list over the configured time with the
high-resolution waitable timer. The last event is sent immediately instead of
after a final sleep.

### The UI thread

Owns `AppConfig`, the dashboard, the tray icon, and the DirectX device. It
publishes a new runtime snapshot after every change, before writing the file.
When the window is hidden the loop still pumps messages and sleeps 50 ms; it
does not render.

## Data flow

```
keyboard/mouse event
        │
        ▼
hook thread ── matches trigger? ──► TurnWorker::request_turn(sign)
        │                                   │
        │ matches toggle/calibration?       ▼
        └──► ActionMailbox ──► UI thread   turn worker ──► SendInput
                                   │
                                   └──► AppConfig ──► RuntimeStore
                                                         │
                              hook thread reads ◄────────┘
```

### Runtime snapshots

The hook thread must not lock, allocate, or touch a reference count. The UI
therefore publishes an immutable `RuntimeConfig` and the hook reads a raw
pointer with one acquire load. `RuntimeStore` retains every published snapshot
for the lifetime of the process, which costs a few hundred bytes per settings
change and removes the reclamation problem entirely.

The snapshot carries everything the input path needs: enable state, lock state,
calibration state, capture state, the turn settings, all chords, the trim step,
and the beep preference. A change is visible to the hook from the next event.

### The action mailbox

The hook thread cannot mutate the configuration. It posts counted actions -
toggle, trim up, trim down, save calibration, show dashboard - into an array of
atomics. The UI drains each counter with an exchange, so two presses between
frames are applied twice rather than collapsing into one. The mailbox never
allocates.

### Capture suppression

While a recorder in the dashboard is open, pressing the trigger would fire a
turn for the very input the user is trying to bind. The dashboard sets
`capture_active` while any recorder is open; the flag is published in the
runtime snapshot and the hook ignores the trigger while it is set. The flag is
recomputed from the open recorders every frame, so closing a recorder always
releases it. Hiding the dashboard also clears it.

## Modules

```
src/
├── core/          platform independent, no <windows.h>
│   ├── action_mailbox  lock-free hook-to-UI action counters
│   ├── chord      modifier mask + key, parse and format
│   ├── config     JSON schema, defaults, sanitize, validate
│   ├── i18n       locale loading with English fallback
│   ├── keycodes   canonical key ids and display labels
│   ├── runtime_config  the immutable hook-thread snapshot and store
│   ├── turn       trigger model and the turn calculation
│   └── version    version string from CMake
├── input/         Windows input path
│   ├── backend_win32   both low-level hooks and chord matching
│   ├── injection       SendInput batch with an injection tag
│   ├── keycodes_win32  virtual-key to canonical key mapping
│   └── turn_worker     the injection thread
├── platform/      small Win32 services
│   ├── beep, dpi, locale, mouse_state, session, startup, timer
│   └── paths      %LOCALAPPDATA%\RearMarch and the locales directory
├── ui/            Dear ImGui dashboard
│   ├── capture    key and chord recorder widgets
│   ├── dashboard  layout and controls
│   ├── fonts, theme
│   └── tray       notification icon, menu, status colours
├── app/
│   ├── application  lifetime, window, config, actions, tray wiring
│   ├── cli          argument parsing
│   └── single_instance  named mutex + named pipe to raise the running copy
└── main.cpp
```

## Turn calculation

```
effective_counts_per_360 = counts_per_360 * (dpi / 1000)
turn_counts              = round(effective_counts_per_360 * angle / 360)
```

The result is clamped to at least one count. It is split into
`ceil(counts / 256)` events, clamped to 128 events; with more than 128 events
each event carries proportionally more than 256 counts. The remainder is spread
over the first events, so the sum is exactly `turn_counts`. All events go out in
one `SendInput` call when the ramp is zero.

Splitting is compatibility work: many games clamp the delta of a single mouse
event, and one 9000-count event can arrive as a fraction of the intended turn.

## Threading rules

- The hook callback may not allocate, lock, do file I/O, or call `SendInput`.
- Anything shared with the hook is either an atomic or a pointer to an immutable
  snapshot that is never freed.
- The UI owns the configuration. The worker reads snapshots; it never writes
  configuration.
- The worker and hooks are stopped before the runtime store is destroyed.

## Session handling

`WTSRegisterSessionNotification` reports lock and unlock. While the workstation
is locked the runtime snapshot says so, and the trigger does nothing. The toggle
and calibration chords remain active so the application can be recovered
without a keyboard that is still routed to the game.

## Single instance

A named mutex (`Local\RearMarch.Instance.v1`) makes the second instance exit. It
first connects to `\\.\pipe\RearMarch.Command.v1` and sends `show`, so launching
the executable again raises the existing dashboard instead of starting a second
pair of hooks. The mutex name is also the installer's `AppMutex`, so Setup asks
the running copy to close before replacing it.
