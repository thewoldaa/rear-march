#requires -Version 5.1
<#
.SYNOPSIS
    Captures the RearMarch dashboard with PrintWindow for a visual check.

.DESCRIPTION
    CopyFromScreen returns a white client area for a D3D11 window on some
    driver/MPO combinations; PrintWindow with PW_RENDERFULLCONTENT captures the
    rendered frame reliably. The window is raised to the top first so nothing
    overlaps it at the moment of capture.
#>
[CmdletBinding()]
param(
    [string]$Out,
    [string]$Exe,
    [string]$Config
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Exe) { $Exe = Join-Path $repoRoot 'build\Release\RearMarch.exe' }
if (-not $Config) { $Config = Join-Path $env:TEMP 'rearmarch-smoke\config.json' }
if (-not $Out) { $Out = Join-Path $env:TEMP 'rearmarch-smoke\dashboard.png' }

Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Capture {
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$process = Start-Process -FilePath $Exe -ArgumentList "--config", "`"$Config`"" -PassThru
Start-Sleep -Milliseconds 1800

try {
    $hwnd = [Capture]::FindWindow([NullString]::Value, "RearMarch")
    if ($hwnd -eq [IntPtr]::Zero) { throw "The dashboard window was not found" }

    $topmost = [IntPtr](-1)
    $flags = 0x43  # SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE
    [Capture]::SetWindowPos($hwnd, $topmost, 0, 0, 0, 0, $flags) | Out-Null
    Start-Sleep -Milliseconds 600

    $rect = New-Object Capture+RECT
    [Capture]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top

    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $hdc = $graphics.GetHdc()
    $ok = [Capture]::PrintWindow($hwnd, $hdc, 2)  # PW_RENDERFULLCONTENT
    $graphics.ReleaseHdc($hdc)
    $graphics.Dispose()

    if (-not $ok) { throw "PrintWindow failed" }

    $bitmap.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    Write-Host "Wrote $Out ($width x $height)"
} finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        Start-Sleep -Milliseconds 400
    }
}
