# Acceptance specification

## Product

Reimplement Minecraft: Java Edition **26.3**, not a Minecraft-inspired game. The game/engine implementation must be C. The target player experience is the original game's experience. A known discrepancy is unfinished work, even if a custom replacement is playable or attractive.

## Presentation

Match every screen, widget, dimensions, positioning, GUI scale, font/provider, glyph advance, shadow, color, texture, tooltip, focus state, mouse/keyboard navigation, scroll behavior, transitions, accessibility behavior, narration, animation and sound. Use resources from the user's local owned installation. Match title panorama, splash behavior, inventory, creative tabs/search, crafting screens, recipe book, HUD, chat, debug views, advancements, pause/options, loading, world selection/creation, death/respawn and every other vanilla screen.

Match block models, blockstates, entity and block-entity geometry/animation, held items, particles, clouds, sky, weather, biome tint, lightmap, ambient occlusion, fog, blending, water/lava, camera interpolation, FOV and rendering state. A cube for a non-cube block or a generic box for a mob is a placeholder, not completion.

## Controls and feel

Match default keybindings, mouse sensitivity curve, frame/tick timing, movement acceleration, friction, collision/stepping, sneaking/ledge behavior, sprinting, swimming, flight eligibility and double-tap behavior, jumping, camera bob, mining duration, reach, cooldowns, item use and all inventory interactions. Do not introduce survival flight, starter items, instant mining, altered nutrition, or custom shortcuts as replacements for vanilla behavior.

## Simulation and content

Match block/entity ticking, scheduled/random ticks, fluids, redstone, physics, mob AI, pathfinding, spawning/despawning, attributes, combat, effects, food, equipment, item components/durability, containers, crafting/smelting and all workstations, loot, recipes, tags, registries, dimensions, structures, biomes, terrain and seed behavior. Imported JSON is data; the C systems which interpret and execute it must be implemented too.

## Advancements

Use the original definitions, parent graph, visible/hidden state, criteria, requirements, triggers, rewards, toasts, GUI and persistence. Do not substitute a custom list or mark a definition complete because it loads. Validate advancement events against the reference simulation. The local 26.3 resources currently contain 1,866 definitions, of which 126 have display objects; the remainder include recipe unlocks.

## Worlds and compatibility

Implement native NBT/Anvil region/chunk storage and the appropriate 26.3 data version. Preserve player, entity, block-entity, inventory and advancement data. Implement the native world-creation and generation pipeline. Never change a user's original save while developing: compare isolated copies. A custom finite generator and `.pxw` format do not satisfy this requirement. Pin protocol 777 and data version 5023 for this target; unsupported version upgrades must be explicit.

## Implementation provenance

Use the MIT-licensed Horizon Rust implementation as the source-port baseline, with its license/attribution preserved and a pinned source revision. Translate the actual systems to C and compare them against their Rust implementations. Do not embed or distribute copied/decompiled Minecraft Java code, bytecode, game-native libraries or original shader programs. Extract assets and declarative resource data locally and keep them out of Git. Reference tools may run the original game/JDK as a black box. A C wrapper around Java or Rust is not a C rewrite. Horizon has its own remaining compatibility gaps; porting it does not automatically establish perfect vanilla equivalence.

## Evidence and reporting

Record implemented behavior and verified coverage separately. Use native screenshots at matching viewport, GUI scale, language, assets and state; compare pixels in deterministic areas and controlled sequences for animation/input. Compare deterministic simulation traces, random streams, seeds/chunks, save round-trips and advancement events. Record known discrepancies and missing tests. A passing finite test set is not proof of universal equivalence; never label the whole game 100% complete on that basis.
