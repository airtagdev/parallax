# Parallax — Minecraft 26.3 in C

The target is a faithful, independently implemented C rewrite of **Minecraft: Java Edition 26.3**: the same appearance, menus, controls, movement, simulation, world generation, world format, content and advancement behavior. Similar-looking replacement mechanics are not acceptable substitutes. See [the acceptance specification](docs/SPECIFICATION.md).

**The complete game is not implemented. The current executable is a title-rendering and compatibility-test build, not a playable Minecraft replacement.** Singleplayer and other unfinished screens report implementation gaps. They do not open the previous sandbox.

## What changed

The original 0.1 release was a custom voxel sandbox. It was the wrong implementation target. Its code and historical README are preserved under `experimental/sandbox/`, excluded from the default build. Its custom terrain, achievements, recipes, controls and `.pxw` saves are not considered Minecraft compatibility work.

The new default executable is built from `src/rewrite/`. It contains no copied Minecraft Java implementation, bytecode, native game library, or original Minecraft shader program. Windows/OpenGL platform APIs are used by the C application. Java appears only in optional, separate reference tools; the C executable does not start or embed a JVM.

## Current evidence

| Component | Implemented / tested | Limits |
| --- | --- | --- |
| Resource preparation | Version 26.3 pinned; JAR SHA-256 recorded; local asset-index/cache SHA-1 verified | Local preparation tool, not a complete runtime resource-pack system |
| Resource parsing | 2,042 recipe files; 1,866 advancement files; 126 displayed advancements; 4,045 criteria | Definitions are parsed, not implemented gameplay |
| Font rendering | Imported bitmap provider order, glyph widths, English localization and shadows | Unifont and complete text formatting remain missing |
| Title widgets | Seven full button regions match a native 1440x900 screenshot exactly, including text and language/accessibility icons | One resolution, GUI scale 3, English, default assets and non-hover state only |
| Legacy random stream | 4,096 Java reference sequences match C ints, bounded ints, longs, float bits and double bits | Other generators, Gaussian draws and world generation are unfinished |

The title screen still differs: player-appearance rendering, splash selection/formatting, fade, exact panorama movement/filtering, hover/focus narration, sounds and menu actions are not complete. Matching seven rectangles does not establish that the title screen, much less the game, is fully equivalent.

## Prepare and build

Requirements: Windows x64, CMake 3.20+, Visual Studio's native compiler/Windows SDK and PowerShell 7. The Visual Studio workload is called Desktop development with C++, but all application translation units are C17.

```powershell
.\scripts\build.ps1
```

This imports resources from your existing owned `.minecraft/versions/26.3/26.3.jar`, resolves real panorama textures and sounds from the local content-addressed cache, compiles C, then runs the core/resource checks. Nothing is downloaded. No password, account file, session token or browser cache is read. Existing Minecraft worlds are not modified.

For a different installation:

```powershell
.\scripts\import-assets.ps1 -MinecraftDirectory 'D:\Minecraft' -JarPath 'D:\Minecraft\versions\26.3\26.3.jar'
.\scripts\build.ps1 -SkipAssets
```

Output: `release/Parallax.exe`. Keep `release/resources/` beside it. Imported assets/data and the entire release directory stay local and are ignored by Git. Shader source, Java/native source and game bytecode are excluded from resource import.

## Verification

```powershell
ctest --test-dir build -C Release --output-on-failure
.\release\Parallax.exe --verify-resources
.\release\Parallax.exe --capture-title
```

The latter produces a deterministic C-rendered BMP for comparison. No synthetic world is generated. For live Java reference sequences and a locally captured native title frame:

```powershell
.\tools\test-parity.ps1 -JavaPath 'PATH\TO\JAVA25\java.exe' -ReferenceTitle 'PATH\TO\NATIVE\title.png'
```

`tools/launch-reference.ps1` can run the owned native 26.3 installation in an isolated, ignored `.local/reference-26.3/` game directory. It uses a dummy local offline session and does not read credentials. It is a development reference tool, never the implementation behind the C game. Capture the native title with F2. See [validation](docs/VALIDATION.md) for what the comparison actually covers.

## Source layout

- `src/rewrite/`: independently written C implementation.
- `tests/`: C JSON/compatibility and image-comparison tests.
- `scripts/`: local preparation/build tools.
- `tools/`: separate native/Java reference tools.
- `docs/SPECIFICATION.md`: required behavior and scope.
- `docs/PARITY.md`: explicit implementation/verification gaps.
- `experimental/sandbox/`: retired 0.1 experiment, not the rewrite.

Parallax is unofficial and is not associated with Mojang, Microsoft or Horizon. Minecraft assets and game data are not included in the public source tree.
