# Wakatime Integration for UE4
Simple Unreal Engine plugin for sending Wakatime plugins to an endpoint of choice. Currently only Windows builds, but should work on MacOS if you self-compile on a macbook.

![Screenshot of installed plugin](scrshot.png)

Features:
-
- Customiseable heartbeat intervals
- Sends last modified asset (Blueprints, Materials, Structs, etc)
- Added and removed blueprints pushed as `line_additions` and `line_deletions`
- Hopefully thread safe
- Might maybe work in UE5, haven't tested


Installation
-
- Go to the [latest release](https://github.com/ZXMushroom63/WakatimeIntegration/releases/latest), and download `WakatimeIntegration.zip`
- Extract the folder containing `WakatimeIntegration.uplugin`
- Move this folder to `Engine/Plugins/` in your Unreal Engine 4 installation folder
  - `C:\Program Files\Epic Games\UE_[Version]\Engine\Plugins\` for Windows users
- Enable the plugin in the plugins menu. You may need to do this for each project you wish to track.
- In editor settings, look for `Wakatime Integration`, and set your token and endpoint, as well as heartbeat interval. These settings are saved globally.

Building from source:
-
[instructions here](https://hackatime.hackclub.com/docs/editors/unreal-engine-4)