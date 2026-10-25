# LeviOptimize

[![English](https://img.shields.io/badge/English-informational?style=for-the-badge)](README.md)
![中文](https://img.shields.io/badge/简体中文-inactive?style=for-the-badge)

LeviOptimize 是运行在 [LeviLamina](https://github.com/LiteLDev/LeviLamina) 上的 Minecraft 基岩版服务端 / 客户端性能模组。
它针对 TPS 偏低的热点原版路径做替换：区块发送、实体广播、红石更新、定位栏、随机刻和异步 worker 调度。

每个功能相互独立，都有自己的开关，可以在运行时用 `/lo` 启用或禁用。

## 目录

- [安全性](#安全性)
- [安装](#安装)
- [使用](#使用)
- [命令](#命令)
- [功能](#功能)
- [诊断](#诊断)
- [从源码构建](#从源码构建)
- [贡献](#贡献)
- [许可证](#许可证)

## 安全性

LeviOptimize 是独立项目，不隶属于 Mojang 或 Microsoft，也不受其官方支持。项目开源且免费使用；请自行承担使用风险，
作者不对服务器或数据可能造成的损害负责。

## 安装

### 使用 Lip

```sh
lip install github.com/LiteLDev/LeviOptimize
```

### 手动安装

1. 从[发布页面](https://github.com/LiteLDev/LeviOptimize/releases)下载最新版本；
2. 解压 `LeviOptimize-windows-x64.zip`；
3. 服务端复制 `LeviOptimize` 目录到 `plugins/`，客户端复制到 `mods/`。

## 使用

首次启动时，模组会在服务端的 `plugins/LeviOptimize/config/` 或客户端的 `mods/LeviOptimize/config/` 生成
`config.json`。每个安装目录拥有自己的配置文件。

配置包含版本号和按功能分区的内容。只有开关的功能直接写布尔值；带参数的功能写成对象：

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

- `features`：性能功能；
- `commands`：命令功能本身；
- `diagnostics`：测量工具。

缺失条目按默认值处理，未知分区和未知功能条目在回写时原样保留；修改后执行 `/lo reload` 即可生效，无需重启服务端。

## 命令

命令本身也是功能，可以在 `commands` 里关闭。

### 服务端：`/lo`（GameDirectors）

- `/lo list`：列出服务端功能及其运行 / 配置状态；
- `/lo triggers`：列出已注册的触发器；
- `/lo reload`：重新读取 `config.json` 并重新判断触发器；
- `/lo reload <feature>`：只重载某个功能；
- `/lo enable <feature>` / `/lo disable <feature>`：切换功能开关并写回配置；
- `/lo profile`：查看 `full_frame_timing` 诊断的当前 / 平均全帧 MSPT 与 TPS。

### 客户端：`/clilo`

动作与服务端一致，权限为 `Any`。客户端命令不依赖服务端事件。

## 功能

| 功能 | 配置键 | 说明 |
| --- | --- | --- |
| `ChunkCachesOpt` | `chunk_caches_opt` | 提高区块混合 / 存在性缓存上限，避免玩家分散时反复清空缓存；会保留更多内存。 |
| `ChunkLeakFix` | `chunk_leak_fix` | 玩家断线时通过原生清理移除地图 tracker、装饰和脏状态。 |
| `CircuitSceneGraphOpt` | `circuit_scene_graph_opt` | 电路更新前清理失效的红石供电关联，保留仍待加入的组件。 |
| `EntityPacketRecipientsOpt` | `entity_packet_recipients_opt` | 仅服务端：实体广播只读一次实体 ID，按玩家附近实体批量收集接收者；ID 无效时回退原生路径。 |
| `LocatorOpt` | `locator_opt` | 按观察者 / 被观察玩家缓存上次发送的定位位置，没变化时跳过原生可见性判断。 |
| `ParkedChunksOpt` | `parked_chunks_opt` | 暂存暂时发不出的 publisher 队列节点，在区块加载或视野变化时重新检查。 |
| `PlayerNetworkingOpt` | `player_networking_opt` | 玩家网络处理结束后，每个区块生成优先级列表只重建一次，而不是每个玩家重建一次。 |
| `PlayerSnapshotOpt` | `player_snapshot_opt` | 复用经过校验的活动玩家快照，加速维度遍历和人数统计。 |
| `PushEntityOpt` | `push_entity_opt` | 可选的实体推动限制：关闭零向量推动、限制每 tick 推动次数、玩家相关推动不限次。 |
| `RandomChunkOpt` | `random_chunk_opt` | 自动保存时对区块坐标采样，避免每次都遍历整个区块表。 |
| `RandomTickPrefetchOpt` | `random_tick_prefetch_opt` | 预测原版随机刻要读的存储并预取，不推进游戏 RNG。 |
| `SeenPercentOpt` | `seen_percent_opt` | 按坐标和包围盒缓存 SeenPercent 值。 |
| `VanillaWaypointsOpt` | `vanilla_waypoints_opt` | 定位栏路标的跟踪集合和图标没变化时，直接跳过原生更新。 |
| `WorkerBacklogOpt` | `worker_backlog_opt` | 校验目标字段后，把异步 worker 的积压等待从 0.05 秒设为零。 |

## 诊断

`FullFrameTiming`（`full_frame_timing`，位于 `diagnostics`）是测量工具，不是优化。它统计
`Minecraft::update` 的整帧时间，并用 `Level::tick` 切分补跑 tick；`/lo profile` 输出最近 20 个样本。
可以像其他功能一样关闭。

## 从源码构建

需要 [xmake](https://xmake.io) 和 LLVM/clang-cl 工具链。

```sh
# 专用服务端
xmake f -c -m release --target_type=server
xmake

# 客户端
xmake f -c -m release --target_type=client
xmake
```

打包后的模组输出在 `bin/LeviOptimize`。

## 贡献

欢迎提交 issue、提问和 PR。

## 许可证

GPL-3.0-or-later © LiteLDev
