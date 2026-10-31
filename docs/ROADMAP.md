# Rewrite milestones

All milestones follow SPECIFICATION.md. The experimental 0.1 sandbox is not the implementation baseline.

1. **Reference and resources:** pinned artifact hashes, safe local extraction, registries/catalogs, reusable black-box oracles and screenshot comparison. Initial importer/JSON/random-stream/widget tests now exist.
2. **Complete native shell:** title, world selection/creation, options, controls, resource selection, loading and all menu states, with keyboard/mouse/focus behavior, sounds and narration. Resolve every title discrepancy before declaring that screen complete.
3. **Native world data:** NBT, Anvil, block-state/model and item-component registries, safe save-copy round trips, chunk streaming, rendering, light and collision. Open native isolated test worlds before inventing a generator.
4. **Player and survival:** a deterministic 20 Hz authoritative simulation with exact native input transformation, movement, interaction, inventory/crafting and persistent state. Compare traces against native scenarios.
5. **Generation and ticking:** exact random/math streams and vanilla worldgen definitions, dimensions, biomes, structures, scheduled/random blocks, fluids and redstone. Verify same-seed chunks.
6. **Entities and content:** native model/animation and behavior systems, spawning, pathfinding, combat, workstations, food, loot, recipes and all remaining mechanics.
7. **Advancements and end-to-end progression:** native trigger/criteria/requirements/reward engine, original advancement GUI and saves; verify complete survival progression scenarios.
8. **Networking, broader fidelity and release:** native protocol/auth boundaries, additional hardware/GUI scales/locales, long-session saves and performance comparisons. Maintain the explicit gap ledger throughout.

These are implementation dependencies, not permission gates or a promise that the complete game can be delivered in one turn.
