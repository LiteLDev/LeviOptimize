# Third-party notices

The following files adapt code or algorithms from [Endstone](https://github.com/EndstoneMC/endstone):

- `src/features/PlayerNetworkingOpt.cpp` and `ChunkOptimizationState.h`: `099d6f028` (batched influence updates).
- `src/features/RandomChunkOpt.cpp` and `ChunkOptimizationState.h`: `6f0c32205` and `bec9a2fd7` (autosave key snapshots).
- `src/features/CircuitSceneGraphOpt.cpp`: `55e5e34be` (stale redstone relationships).
- `src/features/ChunkGenDispatcherOpt.cpp` and `ChunkGenDispatcherSignature.h`: `7049af40e7ec7364627d9a2a1e14ebe4c2eb714d` (generation task limits).
- `src/features/ChunkCachesOpt.cpp` and `ChunkCacheSignatures.h`: `2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce` (blending/existence cache limits).
- `src/features/VerifiedBytePatch.h`: shared guarded patch lifecycle adapted while porting the two patch groups above.
- `src/features/VanillaWaypointsOpt.cpp` and `VanillaWaypointsState.h`: `874c1fa310545487cc86f7f7f5c0e207514bdb3f` (locator waypoint membership checks).
- `src/features/EntityPacketRecipientsOpt.cpp` and `EntityPacketRecipientsState.h`: `fa7b3d4eb488c5180bf722d7777b12536e35cc50` (entity broadcast recipient selection, without the active-user snapshot).

Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

These portions are licensed under the Apache License, Version 2.0. A copy is included in [LICENSES/Apache-2.0.txt](LICENSES/Apache-2.0.txt). The modified files identify the adaptations for LeviLamina, including scoped batch state, cache lifetime cleanup, and bounded instruction matching with restoration on disable.

`ChunkLeakFix.cpp` also follows the native map cleanup approach described by Endstone commit `d520c1938`; it calls BDS's existing method rather than copying Endstone's implementation of that method.

LeviOptimize's project license is provided in [LICENSE](LICENSE).
