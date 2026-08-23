# Installs the Space Calibrator overlay and driver from the local build output.
# Run this after `cmake --build bin --config Release` to deploy new builds.
#
# Usage:
#   .\install\Install-Dev.ps1              # installs both overlay and driver
#   .\install\Install-Dev.ps1 -OverlayOnly # installs only the overlay
#   .\install\Install-Dev.ps1 -DriverOnly  # installs only the driver
#
# Requires SteamVR to be running for manifest registration (-installmanifest).

[CmdletBinding()]
param(
    [switch]$OverlayOnly,
    [switch]$DriverOnly,
    [string]$SteamVRRuntime = "E:\Steam\steamapps\common\SteamVR"
)

$ErrorActionPreference = "Stop"

$RepoRoot   = Split-Path -Parent $PSScriptRoot
$Artifacts  = Join-Path $RepoRoot "bin\artifacts\Release"
$DriverSrc  = Join-Path $RepoRoot "bin\driver_01spacecalibrator"
$OverlayDst = Join-Path $env:LOCALAPPDATA "SpaceCalibrator"
$DriverDst  = Join-Path $SteamVRRuntime "drivers\driver_01spacecalibrator"

$OverlayFiles = @(
    "SpaceCalibrator.exe",
    "openvr_api.dll",
    "manifest.vrmanifest",
    "icon.png",
    "taskbar_icon.png",
    "LICENSE",
    "README"
)

function Assert-FileExists($Path, $Description) {
    if (-not (Test-Path $Path)) {
        Write-Error "$Description not found: $Path`nDid you run 'cmake --build bin --config Release' first?"
    }
}

function Install-Overlay {
    Write-Host "Installing overlay to $OverlayDst" -ForegroundColor Cyan

    foreach ($f in $OverlayFiles) {
        $src = Join-Path $Artifacts $f
        Assert-FileExists $src "Overlay artifact ($f)"
    }

    New-Item -ItemType Directory -Path $OverlayDst -Force | Out-Null
    foreach ($f in $OverlayFiles) {
        Copy-Item (Join-Path $Artifacts $f) (Join-Path $OverlayDst $f) -Force
    }

    # Re-register the manifest so SteamVR picks up the new path.
    # -installmanifest removes the old manifest path and adds the current one,
    # so this is safe to run repeatedly.
    $exe = Join-Path $OverlayDst "SpaceCalibrator.exe"
    Write-Host "Registering manifest with SteamVR..." -ForegroundColor Cyan
    Push-Location $OverlayDst
    try {
        $proc = Start-Process -FilePath $exe -ArgumentList "-installmanifest" -Wait -NoNewWindow -PassThru
        if ($proc.ExitCode -ne 0 -and $proc.ExitCode -ne -2) {
            Write-Warning "SpaceCalibrator.exe -installmanifest exited with code $($proc.ExitCode). Is SteamVR running?"
        }
    } finally {
        Pop-Location
    }

    Write-Host "Overlay installed." -ForegroundColor Green
}

function Install-Driver {
    Write-Host "Installing driver to $DriverDst" -ForegroundColor Cyan

    $driverFiles = @(
        @{ Src = Join-Path $DriverSrc "driver.vrdrivermanifest";                Dst = $DriverDst },
        @{ Src = Join-Path $DriverSrc "resources\driver.vrresources";           Dst = Join-Path $DriverDst "resources" },
        @{ Src = Join-Path $DriverSrc "resources\settings\default.vrsettings";  Dst = Join-Path $DriverDst "resources\settings" },
        @{ Src = Join-Path $DriverSrc "bin\win64\driver_01spacecalibrator.dll"; Dst = Join-Path $DriverDst "bin\win64" }
    )

    foreach ($f in $driverFiles) {
        Assert-FileExists $f.Src "Driver artifact"
    }

    foreach ($f in $driverFiles) {
        New-Item -ItemType Directory -Path $f.Dst -Force | Out-Null
        Copy-Item $f.Src $f.Dst -Force
    }

    # Register the external driver path with SteamVR if not already present.
    $vrpathreg = Join-Path $SteamVRRuntime "bin\win64\vrpathreg.exe"
    if (Test-Path $vrpathreg) {
        Write-Host "Ensuring driver is registered with vrpathreg..." -ForegroundColor Cyan
        & $vrpathreg finddriver 01spacecalibrator 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 1) {
            & $vrpathreg adddriver "$DriverDst\" 2>&1 | Out-Null
            Write-Host "Driver registered with vrpathreg." -ForegroundColor Green
        } else {
            Write-Host "Driver already registered with vrpathreg." -ForegroundColor DarkGray
        }
    } else {
        Write-Warning "vrpathreg.exe not found at $vrpathreg - skipping driver path registration."
    }

    Write-Host "Driver installed." -ForegroundColor Green
}

# --- Main ---

if (-not $OverlayOnly -and -not $DriverOnly) {
    $OverlayOnly = $true
    $DriverOnly  = $true
}

if ($OverlayOnly) { Install-Overlay }
if ($DriverOnly)  { Install-Driver }

Write-Host "`nDone. Restart SteamVR for changes to take effect." -ForegroundColor Yellow
