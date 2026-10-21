#requires -Version 5.1
<#
.SYNOPSIS
    Synthetic UAT for RearMarch: launch with a temp config, press the trigger,
    measure the injected deltas, toggle, and clean up.

.DESCRIPTION
    Runs against a real build. Nothing in the operator's own
    %LOCALAPPDATA%\RearMarch is touched because --config points elsewhere.

    A missing config is created with the trigger on F7, the application
    enabled, and the startup entry off, so the run is reproducible and cannot
    write to the operator's Run key.

    The trigger is set to F7 in the temp config. The test moves the cursor to
    the centre of the primary screen, records the position, injects F7 with
    keybd_event, waits, and compares. Pointer acceleration makes the cursor
    distance differ from the raw count total, so the assertion is on direction
    and on a generous magnitude range; the exact counts are covered by the unit
    tests.
#>
[CmdletBinding()]
param(
    [string]$Exe,
    [string]$Config
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# $PSScriptRoot is not available in a param default, so resolve the build
# output here.
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Exe) { $Exe = Join-Path $repoRoot 'build\Release\RearMarch.exe' }
if (-not $Config) { $Config = Join-Path $env:TEMP 'rearmarch-smoke\config.json' }

Add-Type -AssemblyName UIAutomationTypes

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Native {
    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, int dx, int dy, uint data, UIntPtr extra);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X; public int Y; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

# Physical pixels everywhere: window rectangles, cursor positions, and clicks
# must agree or the calibration click lands somewhere else on a scaled display.
[Native]::SetProcessDpiAwarenessContext([IntPtr](-4)) | Out-Null

