#requires -Version 5.1
<#
.SYNOPSIS
    Generates resources/rearmarch.ico.

.DESCRIPTION
    The application icon is generated rather than committed as an opaque binary,
    so a palette change can be applied by editing one file and re-running this
    script. Windows PowerShell only: it uses System.Drawing, which ships with
    the .NET Framework and needs no extra downloads.

    The mark is a circular arrow: the "turn around" of a snap turn.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools/make_icon.ps1
#>
[CmdletBinding()]
param(
    [string]$Output
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Output) {
    $Output = Join-Path $repoRoot 'resources\rearmarch.ico'
}

# Palette, kept in step with src/ui/theme.cpp.
$Background = [System.Drawing.Color]::FromArgb(255, 0x15, 0x18, 0x1D)
$Accent = [System.Drawing.Color]::FromArgb(255, 0x4C, 0xC2, 0xA6)

function New-IconBitmap {
    param([int]$Size)

    $bitmap = New-Object System.Drawing.Bitmap(
        $Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.Clear([System.Drawing.Color]::Transparent)

    $s = [double]$Size

    # Rounded background.
    $backgroundPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $radius = $s * 0.22
    $diameter = $radius * 2
    $x = $s * 0.02
    $y = $s * 0.02
    $width = $s * 0.96
    $height = $s * 0.96
    $backgroundPath.AddArc($x, $y, $diameter, $diameter, 180, 90)
    $backgroundPath.AddArc($x + $width - $diameter, $y, $diameter, $diameter, 270, 90)
    $backgroundPath.AddArc($x + $width - $diameter, $y + $height - $diameter, $diameter, $diameter, 0, 90)
    $backgroundPath.AddArc($x, $y + $height - $diameter, $diameter, $diameter, 90, 90)
    $backgroundPath.CloseFigure()
    $backgroundBrush = New-Object System.Drawing.SolidBrush($Background)
    $graphics.FillPath($backgroundBrush, $backgroundPath)

    # Arc: a three-quarter circle, thick enough to read at 16 px.
    $margin = $s * 0.22
    $arcSize = $s - ($margin * 2)
    $penWidth = [Math]::Max(1.5, $s * 0.11)
    $pen = New-Object System.Drawing.Pen($Accent, $penWidth)
    $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $graphics.DrawArc($pen, $margin, $margin, $arcSize, $arcSize, -55, 280)

    # Arrow head at the open end, pointing along the arc.
    $head = $s * 0.15
    $cx = $s / 2.0
    $cy = $s / 2.0
    $rx = $arcSize / 2.0
    $angle = (-55 + 280) * [Math]::PI / 180.0
    $tipX = $cx + ($rx * [Math]::Cos($angle))
    $tipY = $cy + ($rx * [Math]::Sin($angle))
    $point = New-Object System.Drawing.PointF($tipX, $tipY)
    $left = New-Object System.Drawing.PointF(
        ($tipX - $head * [Math]::Cos($angle - 0.6)),
        ($tipY - $head * [Math]::Sin($angle - 0.6)))
    $right = New-Object System.Drawing.PointF(
        ($tipX - $head * [Math]::Cos($angle + 0.6)),
        ($tipY - $head * [Math]::Sin($angle + 0.6)))
    $triangle = @($point, $left, $right)
    $headBrush = New-Object System.Drawing.SolidBrush($Accent)
    $graphics.FillPolygon($headBrush, $triangle)

    $headBrush.Dispose()
    $pen.Dispose()
    $backgroundBrush.Dispose()
    $backgroundPath.Dispose()
    $graphics.Dispose()

    return $bitmap
}

function Get-PngBytes {
    param([System.Drawing.Bitmap]$Bitmap)

    $stream = New-Object System.IO.MemoryStream
    $Bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $stream.ToArray()
    $stream.Dispose()
    # The leading comma keeps the array intact: returning it bare lets
    # PowerShell unroll it to Object[], which BinaryWriter then coerces to a
    # single byte instead of writing the whole frame.
    return ,$bytes
}

function Get-DibBytes {
    param([System.Drawing.Bitmap]$Bitmap)

    $width = $Bitmap.Width
    $height = $Bitmap.Height
    $stream = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter($stream)

    # BITMAPINFOHEADER. biHeight is doubled because the DIB holds the colour
    # bitmap followed by the (unused) AND mask.
    $writer.Write([UInt32]40)
    $writer.Write([Int32]$width)
    $writer.Write([Int32]($height * 2))
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]0)
    $writer.Write([UInt32]($width * $height * 4))
    $writer.Write([Int32]0)
    $writer.Write([Int32]0)
    $writer.Write([UInt32]0)
    $writer.Write([UInt32]0)

    for ($y = $height - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $width; $x++) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $writer.Write([byte]$pixel.B)
            $writer.Write([byte]$pixel.G)
            $writer.Write([byte]$pixel.R)
            $writer.Write([byte]$pixel.A)
        }
    }

    $maskRowBytes = [Math]::Floor(($width + 31) / 32) * 4
    $writer.Write((New-Object byte[] ($maskRowBytes * $height)))
    $writer.Flush()

    $bytes = $stream.ToArray()
    $writer.Dispose()
    $stream.Dispose()
    # See Get-PngBytes: the comma preserves the array type through the return.
    return ,$bytes
}

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$frames = @()

foreach ($size in $sizes) {
    $bitmap = New-IconBitmap $size
    [byte[]]$data = if ($size -ge 128) { Get-PngBytes $bitmap } else { Get-DibBytes $bitmap }
    $frames += [pscustomobject]@{ Size = $size; Data = $data }
    $bitmap.Dispose()
}

$directory = Split-Path -Parent $Output
if (-not (Test-Path $directory)) {
    New-Item -ItemType Directory -Path $directory -Force | Out-Null
}

$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($stream)

# ICONDIR
$writer.Write([UInt16]0)
$writer.Write([UInt16]1)
$writer.Write([UInt16]$frames.Count)

# ICONDIRENTRY for each frame.
$offset = 6 + (16 * $frames.Count)
foreach ($frame in $frames) {
    $dimension = if ($frame.Size -ge 256) { 0 } else { $frame.Size }
    $writer.Write([byte]$dimension)
    $writer.Write([byte]$dimension)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]$frame.Data.Length)
    $writer.Write([UInt32]$offset)
    $offset += $frame.Data.Length
}

foreach ($frame in $frames) {
    $writer.Write($frame.Data)
}

$writer.Flush()
[System.IO.File]::WriteAllBytes($Output, $stream.ToArray())
$writer.Dispose()
$stream.Dispose()

Write-Host ("Wrote {0} ({1} bytes, sizes: {2})" -f `
    $Output, (Get-Item $Output).Length, ($sizes -join ', '))
