# Game compatibility

RearMarch injects movement through `SendInput` with `MOUSEEVENTF_MOVE`, the
standard Windows relative-mouse path. Some games read it, some do not, and some
actively reject it. This page explains which is which and what to do about it.

## What works

| Input model | RearMarch works? | Notes |
|---|---|---|
| Standard Win32 message queue | Yes | Most desktop games and every non-game application |
| DirectInput (legacy) | Yes | DirectInput sits on the same path |
| Raw Input, non-exclusive | Usually | The game sees the delta; a few filter injected events |
| Raw Input, exclusive (`RIDEV_NOLEGACY`) | Sometimes | Depends on whether the game filters injected input |
| Kernel-level anti-cheat | No | See below |

The count that reaches the game is exact. What the game does with it depends on
its sensitivity, its own acceleration curve, and whether it reads raw deltas or
the cursor position.

## What does not work, and why

### Anti-cheat

Competitive titles with kernel-level anti-cheat (Vanguard, Easy Anti-Cheat,
BattlEye, FACEIT) can detect injected input and may flag the account. RearMarch
does not attempt to evade detection, spoof hardware, or disguise injected
events. If a game blocks injected input, that is the game's decision and it
stands.

This is a hard boundary, not a limitation to be worked around.

### Exclusive Raw Input

Some games register raw input with `RIDEV_NOLEGACY` and read `RAWINPUT`
directly. Whether injected motion reaches them depends on the game and on its
input library. There is no general fix; the game decides.

### Games that clamp a single event

Many engines limit how far one mouse event may turn the view. RearMarch splits a
turn into events of at most 256 counts for exactly this reason. A very large
turn (a high DPI multiplier on a high `counts_per_360`) can still exceed a
game's per-event clamp; lower the DPI multiplier or the angle if the turn
arrives short.

### Elevated games

A non-elevated RearMarch cannot inject into an elevated process because of
Windows UIPI. Run RearMarch as administrator if you need this. Running elevated
also means the dashboard runs elevated, which is a larger trust surface.

### The lock screen and UAC

Windows switches to a secure desktop for the lock screen and UAC prompts. No
normal application can inject there, and RearMarch deliberately stops trying
while the workstation is locked.

## Calibrating for a game

Every game applies its own sensitivity. `counts_per_360` is the number that
makes a full circle in one particular game at one particular sensitivity, so
calibrate per game:

1. Set the game's sensitivity to the value you actually play with.
2. Open RearMarch, click **Calibrate**, then return to the game.
3. Turn 180 degrees with the trigger. If the view stops short, press
   `Ctrl+Alt+Up`; if it overshoots, press `Ctrl+Alt+Down`.
4. Repeat until the turn lands where you want it, then press `Ctrl+Alt+Enter`.

The beeps tell you which way the value moved without leaving the game: higher
for more counts, lower for fewer. The dashboard shows the live value if you
alt-tab back.

If you change the game's sensitivity or switch games, recalibrate. The DPI field
is a multiplier on top of the calibrated value, not a replacement for it.

## Practical advice

- **Try it first.** Test with a harmless binding before relying on it in a
  match.
- **Bind a key the game does not use.** `Ctrl+Alt+Arrow` is a common choice for
  the trim chords precisely because games rarely claim it.
- **Prefer a mouse side button as the trigger.** X1/X2 are usually unbound in
  games, and a side button does not compete with movement keys.
- **Keep the real mouse still while the turn fires.** RearMarch sends a fixed
  delta; a simultaneous physical movement adds to it.
- **Use the ramp if the turn looks like a teleport.** A short ramp (20-60 ms)
  spreads the events and can look smoother, at the cost of a slightly slower
  turn.
- **Expect pointer acceleration on the desktop.** The count is exact, but
  Windows' "enhance pointer precision" changes how far the cursor travels. Games
  that read raw input are usually unaffected.
- **Do not use this to gain an unfair advantage in competitive multiplayer.**
  Beyond the ethics, anti-cheat will usually catch it and the account is lost.

## Known-good categories

Turn-based games, strategy, simulation, MMOs, single-player titles, and anything
that does not use kernel anti-cheat. Accessibility use cases - a player who
cannot comfortably perform a 180 degree flick - fall almost entirely in this
set.

## Reporting a compatibility result

Open an issue with the game name, whether the turn worked, whether the game uses
anti-cheat, and the `counts_per_360` and DPI values you settled on. Results are
collected in this file.
