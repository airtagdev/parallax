# Implementation roadmap

The first release establishes a small, playable C application. Progress is measured by features in the executable and reproducible tests, rather than a percentage of Minecraft completeness.

## 1. World and player foundation

Implemented: generated terrain, simple meshing, textures, movement, collision, mining/placement, inventory and custom saves.

Next: split world, simulation and rendering modules; asynchronous generation and meshing; chunk streaming with practically unbounded coordinates; robust save migration; texture mipmaps and ambient occlusion; actual sky/block lighting. Add NBT/Anvil reading before claiming vanilla world support.

## 2. Survival and progression

Implemented: basic health/hunger, four recipes, resource collection, respawn and 24 custom achievement triggers.

Next: item stacks and drops, tools, mining times, durability, workstations, recipe data loading, containers, food items, fall/drowning damage and a trigger-based advancement engine. Importing the vanilla advancement JSON catalog alone will not make its triggers work; each required gameplay system and trigger must be implemented and exercised.

## 3. Terrain and entities

Implemented: deterministic hills, biome-like surface variation, caves, water, ores, trees, basic animals and zombies.

Next: proper biome distribution, structures, incremental world generation, richer caves, fluid ticks, entity collision and pathfinding, spawn/despawn rules, model loading, loot tables, farming, breeding and additional dimensions.

## 4. Compatibility and multiplayer

Not implemented: Java protocol, Microsoft sign-in, vanilla world round-trips, Java mods or server hosting.

First isolate an authoritative simulation from the client, add deterministic tick/replay tests, and use a local reference game to compare representative behavior. Add networking only after chunk and entity state are stable. Any future account flow should be interactive OAuth/device authorization; never collect account passwords in the game or source repository.