function Start-RearMarch {
    $process = Start-Process -FilePath $Exe -ArgumentList "--config", "`"$Config`"" -PassThru
    Start-Sleep -Milliseconds 1500
    return $process
}

function Stop-RearMarch {
    param($Process)
    if ($Process -and -not $Process.HasExited) {
        Stop-Process -Id $Process.Id -Force
        Start-Sleep -Milliseconds 500
    }
}

$failures = @()
function Assert-That {
    param([bool]$Condition, [string]$Message)
    if ($Condition) {
        Write-Host "  PASS  $Message"
    } else {
        Write-Host "  FAIL  $Message" -ForegroundColor Red
        $script:failures += $Message
    }
}

function Read-Config {
    return (Get-Content $Config -Raw | ConvertFrom-Json)
}

function Invoke-Click {
    param([int]$X, [int]$Y)
    [Native]::SetCursorPos($X, $Y) | Out-Null
    Start-Sleep -Milliseconds 150
    [Native]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 60
    [Native]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Send-Chord {
    param([byte[]]$Keys)
    foreach ($key in $Keys) {
        [Native]::keybd_event($key, 0, 0, [UIntPtr]::Zero)
    }
    Start-Sleep -Milliseconds 60
    for ($i = $Keys.Count - 1; $i -ge 0; $i--) {
        [Native]::keybd_event($Keys[$i], 0, 2, [UIntPtr]::Zero)
    }
}

# Screen point at a fraction of the ImGui client area. The dashboard is a fixed
# layout that scales with DPI, so fractions stay valid while pixel coordinates
# would not. Calibrate sits at (0.145, 0.546) in the default English layout.
function Get-ClientPoint {
    param([IntPtr]$Hwnd, [double]$FractionX, [double]$FractionY)
    $client = New-Object Native+RECT
    [Native]::GetClientRect($Hwnd, [ref]$client) | Out-Null
    $origin = New-Object Native+POINT
    $origin.X = 0
    $origin.Y = 0
    [Native]::ClientToScreen($Hwnd, [ref]$origin) | Out-Null
    return @(
        [int]($origin.X + $client.Right * $FractionX),
        [int]($origin.Y + $client.Bottom * $FractionY)
    )
}

Write-Host "RearMarch synthetic UAT"
Write-Host "  exe:    $Exe"
Write-Host "  config: $Config"
Write-Host ""

if (-not (Test-Path $Exe)) { throw "The executable does not exist: $Exe" }

if (-not (Test-Path $Config)) {
    # A fresh config makes the test reproducible: the trigger has to be F7 and
    # the startup entry must be off, or the run would depend on (and modify)
    # the operator's own settings.
    $configDir = Split-Path -Parent $Config
    if ($configDir) { New-Item -ItemType Directory -Force $configDir | Out-Null }
    $testConfig = [ordered]@{
        schema  = 1
        enabled = $true
        general = [ordered]@{
            locale             = 'en'
            start_minimized    = $false
            start_with_windows = $false
        }
        turn    = [ordered]@{
            trigger        = 'F7'
            angle_deg      = 180.0
            direction      = 'right'
            dpi            = 1000
            counts_per_360 = 6000.0
            ramp_ms        = 0
        }
        feedback = [ordered]@{ beep = $true }
    }
    $testConfig | ConvertTo-Json -Depth 5 | Set-Content -Path $Config -Encoding UTF8
    Write-Host "  created a fresh config: $Config"
}

$process = Start-RearMarch
$hwnd = [IntPtr]::Zero
$wasTopmost = $false
try {
    # The dashboard is expected on a first run with a fresh config.
    $hwnd = [Native]::FindWindow([NullString]::Value, "RearMarch")
    Assert-That ($hwnd -ne [IntPtr]::Zero) "the dashboard window exists"
    Assert-That ([Native]::IsWindowVisible($hwnd)) "the dashboard is visible"

    # --- Trigger ---------------------------------------------------------
    $screenWidth = [Native]::GetSystemMetrics(0)
    $screenHeight = [Native]::GetSystemMetrics(1)
    [Native]::SetCursorPos([int]($screenWidth / 2), [int]($screenHeight / 2)) | Out-Null
    Start-Sleep -Milliseconds 100
    $before = New-Object Native+POINT
    [Native]::GetCursorPos([ref]$before) | Out-Null

    # F7 = 0x76. Press and release; the hook is an observer, so this reaches
    # the application whether or not the game is focused.
    Send-Chord @(0x76)
    Start-Sleep -Milliseconds 300

    $after = New-Object Native+POINT
    [Native]::GetCursorPos([ref]$after) | Out-Null
    $dx = $after.X - $before.X
    Write-Host "  cursor dx after one turn: $dx"
    Assert-That ($dx -gt 50) "the turn moved the cursor to the right"

    # --- Toggle ----------------------------------------------------------
    # Ctrl (0x11) + Alt (0x12) + M (0x4D)
    Send-Chord @(0x11, 0x12, 0x4D)
    Start-Sleep -Milliseconds 500

    [Native]::SetCursorPos([int]($screenWidth / 2), [int]($screenHeight / 2)) | Out-Null
    Start-Sleep -Milliseconds 100
    [Native]::GetCursorPos([ref]$before) | Out-Null
    Send-Chord @(0x76)
    Start-Sleep -Milliseconds 300
    [Native]::GetCursorPos([ref]$after) | Out-Null
    $dx_disabled = $after.X - $before.X
    Write-Host "  cursor dx while disabled: $dx_disabled"
    Assert-That ([Math]::Abs($dx_disabled) -lt 10) "a disabled RearMarch does not turn"

    # --- Persistence -----------------------------------------------------
    $saved = Read-Config
    Assert-That (-not $saved.enabled) "the toggle was written to the config file"

    # --- Calibration -----------------------------------------------------
    # Re-enable, then click the real Calibrate button. The dashboard is behind
    # whatever the operator had in front, so raise it and activate it with a
    # title-bar click before clicking the button. Everything is restored in the
    # finally block.
    Send-Chord @(0x11, 0x12, 0x4D)
    Start-Sleep -Milliseconds 500

    $beforeCalibration = (Read-Config).turn.counts_per_360
    [Native]::SetWindowPos($hwnd, [IntPtr](-1), 0, 0, 0, 0, 0x0003) | Out-Null
    $wasTopmost = $true
    Start-Sleep -Milliseconds 300
    [Native]::SetForegroundWindow($hwnd) | Out-Null
    $windowRect = New-Object Native+RECT
    [Native]::GetWindowRect($hwnd, [ref]$windowRect) | Out-Null
    for ($attempt = 0; $attempt -lt 3; $attempt++) {
        Invoke-Click ([int](($windowRect.Left + $windowRect.Right) / 2)) `
                     ([int]($windowRect.Top + 12))
        Start-Sleep -Milliseconds 300
        if ([Native]::GetForegroundWindow() -eq $hwnd) { break }
    }
    # Activation is best effort: Windows can refuse a foreground change while
    # the console still owns it, yet the click below is delivered anyway. The
    # trim assertion is the real proof that calibration mode was entered.
    if ([Native]::GetForegroundWindow() -eq $hwnd) {
        Write-Host "  dashboard activated"
    } else {
        Write-Host "  note: the dashboard did not report foreground; clicking anyway"
    }

    $calibrate = Get-ClientPoint $hwnd 0.145 0.546
    Invoke-Click $calibrate[0] $calibrate[1]
    Start-Sleep -Milliseconds 700

    # Ctrl + Alt + Up trims the live count up by the configured 2%.
    Send-Chord @(0x11, 0x12, 0x26)
    Start-Sleep -Milliseconds 700
    $duringCalibration = (Read-Config).turn.counts_per_360
    Write-Host "  counts_per_360 $beforeCalibration -> $duringCalibration"
    Assert-That ($duringCalibration -gt $beforeCalibration) `
        "a calibration trim raised counts_per_360"

    # Ctrl + Alt + Enter saves and leaves calibration mode.
    Send-Chord @(0x11, 0x12, 0x0D)
    Start-Sleep -Milliseconds 700
    $afterSave = (Read-Config).turn.counts_per_360
    Assert-That ([Math]::Abs($afterSave - $duringCalibration) -lt 0.01) `
        "the save chord persisted the calibrated value"

    # --- Recorder suppression -------------------------------------------
    # With the trigger recorder open, pressing the current trigger must not
    # turn: the runtime snapshot has to carry capture_active to the hook.
    $triggerButton = Get-ClientPoint $hwnd 0.368 0.127
    Invoke-Click $triggerButton[0] $triggerButton[1]
    Start-Sleep -Milliseconds 700
    # The click itself moved the pointer to the button; measure from the centre
    # again so only the injected turn can change the position.
    [Native]::SetCursorPos([int]($screenWidth / 2), [int]($screenHeight / 2)) | Out-Null
    Start-Sleep -Milliseconds 150
    [Native]::GetCursorPos([ref]$before) | Out-Null
    Send-Chord @(0x76)
    Start-Sleep -Milliseconds 400
    [Native]::GetCursorPos([ref]$after) | Out-Null
    $dxRecording = $after.X - $before.X
    Write-Host "  cursor dx while recording a trigger: $dxRecording"
    Assert-That ([Math]::Abs($dxRecording) -lt 10) `
        "an open recorder suppresses the trigger"
    Send-Chord @(0x1B)  # Escape closes the recorder
    Start-Sleep -Milliseconds 300
} finally {
    if ($wasTopmost -and $hwnd -ne [IntPtr]::Zero) {
        [Native]::SetWindowPos($hwnd, [IntPtr](-2), 0, 0, 0, 0, 0x0003) | Out-Null
    }
    Stop-RearMarch $process
}

Write-Host ""
if ($failures.Count -gt 0) {
    Write-Host "$($failures.Count) check(s) failed" -ForegroundColor Red
    exit 1
}
Write-Host "all checks passed"
