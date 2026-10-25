# LeviOptimize

![English](https://img.shields.io/badge/English-inactive?style=for-the-badge)
[![中文](https://img.shields.io/badge/简体中文-informational?style=for-the-badge)](README.zh.md)

LeviOptimize is a performance mod for Minecraft Bedrock Dedicated Server and the Bedrock client, built on
[LeviLamina](https://github.com/LiteLDev/LeviLamina). It targets low server TPS by replacing hot native paths with
cheaper ones: chunk publishing, entity broadcasts, redstone updates, locator labels, random ticks and async worker
scheduling.

Features are independent: each one has its own switch and can be enabled or disabled at runtime with `/lo`.

## Table of Contents

- [Security](#security)
- [Installation](#installation)
- [Usage](#usage)
- [Commands](#commands)
- [Features](#features)
- [Diagnostics](#diagnostics)
- [Building from source](#building-from-source)
- [Contributing](#contributing)
- [License](#license)

## Security

LeviOptimize is an independent project, not affiliated with or supported by Mojang or Microsoft. It is open source and
free to use. Use it at your own risk; the authors are not liable for any damage to servers or data.

## Installation

### Using Lip

```sh
lip install github.com/LiteLDev/LeviOptimize
```

### Manual installation

1. Download the latest release from the [Releases page](https://github.com/LiteLDev/LeviOptimize/releases).
2. Unpack the `LeviOptimize-windows-x64.zip` archive.
3. Copy the `LeviOptimize` folder to `plugins/` on a dedicated server or `mods/` on a client.

## Usage

On first start the mod writes `config.json` to `plugins/LeviOptimize/config/` on a server and
`mods/LeviOptimize/config/` on a client. Each installation owns its own file.

The config has a version and one object per feature section. A feature whose only option is the on/off switch is stored
as a boolean; a feature with parameters uses an object:

```json
{
    "version": 1,
    "features": {
        "chunk_leak_fix": true,
        "chunk_caches_opt": true,
        "push_entity_opt": {
            "enable": true,
            "disable_vec0_push": true,
            "max_push_times": -1,
            "unlimited_player_push": false
        }
    },
    "commands": {
        "server_feature_manage_command": true,
        "client_feature_manage_command": true
    },
    "diagnostics": {
        "full_frame_timing": true
    }
}
```

- `features` holds performance features;
- `commands` holds the command features themselves;
- `diagnostics` holds measurement tools.

Missing entries fall back to defaults, unknown sections and unknown feature entries are preserved when the file is
written back, and `/lo reload` applies edits without restarting the server.

## Commands

All command features are registered as regular features and can be turned off in `commands`.

### Server: `/lo` (GameDirectors)

- `/lo list` — list every server feature with its running/config state;
- `/lo triggers` — list registered triggers;
- `/lo reload` — reload `config.json` and re-evaluate triggers;
- `/lo reload <feature>` — reload one feature's section;
- `/lo enable <feature>` / `/lo disable <feature>` — change a feature's switch and persist it;
- `/lo profile` — current and average full-frame MSPT/TPS from the `full_frame_timing` diagnostic.

### Client: `/clilo`

The same actions for client features, using `Any` permission. The client command does not depend on server events.

## Features

| Feature | Config key | Description |
| --- | --- | --- |
| `ChunkCachesOpt` | `chunk_caches_opt` | Raises the chunk blending and chunk existence cache limits so dispersed players stop resetting them. Retains more memory. |
| `ChunkLeakFix` | `chunk_leak_fix` | Removes disconnected players' map trackers, decorations and dirty state through the native cleanup. |
| `CircuitSceneGraphOpt` | `circuit_scene_graph_opt` | Drops stale redstone power associations before a circuit update while keeping components that are still pending. |
| `EntityPacketRecipientsOpt` | `entity_packet_recipients_opt` | Server-only entity broadcast: reads the actor ID once and batches recipients from each player's nearby actors. Falls back to the native path on invalid IDs. |
| `LocatorOpt` | `locator_opt` | Caches the last sent locator position per viewer/observed pair and skips the native visibility check when nothing changed. |
| `ParkedChunksOpt` | `parked_chunks_opt` | Holds publisher queue nodes that cannot be sent yet and rechecks them when chunks load or the view moves. |
| `PlayerNetworkingOpt` | `player_networking_opt` | Rebuilds each chunk generation influence list once after the player networking pass instead of once per player. |
| `PlayerSnapshotOpt` | `player_snapshot_opt` | Reuses a validated active-player snapshot for dimension iteration and active-player counts. |
| `PushEntityOpt` | `push_entity_opt` | Optional entity-push limits: drop zero-vector pushes, cap pushes per tick, or leave player pushes unlimited. |
| `RandomChunkOpt` | `random_chunk_opt` | Samples chunk keys for autosave instead of walking the chunk map every time. |
| `RandomTickPrefetchOpt` | `random_tick_prefetch_opt` | Predicts vanilla random-tick storage reads and prefetches them without advancing the game RNG. |
| `SeenPercentOpt` | `seen_percent_opt` | Caches seen-percent values per position and bounding box. |
| `VanillaWaypointsOpt` | `vanilla_waypoints_opt` | Short-circuits locator waypoint updates when the tracked set and icon have not changed. |
| `WorkerBacklogOpt` | `worker_backlog_opt` | Sets the async worker backlog delay to zero after validating the target field. |

## Diagnostics

`FullFrameTiming` (`full_frame_timing`, section `diagnostics`) is a measurement tool, not an optimization. It times
the server update from `Minecraft::update` and splits catch-up ticks around `Level::tick`; `/lo profile` prints the
last 20 samples. It can be disabled like any other feature.

## Building from source

Requires [xmake](https://xmake.io) and an LLVM/clang-cl toolchain.

```sh
# dedicated server
xmake f -c -m release --target_type=server
xmake

# client
xmake f -c -m release --target_type=client
xmake
```

The packaged mod is written to `bin/LeviOptimize`.

## Contributing

Issues, questions and pull requests are welcome.

## License

GPL-3.0-or-later © LiteLDev
