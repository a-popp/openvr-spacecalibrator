<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://github.com/a-popp/openvr-spacecalibrator/blob/main/.github/logo_light.png?raw=true">
  <source media="(prefers-color-scheme: light)" srcset="https://github.com/a-popp/openvr-spacecalibrator/blob/main/.github/logo_dark.png?raw=true">
  <img alt="Space Calibrator" src="https://github.com/a-popp/openvr-spacecalibrator/blob/main/.github/logo.png?raw=true">
</picture>

This is a modern, maintained fork of [OpenVR Space Calibrator](https://github.com/pushrax/OpenVR-SpaceCalibrator), updated to work with the latest SteamVR and OpenVR SDK releases.

This fork is a standalone build and does not require nor interact with the [Steam store version of Space Calibrator](https://github.com/hyblocker/OpenVR-SpaceCalibrator) created by Hyblocker. It uses its own OpenVR app key and is fully independent.

This program is designed to allow you to synchronise multiple playspaces with one another in SteamVR. It also includes [continuous calibration](#continuous-calibration), an experimental tracking mode which automatically aligns playspaces together using a tracker on the headset.

## Installing

### Manual Installation

1. Make sure SteamVR or any other VR utilities are **not running**.

2. Download the latest `SpaceCalibrator-Overlay` and `SpaceCalibrator-Driver` artifacts from the [Releases](https://github.com/a-popp/openvr-spacecalibrator/releases) page, or build from source (see below).

3. Create a folder for the overlay (e.g. `%LOCALAPPDATA%\SpaceCalibrator`) and copy these files into it:
   - `SpaceCalibrator.exe`
   - `openvr_api.dll`
   - `manifest.vrmanifest`
   - `icon.png`
   - `taskbar_icon.png`

4. Copy the driver folder into your SteamVR drivers folder:
   ```powershell
   Copy-Item -Recurse -Force "driver_01spacecalibrator" "%STEAM_ROOT%\steamapps\common\SteamVR\drivers\driver_01spacecalibrator"
   ```

5. Start SteamVR.

6. Launch `SpaceCalibrator.exe` once. It will auto-register its manifest with SteamVR and auto-start from then on.

> [!NOTE]
> You do not need to add it to Steam. Running the `.exe` directly or letting SteamVR auto-start it is sufficient.

### Building from Source

Requirements:
- Windows 10/11
- [CMake](https://cmake.org/download/) 3.10+
- Visual Studio 2022 (or Build Tools) with the C++ workload

```powershell
git clone --recursive https://github.com/a-popp/openvr-spacecalibrator.git
cd openvr-spacecalibrator
cmake -G "Visual Studio 17 2022" -A x64 -B bin -S .
cmake --build bin --config Release
```

## Regular Calibration

If you do not wish to use continuous calibration, you will have to use regular calibration. This means that every so often you will have to sync your headset's playspace with your tracker's playspace.

To calibrate:
1. Start Steam VR, and put on your VR Headset (HMD).
2. Open the SteamVR dashboard using the home button (or equivalent) on your controller. At the bottom, click on the Space Calibrator icon.
3. In the Space Calibrator Overlay, click `Copy Chaperone Bounds to Profile` followed by `Paste Chaperone Bounds`.
4. Next, you'll see two lists at the top. On the left `Reference Space` column, select the controller you'll be calibrating along (e.g. Quest controller, Pico controller). On the right `Target Space`, select your SteamVR tracker (e.g. Vive Ultimate Tracker, Vive Tracker 3.0). You can use the Identify button to make the controllers blink and tracker LEDs flash to see if you've selected the correct ones.
5. After you have confirmed your selection, physically place the `Target Space` tracker on top of the `Reference Space` controller.
6. With the two device physically on top of each other, click the "Start calibration" button. As the calibration proceeds, move the device pair together as a unit around your play space. It's a good idea to include different kinds of motion, direction, and rotation as you do this to ensure a more accurate calibration.
7. After calibration completes, you should see the silluhette of the your reference controller and target tracker occupy the same space in your HMD, which should match the physical position of the devices.

## Continuous Calibration

> [!NOTE]
> I don't use this particular feature, but I included it from Hyblocker's fork because it may be useful to some users. However, I cannot assist with troubleshooting it, unfortunately.

> [!IMPORTANT]
> **A tracker attached on your headset is required for this.**

To enable continuous calibration mode, first select your headset on the left column, then the tracker on your headset on the right column. Once you've done so, click `Start Calibration`, and click cancel. Then click `Continuous Calibration` to enable continuous calibration.

1. Start SteamVR with the VR headset you wish to use.
2. Turn on **ONLY** the tracker which is attached on the VR headset.
3. Select the VR headset and tracker and calibrate.
4. Turn on your other devices.
5. You should see them line up with you as you after moving around your playspace for a bit for an initial calibration.