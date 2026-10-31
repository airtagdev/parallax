# Horizon to C source port

The user's requirement is the same Minecraft 26.3 game in C, not a voxel sandbox. Horizon is now the source-port baseline. MIT copyright and license terms are preserved in `third_party/horizon/LICENSE`.

## Pinned baseline

Repository: https://github.com/horizon-rust/horizon

Revision: `0d327cc09307a90dce273d5753e1d88bf0f2f461`.

The recursive Git tree at this revision contains 1,892 Rust files and 34,636,202 bytes of Rust source, across the engine, game, launcher, tools and tests. This is source inventory, not a count of modules already converted. The full Git clone was blocked by the execution environment's DNS/network restrictions. Three reference modules were retrieved through the GitHub connector at the pinned commit; their original Git blob hashes are recorded and checked in `horizon.lock.json`. A complete clone has not been downloaded.

## First C translation

`src/rewrite/horizon_input.c` translates the mouse-input primitives from `horizon-client-input/src/mouse.rs` and keyboard direction/input-speed arithmetic from `input.rs`:

- Sensitivity curve with Java float constants widened to doubles.
- Accumulated absolute/relative movement, ignored first movement and active/grabbed state.
- Cinematic smoothing, scoping and inversion.
- Rotation updates and pitch limits, including previous-rotation state.
- Fractional wheel accumulation, direction changes, selection and spectator-speed helpers.
- Grab-position and Control-click emulation helpers.
- Keyboard direction normalization, use/sneak scaling and square-movement arithmetic.

The platform-specific `winit` quit-shortcut adapter is not translated. The `interaction_input.rs` reference is downloaded but its tick/callback behavior is not translated yet. These functions are compiled into the C core and exercised by its probe; they are not yet connected to a playable client simulation.

## Comparison

Run `tools/test-horizon-port.ps1`. It checks the upstream reference blob hashes, compiles the pinned Rust mouse module and unchanged keyboard arithmetic, compiles the C translation, and compares 4,352 output rows exactly:

- 4,096 mouse/frame/rotation/scroll sequences.
- 256 key/use/sneak combinations.

Double and float outputs are compared as raw hexadecimal bit patterns. The tested rows match. This covers those scenarios, not every possible input or an entire game port. Rust is required only for this development reference test; no Rust engine or runtime is embedded in the C application.

## Continuing the conversion

Port dependency groups from the pinned tree, record the Rust source paths and C equivalents, and keep paired probes or the relevant vanilla harnesses. Input lifecycle/interaction, NBT and paletted chunk storage, resource models, integrated-server/world state, rendering, simulation/content, world generation, menus and advancements remain substantial work. Preserve the server/client architecture; do not replace missing systems with the retired sandbox.

No executable currently constitutes the completed Minecraft-to-C conversion. The existing title executable remains an incomplete reference build.
