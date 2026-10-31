# Version 0.1 validation

Built on Windows x64 with MSVC 19.51, C17, the Windows SDK and CMake. No Java or Rust runtime was used to run the game.

## Automated world integrity checks

- Identical terrain bytes for the same seed.
- Different terrain bytes for different seeds.
- Spawn does not intersect solid blocks.
- Movement through empty space.
- Wall and ground collision.
- Flight is limited to a saveable height.
- Recipes reject missing ingredients and account for consumed/produced blocks.
- Ray targeting selects the expected block and placement face.
- Placement consumes a block; mining returns it to inventory.
- Nearby mobs change position during simulation.
- Saved blocks and state survive a load round-trip.
- Deliberately corrupted save is rejected.
- Failed load leaves the current world intact.

Run using CTest or `Parallax.exe --self-test`. The latter writes `self-test-results.txt` in the working directory and returns nonzero on failure. Tests use a separate temporary save directory, not a player slot.

## Rendering verification

`--smoke-test` exercises the actual menu button and keyboard handlers: main menu, world slot, world creation, gameplay, inventory, achievement list, pause, and save/return. It verifies the screen transitions and that the created world was saved. A click is consumed by a button so it cannot activate a second screen's button in the same frame. The test uses an isolated save directory which is removed afterward.

The test captures gameplay, inventory, achievement list, pause and main menu using `glReadPixels`; captured views have been visually inspected. The test does not verify every real input device or long play sessions. Real gameplay feedback is still needed.

## Known testing limits

No Minecraft parity harnesses, vanilla save compatibility tests, multiplayer tests, second-machine tests or comprehensive performance benchmarks have been run. There is no claim of vanilla accuracy, complete advancement support, production stability or measured speedup over Java/Horizon.
