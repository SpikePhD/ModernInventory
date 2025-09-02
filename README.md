ModernInventory (SKSE + CommonLibSSE-NG)

Overview
- SKSE plugin that augments SkyUI’s inventory. Phase 1 detects Inventory open/close and the I key. Phase 2 overlays a Dear ImGui right panel and prepares an offscreen D3D11 target for player preview.

Build Requirements
- Visual Studio 2022, CMake 3.25+
- vcpkg (VS-integrated or standalone) with registries enabled

Configure & Build (Visual Studio)
- Open the folder (File > Open > Folder…).
- Select Configure Preset: “Ninja + vcpkg (x64)” or “VS2022 + vcpkg (x64)”.
- Build using the matching Build Preset (Debug/Release).

Auto-Deploy to Mod Organizer 2
- DLL is copied after build to: `C:/NVGO/mods/ModernInventory/SKSE/Plugins` (see CMakePresets.json `SKSE_DEPLOY_DIR`).
- Alternatively, set environment variable `SKYRIM_MODS_FOLDER` to your MO2 root; the DLL will deploy to `<mods>/<ProjectName>/SKSE/Plugins`.

Dependencies (vcpkg manifest)
- commonlibsse-ng, spdlog, imgui[dx11-binding,win32-binding], minhook.
- Registries configured in `vcpkg-configuration.json`.

Runtime Notes
- Inventory open shows an ImGui right panel; the vanilla item 3D preview is cleared.
- Offscreen D3D11 target renders as a placeholder image; next step is player model preview.

Config (optional)
- Create `ModernInventory.ini` next to the DLL to override defaults:
  - DebugToasts=1
  - PanelWidthRatio=0.75
  - PanelMinWidth=520

Troubleshooting
- If vcpkg fails, check `out/build/<preset>/vcpkg-manifest-install.log`.
- If you see LNK2038 or MDd mismatches, reconfigure using the supplied presets (project forces MD and disables debug iterators in Debug).

