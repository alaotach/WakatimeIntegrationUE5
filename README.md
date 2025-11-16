# Wakatime Integration for UE5

[![Unreal Engine 5](https://img.shields.io/badge/Unreal%20Engine-5.7-blue)](https://www.unrealengine.com/)
[![License](https://img.shields.io/github/license/alaotach/WakatimeIntegrationUE5)](LICENSE)
[![GitHub release](https://img.shields.io/github/v/release/alaotach/WakatimeIntegrationUE5)](https://github.com/alaotach/WakatimeIntegrationUE5/releases)

> **Keywords**: Unreal Engine 5 Plugin, UE5 Wakatime, Wakatime Integration, Time Tracking Plugin, Unreal Engine Time Tracker, UE5 Editor Plugin, Game Development Analytics, Blueprint Tracking, Asset Tracking, Developer Productivity

Simple Unreal Engine 5 plugin for sending Wakatime heartbeats to an endpoint of choice. Currently only Windows builds, but should work on MacOS if you self-compile on a macbook.

## Credits
Originally created by [ZXMushroom63](https://github.com/ZXMushroom63) for Unreal Engine 4.  
Updated and maintained for Unreal Engine 5 by [alaotach](https://github.com/alaotach).

![Screenshot of installed plugin](scrshot.png)

Features:
-
- Customiseable heartbeat intervals
- Sends last modified asset (Blueprints, Materials, Structs, etc)
- Added and removed blueprints pushed as `line_additions` and `line_deletions`
- Hopefully thread safe
- Updated for Unreal Engine 5.7


Installation
-
- Go to the [latest release](https://github.com/alaotach/WakatimeIntegrationUE5/releases/latest), and download `WakatimeIntegration.zip`
- Extract the folder containing `WakatimeIntegration.uplugin`
- Move this folder to `Engine/Plugins/` in your Unreal Engine 5 installation folder
  - `C:\Program Files\Epic Games\UE_[Version]\Engine\Plugins\` for Windows users
- Enable the plugin in the plugins menu. You may need to do this for each project you wish to track.
- In editor settings, look for `Wakatime Integration`, and set your token and endpoint, as well as heartbeat interval. These settings are saved globally.

Building from source:
-
[instructions here](https://hackatime.hackclub.com/docs/editors/unreal-engine-4)