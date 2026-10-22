# Endstone performance port audit

Audited on 2026-10-22 against all fetched Endstone branches and 5,630 reachable commits. The supplied performance index was used as a candidate list, then checked against current source and history.

- Endstone stable `main`: [`bb0475b64744106b328238b3f51d77576ddc6569`](https://github.com/EndstoneMC/endstone/commit/bb0475b64744106b328238b3f51d77576ddc6569), still the v0.11.14 / BDS 1.26.52.3 baseline. There are no later stable-main runtime optimizations.
- Target base: LeviOptimize [`e22a844`](https://github.com/LiteLDev/LeviOptimize/commit/e22a84421563cdb3108cd4e92ada0481f4e2e12f), Windows server/client, LeviLamina `26.51.*`.
- Initial API audit: LeviLamina v26.51.0; compilation resolves the existing dependency range to 26.51.7. Endstone's BDS address tables are not copied into this project.

## Included ports

All five new options default to `false`. Existing `fixChunkLeak` keeps its prior default. Change one option at a time in `features` and restart when comparing behavior and performance.

| Configuration | Upstream evidence | Adaptation |
| --- | --- | --- |
| `optPlayerNetworking` | [`099d6f028`](https://github.com/EndstoneMC/endstone/commit/099d6f0287bb0ac5f7cfc7653b41afec8083d4ba) | Hook named player networking / influence-update methods. Rebuild each affected policy once after the outer networking call. Per-thread scopes isolate nested calls; abandoned scopes discard pointers, including nested exceptions caught by their caller. Calls outside networking retain original timing. |
| `optRandomChunk` | [`6f0c32205`](https://github.com/EndstoneMC/endstone/commit/6f0c32205), [`bec9a2fd7`](https://github.com/EndstoneMC/endstone/commit/bec9a2fd7) | Cache position keys for exactly 400 samples, consume one bounded RNG draw per nonempty-map sample, resolve through the live source, and reject unloaded/expired chunks. Clear on source destruction, an observed empty map, and disable. |
| `optCircuitSceneGraph` | [`55e5e34be`](https://github.com/EndstoneMC/endstone/commit/55e5e34be) | Before native update, remove power associations whose component positions are absent from both all-components and pending-adds. Use LeviLamina named fields and compile-time layout checks. |
| `optChunkGenDispatcher` | [`7049af40e7ec7364627d9a2a1e14ebe4c2eb714d`](https://github.com/EndstoneMC/endstone/commit/7049af40e7ec7364627d9a2a1e14ebe4c2eb714d), `perf/chunk-gen-dispatcher`, 2026-10-13 | Experimental: raise generation task limits from 2n/6n to 8n/24n. Resolve the named BDS symbol directly, require its exact Windows unwind entry in the main image and a unique instruction sequence, and verify bytes again before writing. Disable restores only the sequence installed by this mod. Missing/changed/ambiguous code logs a warning and is skipped. |
| `optChunkCaches` | [`2bf178a99`](https://github.com/EndstoneMC/endstone/commit/2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce) | Independently validate and raise blending/existence cache limits from 10,000/30,000 to 1,058,576/1,078,576 entries. Use the same guarded patch lifecycle as the dispatcher, with signatures restricted to the exact named function boundaries and decorated symbols confirmed from target declarations. Higher limits trade retained memory for fewer full cache resets. |
| `fixChunkLeak` enhancement | [`d520c1938`](https://github.com/EndstoneMC/endstone/commit/d520c1938) | The target already removed player map trackers. Copy matching entity keys first and call BDS's `_removeTrackedMapEntity()` so decorations and dirty state are handled too. |

The input index's circuit description conflated pending updates with association items. The actual upstream code cleans items in affected **power association lists**, not pending updates themselves. This port also differs from LeviOptimize's removed historical `fixPowerAssocMap`: it retains native `update()` and does not replace `removeStaleRelationships()`.

Snapshot sampling is approximate while the chunk map changes. The bounded RNG consumption is preserved, but the selected position can differ from vanilla, and newly added positions wait for the next snapshot. Keys hold no chunk references. The cache mutex protects the mod's cache; it does not synchronize BDS's chunk map. These hooks retain BDS's original world-thread ownership requirements. Policy objects must remain alive during their enclosing successful networking call. Runtime checks must confirm that source destruction reaches the hooked base destructor on the supported binary.

Each cache-capacity patch is checked separately. A changed or unavailable function can leave one or both limits unchanged with a warning. Disabling restores installed instruction bytes; it does not forcibly clear native caches or promise immediate RSS reduction. No target offsets are guessed, and a matching source pattern alone is not a substitute for the runtime checks below.

## Later branch history

The stable release is not the newest branch state. These later commits were checked separately:

| Branch / commit | Finding |
| --- | --- |
| `perf/chunk-gen-dispatcher` / [`7049af4`](https://github.com/EndstoneMC/endstone/commit/7049af40e7ec7364627d9a2a1e14ebe4c2eb714d) | Included as the explicit experimental option above. It is not merged into stable main. |
| `perf/chunk-gen-dispatcher` / [`5696530`](https://github.com/EndstoneMC/endstone/commit/569653076feb1b5256b391271723d007003ea044), 2026-10-13 | Prunes player candidates when scoring chunks. Deferred: invokes the native scorer on a partially constructed `FakePolicy`, retains policy-address caches, and uses sampled comparisons to detect mismatches. It needs target-binary equivalence and lifetime testing. |
| `v0.11` / `develop` / [`c2c778a`](https://github.com/EndstoneMC/endstone/commit/c2c778adf07cab6204cd7a2f3c0004eda9731c77), 2026-10-18 | O(1) player lookup on Endstone's packet-event path. This path does not exist in LeviOptimize. |
| `v0.11` / [`bcaa746`](https://github.com/EndstoneMC/endstone/commit/bcaa746179ace1fc6211e01dcf5883130508771c), 2026-10-18 | Concrete Python cancellation bindings; Endstone-specific. |
| `v0.12` / [`49293ff`](https://github.com/EndstoneMC/endstone/commit/49293ffb38eb6eb20fb86214ffd5e7e187aff856), 2026-10-20 | Decode PlayerList only for hidden-player recipients and remove O(N²) skin caches. Endstone visibility features are absent from the target. |
| `v0.12` / [`606813d`](https://github.com/EndstoneMC/endstone/commit/606813de0d7132d62bd7705569b60f85fe720101), 2026-10-20 UTC | Skip constructing input events without listeners. Endstone event layer only. `v0.12` continues through [`22527ee`](https://github.com/EndstoneMC/endstone/commit/22527eeaea23aeef6e21d8ef24d7869488efc7ed) on 2026-10-21. |

## Remaining candidates and exclusions

| Candidate from the supplied index | Disposition / required work |
| --- | --- |
| Packet event bypass and peer/player lookup (`9cc00a03d`, `6369d0477`) | Endstone framework overhead. LeviLamina event emitters already own their hooks and remove them with the last listener; copying an Endstone plugin manager is inappropriate here. |
| libdeflate, peer splice, trace removal, thread scratch (`4624070dd`, `e41e349ad`, `42b472028`, `fe86ade3f`) | Deferred as a complete network-layer port. LeviLamina marks relevant `CompressedNetworkPeer` methods MCNAPI, and libdeflate is not a target dependency. Must verify symbols, Snappy/hybrid negotiation, raw DEFLATE format, size limits, malformed-data handling, peer lifetimes, reentrancy, scratch consumption, other mods' hooks, and shutdown before replacing the chain. |
| Parked publisher chunks (`099d6f028`) | Separate from the included influence batching. Requires identical hash/node ABI, extraction and reinsertion, chunk/view lifecycle subscriptions, teleport/source-change recovery, and a tested fallback. |
| Worker backlog (`cd97eea16`) | Deferred: target `WorkerPool` still exposes untyped `mUnk*` fields and `ASYNC()` is MCNAPI. Do not guess the backlog member offset from another BDS version. |
| Dimension active-user snapshot (`fa7b3d4eb`) | Deferred until weak-reference invalidation, removed players, dimension filters, and nested callbacks are verified against LeviLamina's entity model. |
| Random-tick prefetch (`6e7417c00`, final tidy `fdea21001`) | Deferred pending 26.51 subchunk/palette layout checks and verification that no RNG draws are added or changed. |
| Locator/waypoint caches (`874c1fa31`) | Deferred pending slot retirement, actor-ID reuse, locator toggle, lambda classification, and player-removal regression tests. |
| Plugin map rendering / actor wrappers (`b7f72336a`, `7fddd32e5`) | Endstone wrapper and renderer paths are absent. |
| Python / scheduler (`79e7fcc51`, `01d79b7d7`) | No corresponding runtime in LeviOptimize. |
| Linux mimalloc bootstrap (`9de00db9b`) | Target project builds Windows server/client and has no Python launcher. LeviLamina already declares its own allocator dependencies. |
| MSPT/TPS accounting, status and devtools (`03be20b63`, `6790acf3c`, `8e5eb0b7f`, `6adfe42ab`) | Endstone-specific facilities. Do not claim the target's `timing` values use Endstone's full-frame measurement. Use an external profiler for comparable full-frame MSPT. |
| Liquid/leaves event gates (`3946f6420`, `4759ba169`) | Endstone event implementations are absent from this mod. |
| Historical async networking / older libdeflate (`3ca0865dd` family, `65ac15afe`) | Distinct network architectures, not patches to mix into the current peer chain. `feat/async-batch` is no longer an advertised remote branch. |
| Blend-answer memoization / random-seed unlock (`a46a12cdb`, `036a4adc8`, `1c496aee4`) | Superseded by cache-capacity changes or removed by `f1f0b65f1`; not restored. |
| Header/build-time refactors | No demonstrated runtime TPS gain; not copied as gameplay optimizations. |

## Validation

The project and standalone tests use C++23. Snapshot sampling returns an owned `std::optional<ChunkPos>`, so a selected key survives snapshot replacement or destruction without borrowing a vector element. C++23 range operations express policy membership and materialize only map keys; refreshes still build a replacement before swapping it into the cache. Function resolution and byte-patch validation use `std::expected` to report concrete failures while retaining the bounded scan, double validation and conditional restoration.

The standalone [regression tests](../tests/README.md) cover real production helper behavior without a BDS executable: batching, recursion, exception recovery, thread isolation, snapshot refresh frequency, changing map sizes, stale keys, absence of chunk ownership, selected-key lifetime, and bounded unique instruction matching. The three suites cover 33 cases. CI runs them in addition to the existing Windows server/client build matrix.

Compilation and these tests do not establish runtime TPS gains. No BDS runtime workload or bot benchmark was available during this port. Before enabling by default or merging for release, run:

| Scenario | Required observations |
| --- | --- |
| Server and client startup, each option on/off, disable/re-enable | Hook symbols and layouts; no patch on mismatched/foreign/ambiguous instructions; dispatcher restoration. |
| 0 / 1 / 100 players, clustered and dispersed | Full-frame MSPT, TPS, CPU, RSS, chunk generation latency, influence rebuild counts. Keep world seed, view/tick distance and client generation settings fixed. |
| Repeated join/leave, teleport, dimension changes, shutdown | No pending policy use after lifetime end; source caches clear before address reuse; no retained chunk references. |
| Autosave during map growth/shrink and unload | Only loaded live chunks returned; saves continue; persistence remains correct after restart. |
| Map held, disconnect repeatedly | Tracker/decorations removed, dirty state preserved, loaded chunks and RSS stabilize. |
| Fast piston clocks / torches / repeaters / levers | Pending additions retained, redstone behavior unchanged, stale-list length and MSPT stable over a long run. |
| Experimental dispatcher with different core counts | CPU/memory peaks, task backlog, join latency and frame time; the fourfold limit can hurt constrained systems. |
| Expanded caches with dispersed players | Confirm both signature matches, cache reset frequency, CPU/MSPT and peak/steady RSS; disable/re-enable restores only the intended bytes and leaves later mods' changes intact. |

Preserve upstream notices in source and packaged output; see [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).
