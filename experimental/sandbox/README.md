# Retired experiment — not the Minecraft rewrite

This is historical documentation for the superseded 0.1 sandbox. Its features do not count toward native 26.3 parity, and its executable path below no longer identifies the default build.

# Parallax

A playable, Windows-first voxel sandbox implemented in C17. The game and world-selection menu are one native executable. This is version 0.1, an early prototype toward a broader Minecraft-style reimplementation; it is not a complete Minecraft port or a Horizon fork.

## Play

Open `release/Parallax.exe`. Choose **Singleplayer worlds**, an empty slot, a name and seed, then **Generate & play**. Survival starts with 16 planks and 8 decorative lanterns; Creative provides unlimited blocks. Flight can be toggled in either mode while the prototype is being developed.

Keep the executable in a writable folder. Worlds are saved beside it in `release/saves/world-N.pxw`, automatically every 60 seconds and on normal exit. There are five independent slots. Back up the `saves` folder before updating; this early save format may change.

The local build can import block textures from your owned installation of Java Edition 26.3. No password, account token, launcher cache, Java game code, or Minecraft assets are included in this repository. Without imported textures the executable uses procedural fallback textures. The game makes no network connections and does not change your `.minecraft` directory.

## Implemented in 0.1

- Seeded 256 x 256 x 96 worlds, with hills, grassy/sandy/snowy terrain, caves, trees, water, coal, iron and diamond ore.
- First-person mouse look, walking, jumping, sprinting, swimming, collision and optional flight.
- Mining, block placement, block targeting, inventory, nine-slot hotbar and four simple crafting recipes.
- Forty basic mobs: sheep, cows, pigs and zombies. Nearby animals wander; zombies approach and damage the player at night. Defeating a mob restores food. Mobs have simple colored block models.
- Health, hunger, respawn with inventory retained, a day/night cycle, fog and face shading.
- Twenty-four working Parallax achievements, with notifications and saved progress.
- Named world slots, survival/creative modes, pause screen, adjustable view distance, screenshots and synthesized interaction sounds.
- Checksummed, versioned saves; writes use a temporary file followed by replacement. Invalid saves are rejected before replacing the active world.

## Controls

| Key | Action |
| --- | --- |
| WASD / mouse | Move / look |
| Space | Jump, swim up, or fly up |
| Left Ctrl | Sprint / faster flight |
| Left Shift | Slow walking / fly down |
| Left / right mouse | Mine or attack / place block; hold to repeat |
| 1-9 / wheel | Select a hotbar block |
| E | Inventory and crafting; click a block to select it |
| L | Achievement list |
| F | Toggle flight |
| F3 | Position, seed and world information |
| F5 | Save now |
| F2 | Save a BMP screenshot in the working directory |
| Escape | Pause / return |

## Build

Requirements: Windows x64, Visual Studio with Desktop development with C++, the Windows SDK, CMake 3.20+, and PowerShell. Despite the Visual Studio workload name, all application translation units are C, compiled as C17. The release links the MSVC runtime statically and uses Windows/OpenGL system APIs; there is no Rust, C++, JVM, browser engine, or third-party game engine in the application.

```powershell
.\scripts\build.ps1
```

If your Minecraft directory or JAR is elsewhere:

```powershell
.\scripts\build.ps1 -SkipAssets
.\scripts\import-assets.ps1 -JarPath 'D:\Games\Minecraft\26.3.jar'
```

Asset import extracts only a fixed allowlist of 18 PNG block textures, to the ignored `release/assets` directory. Keep those extracted files private.

## Verification

```powershell
ctest --test-dir build -C Release --output-on-failure
.\release\Parallax.exe --self-test
.\release\Parallax.exe --smoke-test
```

The world integrity suite checks deterministic generation, different seeds, safe spawning, movement, walls, ground collision, flight height, ingredient accounting, block targeting, mining, placement, nearby mob movement, save/load identity and corrupted-save rejection. The renderer smoke run exercises menu buttons and keyboard handlers through world creation, inventory, achievements, pause, and save/return; it captures five views, then removes its isolated test save. It never uses a player's save slot. Run smoke tests from a writable working directory. These checks are engineering smoke tests, not evidence of vanilla compatibility or a substitute for playtesting.

## Current limits

This alpha uses its own finite world generation, `.pxw` saves, rules and achievements. It cannot load or export vanilla worlds, join servers, run Java mods, generate infinite terrain, or reproduce Minecraft's complete advancement tree. There is no Nether/End, redstone, fluid simulation, tools/durability, full recipe catalog, furnaces, containers, farming, breeding, fall damage, dynamic block lighting or vanilla sound engine yet. Lanterns are decorative. Mobs use basic steering instead of full pathfinding. Terrain meshes rebuild on the main thread under a per-frame budget, so editing large areas or generating visible chunks can cause stutter. Only Windows x64 has been built and tested.

## Source layout

- `src/main.c`: window, input, menus, HUD, application lifecycle and render smoke test.
- `src/world.c`: terrain, physics, mobs, interaction, crafting, achievements, saves and integrity tests.
- `src/render.c`: WGL/OpenGL renderer, chunk meshes, WIC texture decoding and text drawing.
- `scripts/`: local asset preparation and build commands.
- `docs/ROADMAP.md`: next implementation milestones.

Parallax is unofficial and is not affiliated with Mojang, Microsoft or Horizon. Minecraft code and assets are not licensed by this project.
