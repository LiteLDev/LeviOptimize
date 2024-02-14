# LeviOptimize

LeviOptimize is a performance optimization plugin for Minecraft Bedrock Dedicated Server (BDS), designed to enhance server performance by addressing low ticks per second (TPS) and frames per second (FPS) issues. This plugin operates within the LeviLamina framework, modifying BDS code to achieve significant performance improvements.

## Features

- **fixHopperItem**: Optimizes item transfers between hoppers and containers to enhance server TPS, addressing performance issues associated with item duplication.

- **fixMovingBlock**: Boosts FPS and server TPS by optimizing the processing of block entities with extensive NBT data during piston movements, minimizing the frequency and load of data transmissions from the server to the client.

- **optPushEntity**: Aims to mitigate the TPS decrease caused by entity cramming.

- **optBlockLookup**: Replaces the original block query mechanism with hashmap instead of map for queries, generally enhancing query speed but increasing memory usage.
---

LeviOptimize is not affiliated with Mojang or Microsoft.