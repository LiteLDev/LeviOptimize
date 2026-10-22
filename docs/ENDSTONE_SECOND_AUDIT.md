# Endstone 性能移植第二轮审计

审计日期：2026-10-23。本文补充用户提供的 `BDS_PERFORMANCE_PORTING.md` 与 [移植审计](ENDSTONE_PORTING.md)，重点检查原索引遗漏及“同一提交被整体延期，实际可以拆出安全部分”的情况。逐编号状态和未移植原因见[原文档对照清单](PORTING_CHECKLIST.zh.md)。源码审查与编译结果不代表已经完成运行基准。

## 结论与实际检查范围

第二轮确认两项值得独立移植的原生路径：`Dimension::sendPacketForEntity` 单次调用内收集收件人、`VanillaWaypointManager::update` 成员完整时跳过重复更新。它们都包含在原索引已经列过的大型优化提交中，第一轮将整个快照/定位器系列一起延期，遗漏了可单独适配的部分。当前已分别落实为默认关闭的 `optEntityPacketRecipients` 和 `optVanillaWaypoints`，连同第一轮五项共七项新配置。

还发现原索引没有列出的 ActorUniqueID 哈希修正、踢出连接收尾、物品输入防卡服、延迟包限速配置，以及旧 RakNet 防护被原生修复替代的历史。这里既有 SDK 已修复的问题，也有负载保护和需要目标二进制证据的候选，不能都当作新的通用 TPS 开关。

本地完整提交图包含 5,630 个可达提交；源码采用 partial clone，部分旧 blob 通过 GitHub commit API 补读。检查方法为全提交图候选扫描、运行时 hook/网络/配方/启动器路径历史扫描、当前实现与候选 diff 逐项比对、撤销/替代关系与跨分支核查。关键词涵盖 perf/optimi/cache/alloc/thread/batch/snapshot，也检查 hang/flood/spam 与源码 `#blameMojang` 注释。**没有逐一通读全部 5,630 个提交的 diff**，也没有据标题直接认定收益。

- stable `main`：[`bb0475b64744106b328238b3f51d77576ddc6569`](https://github.com/EndstoneMC/endstone/commit/bb0475b64744106b328238b3f51d77576ddc6569)，仍是 v0.11.14 / BDS 1.26.52.3 基线。
- 较新分支单独检查：`develop`=`4d88aef98a6f6db4e9ac37177dfa03bd5831ebcd`，`v0.11`=`b3857a990210972a59c34839c306baae2b41d627`，`v0.12`=`22527eeaea23aeef6e21d8ef24d7869488efc7ed`，`perf/chunk-gen-dispatcher`=`569653076feb1b5256b391271723d007003ea044`；同时检查 `feat/brigadier`、`prototype`、旧版本分支候选。没有发现基线之后新合入 stable main 的另一批原生优化。
- 目标初始 API 核查使用 LeviLamina v26.51.0 clone；实际 xmake 解析 `26.51.*` 为 **26.51.7**，必须区别两者，尤其是 ActorUniqueID 哈希。
- 未具备真实 BDS 工作负载、机器人或目标二进制反汇编证据。源码与编译检查不等于 TPS/CPU/内存收益已经测量。

## 已拆出的两项

### Entity packet 收件人

来源：[`fa7b3d4eb488c5180bf722d7777b12536e35cc50`](https://github.com/EndstoneMC/endstone/commit/fa7b3d4eb488c5180bf722d7777b12536e35cc50)，当前 `src/endstone/runtime/bedrock_hooks/dimension.cpp:83–117`。同一提交还替换了 `Dimension::forEachPlayer` 和 `Level::getActivePlayerCount`，但广播本身不依赖跨调用 active-user 快照。

目标 `src/features/EntityPacketRecipientsOpt.cpp` 保留原生 `forEachPlayer`，只在一次广播内读取 actor ID、按附近实体集合选择收件人，最后调用原生 `PacketSender::sendToClients`。LL26.51.7 提供命名 API/字段：`Dimension::$sendPacketForEntity`、`ActorUniqueIDComponent::mActorUniqueID`、`ServerPlayer::mNearbyActors`、`UserEntityIdentifierComponent::mNetworkId/mClientSubId`、`ILevel::getPacketSender`。保留 except/self/附近玩家与子客户端 ID；选中玩家缺 identifier 时不发送给该玩家；任一必需 ID 无效时丢弃全部临时结果，再完整走原始广播，不会先发一部分再 fallback。没有持久玩家指针、弱引用字节复制或跨调用缓存。仅专用服务端装 hook，客户端保留原生行为并提示。

收益依据是少做重复 actor ID/component/hash 查询；没有外推具体百分比。风险是直接实现 vanilla relevance 会绕过其他 mod 的 `Dimension::isActorRelevantForPlayer` 自定义规则，启用前应对照收件人集合。`EntityPacketRecipientsState.h` 是生产代码实际调用的纯辅助状态，10 个测试覆盖 except、self、nearby、缺 identifier、子客户端顺序、无效 ID、停止与禁止部分发送，独立审查暂未发现确定问题。

### VanillaWaypointManager 成员检查

来源：[`874c1fa310545487cc86f7f7f5c0e207514bdb3f`](https://github.com/EndstoneMC/endstone/commit/874c1fa310545487cc86f7f7f5c0e207514bdb3f)。当前上游分别修改 `VanillaWaypointManager`、`ServerWaypoint` 和 `PlayerLocationSender`。

目标 `src/features/VanillaWaypointsOpt.cpp` 仅拆出 manager 部分：已有组且开关状态未变时收集 waypoint actor IDs，排序并检查玩家列表，只有没有缺失的存活非自身玩家才跳过原生更新；组创建、开关切换、新玩家仍交给原生。LL 命名的 group、waypoint `tryGetActorID`、player-list、`Level::fetchEntity` 足以适配，不需跨调用缓存。它不是完整定位器矩阵移植，也没有替换每个 waypoint 的位置/可见性/发包逻辑。

`ServerWaypoint::update` 原本也值得拆分，但当前 26.51.7 的 `WorldPosition` 成员仍未命名，未据布局猜偏移。更大的 `PlayerLocationSender::_forEachClientPlayerPair` 优化依赖 RTTI 回调分类、pair/slot 缓存、sent-map 临时交换、玩家移除订阅与寿命验证，继续延期。

## 原索引确实遗漏的项目

### ActorUniqueID 哈希：目标 26.51.7 已修复

[`c54592af5184910bf056682c422fb9e96dd696ba`](https://github.com/EndstoneMC/endstone/commit/c54592af5184910bf056682c422fb9e96dd696ba) 将 `src/bedrock/world/actor/actor_unique_id.h` 的低位 mask 从 `0xffffff` 修为 `0xffffffff`。提交解释 BDS 自有 unordered_map 在 ID 达到 `0x1000000` 后会查错桶；Endstone 的地图初始化任务因此可能反复入队，性能问题来自错误的 SDK 哈希与框架任务行为。提交中的 Linux BDS 1.26.51 对照不是本次对 Windows 目标重新反汇编的结果。

LL v26.51.0 审计 clone 也有 low24 实现，但实际安装的 **26.51.7 `include/mc/legacy/ActorUniqueID.h:34–40` 已使用 low32**。因此不重复 patch std::hash，也不把已修 SDK 算成新 BDS 优化。Entity packet 优化安装前检查含 bit24..31、负值与高位的 SDK 哈希样本，与正确 high32/low32 组合参考比较；不匹配就不装 hook。该检查能拒绝已知旧 SDK 差异，不能替代未来目标 BDS 的独立哈希验证。

### 已踢出连接继续占资源：当前证据不足以替换目标网络寿命流程

历史：[`f454f49adbad2448e66c3cafe497dd191fc38a74`](https://github.com/EndstoneMC/endstone/commit/f454f49adbad2448e66c3cafe497dd191fc38a74) 初始修复；`06ce8871e4ce49b09079845f097f839e23a094e5` 注入断连包；`a87b940e1d79d1cc5d677f0fe529d51e0a218e05` 改成只关闭 PrimaryClient；`6e4b448c5533405c1e60d4751862472c76a2f335` 修正崩溃相关处理；最后 [`ccdef427fc488834d1e719ad0b0e0724dbe2a16e`](https://github.com/EndstoneMC/endstone/commit/ccdef427fc488834d1e719ad0b0e0724dbe2a16e) 撤下直接 `closeConnection`。该提交明确说明直接关闭在 `/reload` 路径可能双重调用 `onConnectionClosed`，产生 UAF。

当前 `server_network_handler.cpp` 先调用原 `disconnectClientWithMessage`，仅 `SubClientId::PrimaryClient` 调 `NetworkSystem::setCloseConnection(id, reason)`。这个 setter **是 Endstone 自己写的辅助函数，不是一个已经确认可调用的原生 MCAPI setter**。`src/bedrock/network/network_system.cpp` 查找同 ID 的现有 connection，写 `should_close_connection_`、reason、`closed_time=steady_clock::now()`，随后由正常接收任务关闭一次。

LL26.51 命名字段包括 `mConnectionsMutex`、`mConnections`、`mShouldCloseConnection`、`mShouldCloseConnectionReason`、`mClosedTime`，并暴露 disconnect 相关 MCAPI，但仅凭字段和 `disconnectPrimaryClient` 名称不能证明目标 BDS 尚未正确调度关闭。若后续用目标二进制/复现场景确认仍有缺陷，设计应是 origin 后仅 PrimaryClient、受连接锁保护、查找现存 connection、保留已在关闭中的 reason/time、完整处理三个状态字段并让正常接收任务完成通知；不能只写一个 bool，更不能恢复旧的立即 close。当前不实施；收益主要是恶意/异常连接的 CPU 与资源上限，不应声称普通 TPS 提升。

### Item 输入反序列化防卡服：仍在当前上游，须核查目标是否已修

[`81d3ee0391bdae8e80b719db999b110c14e9ac7b`](https://github.com/EndstoneMC/endstone/commit/81d3ee0391bdae8e80b719db999b110c14e9ac7b) 标题是 vulnerability/server hang，原索引未列。当前 main 的 `bedrock_hooks/item.cpp`、`Item::readUserData` 的 `ENDSTONE_HOOK`、windows/linux symbol 表与 CMake 均仍在，不能当成已撤下。

实现读取 marker 和版本时检查失败；marker=-1 时只接受版本1并读取 NBT，marker=0 继续，其余格式返回，最后 `stack.deserializeComponents(input)`。LL26.51 有 `Item::$readUserData` MCAPI 和 `ItemStackBase::deserializeComponents` LLAPI；LL 的后者源码会检查 count/string 读取结果并在失败时返回。这证明接口可接入，却不能证明目标原生 reader 仍有同一缺陷，也不能证明直接禁止旧 marker 格式的兼容性。未替换生产反序列化器；后续应先建立畸形/截断输入及合法物品格式回归，再比较目标 reader 的行为。归类为输入负载保护候选，不计入新增 TPS 选项。

### NetworkStackLatencyPacket 限速：配置层保护，不是已经实现的 mod flag

[`77418294fe7af8edc117e90da88769f525e85b38`](https://github.com/EndstoneMC/endstone/commit/77418294fe7af8edc117e90da88769f525e85b38)，来自 `84c429bc08112fd889d8eb9c34285c34c1e4adb2`，当前 `endstone/cli/base.py:302–338`。上游说明原生 `NetworkStackLatencyPacket`（packet ID **115**）在登录前可能无界处理，产生 CPU 负载；使用 BDS 自带 bucket limiter 即可约束。

真实路径为 **服务端目录下 `packetlimitconfig.json`**（与 executable/server folder 对应，不是 mod 的 config.json）。下例是 `limitGroups` 中的一个完整 group；应合并到已有数组，保留其他组及根字段：

```json
{
  "limitGroups": [
    {
      "minecraftPacketIds": [115],
      "algorithm": {
        "name": "BucketPacketLimitAlgorithm",
        "params": {"drainRatePerSec": 5, "maxBucketSize": 20}
      }
    }
  ]
}
```

上游保留语义：任何已有 group 的 `minecraftPacketIds` 数组已经包含115就完全不改；读取/解析失败、根不是 object、`limitGroups` 不是 array也返回；缺文件可创建，缺数组可补充，然后仅追加该规则。不会把所有规则换成示例。上述字段和值来自实际提交和当前源码，不是推测的配置键。

这是**原索引遗漏的配置/负载保护项**。Endstone 在启动器 bootstrap 更新配置阶段、启动 BDS 前修改；LeviOptimize 在 mod 装载时机不同，本次没有静默写宿主文件，没有新增启用该改写的 flag，也没有验证运行中配置热重载。适合提供管理员显式采用的启动前配置建议；不承诺普通玩家数量下的 TPS 收益。

## 排查后的历史替代关系与排除项

| 项目 | 证据与结论 |
| --- | --- |
| RakNet ACK/NAK 范围上限 | 初始 `f6f09539854444588017f989b2c2b43ac85a393e`、`ebe6995f1f1a9c2e83066658116bbae9402f439b`；[`4bca69dd8d78bfae6fa32ee3eeb604aa1b243c04`](https://github.com/EndstoneMC/endstone/commit/4bca69dd8d78bfae6fa32ee3eeb604aa1b243c04) 明确 BDS 已有 ranges<=0x400、total ACKs<=0x4000 及0xFFFFFF检查，删除重复运行时输入过滤。现存 RakNet recipe patch 给 Endstone 自带 RakNet 构建补同类检查，不代表应该给 BDS 再加一遍解析。`6676e26b00059ffd482a646ab36a95a72a9aedd6` 又移除冗余0x86检查。 |
| NetherNet 共享 UDP/发布地址 | [`9840bf5da9cb66266232708264da6b6b005dbfbc`](https://github.com/EndstoneMC/endstone/commit/9840bf5da9cb66266232708264da6b6b005dbfbc)、后续 STUN `9190d7622a4ad74a8024bdbc75827839bd6df814`。全局UDP和信令端口/发布地址设置及401日志处理，属于连接配置/网络可用性。LL目标 `TransportConfiguration`/HTTP结构仍有大量未命名字段，缺安全 ABI 适配证据，不记为普通TPS项。 |
| 数据库/低优先级日志早退 | `a42076ffc5218decf378b418187a7a5da85dd8ee`、`bdb2ad704c6666fd446c1e48e74536e873b3dd0b`、`f0cbdedc05d40ff10742ac3f1191c7098d50bd48`；当前 logger 对 Database/System 一些低级日志在格式化前早退。LL有自己的日志设施与插件诊断需求，没有证据支持为此重写日志链或宣称显著TPS。 |
| “scripting disabled”标题 | `b7dac6c070f61e89595e4abac3208d9d97608457` 实际是在用户本来就关闭脚本时避免空 handler 崩溃，不是通过默认关闭脚本提速。 |
| 随机种子/混合结果旧方案 | `036a4adc87ce8474bb27ff0c62431e4bc3fbac24`、`1c496aee49cd9c443d82139e07036d41c06c408a` 被 `f1f0b65f19626c9f38ba313b777e5affd26693b3` 移除；`a46a12cdbe098e3bfd71b328ec7bca16c6346618` 的旧 blend memo 被后续 cache-cap 方案替代，不能叠加复活。 |

## 已知可观收益候选为什么仍未全部移植

| 候选 | 当前实际实现、LL适配证据与风险 |
| --- | --- |
| 网络 libdeflate/peer splice/thread scratch | `4624070dd`、`e41e349ad`、`42b472028`、`fe86ade3f` 当前主线网络链。目标缺 libdeflate 依赖，相关方法部分是 MCNAPI；MCNAPI不表示绝对做不到，但须独立验证符号、协商snappy/hybrid、rawDEFLATE格式、长度/坏数据、buffer消费、重入、peer寿命与其他mod hook。不能只换一次compress调用。 |
| Worker backlog | [`cd97eea16d90c1ec686e54683bb444e2f17c495c`](https://github.com/EndstoneMC/endstone/commit/cd97eea16d90c1ec686e54683bb444e2f17c495c)；LL WorkerPool 成员仍未命名、ASYNC为MCNAPI。缺目标 backlog 成员证据，未猜偏移。 |
| Parked publisher chunks | [`099d6f0287bb0ac5f7cfc7653b41afec8083d4ba`](https://github.com/EndstoneMC/endstone/commit/099d6f0287bb0ac5f7cfc7653b41afec8083d4ba) 的另一部分；需要hash/node ABI、提取再插入、chunk/view/传送/切维度寿命恢复，不能和已完成的 influence batching 混为一谈。 |
| Active-user快照 | `fa7b3d4eb` 的其余部分；涉及weakref、玩家移除、registry销毁、递归callback与维度筛选。广播已单拆，快照仍需目标生命周期验证。 |
| 随机tick预取 | [`6e7417c001190d497aef2486925c700f3ede9a00`](https://github.com/EndstoneMC/endstone/commit/6e7417c001190d497aef2486925c700f3ede9a00)、最终整理 `fdea21001193a4aa91fd9725bdcc526f85530474`。命名subchunk存储可用，但预测原生样本所用 `LevelChunk::tickImpl::seed` static 尚未安全定位，不能额外消耗RNG或套别版地址。 |
| 新分支chunk priority | [`569653076feb1b5256b391271723d007003ea044`](https://github.com/EndstoneMC/endstone/commit/569653076feb1b5256b391271723d007003ea044)，只在 `perf/chunk-gen-dispatcher`；部分构造 FakePolicy 调用原生 scorer、持久policy地址缓存及采样比对不足以证明目标安全，继续延期。同分支 `7049af40e7ec7364627d9a2a1e14ebe4c2eb714d` 的任务上限已有独立实验开关。 |
| Endstone框架成本 | 新的packet玩家查找 `c2c778adf07cab6204cd7a2f3c0004eda9731c77`、v0.12皮肤缓存 `49293ffb38eb6eb20fb86214ffd5e7e187aff856`、无监听输入事件 `606813de0d7132d62bd7705569b60f85fe720101`；还有Python/scheduler/mapwrapper/event gates。它们优化Endstone自己增加的行为，LeviOptimize没有相同路径，不能复制后声称原生优化。 |

## 第一轮 cache cap 核查修正

`src/endstone/runtime/hook.cpp` 的两项容量补丁确实是有效原生优化，来源 [`2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce`](https://github.com/EndstoneMC/endstone/commit/2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce)，不是只有符号表而未安装。它们把比较常量的高字节0x00改0x10，将10,000/30,000上限增为1,058,576/1,078,576，降低分散玩家造成的频繁整表清空，代价是保留更多内存。第一轮早期因不同BDS地址/指令风险而延期，随后已经完成目标适配为默认关闭的 `optChunkCaches`。

目标实现通过 LL 导入声明/clang汇编探针确认装饰符号，使用 `SymbolView.resolve(true)` 找实际入口、Windows unwind确认命名函数精确边界、纯signature匹配且必须唯一、完整expected bytes二次检查、暂停其他线程修改、FlushInstructionCache，并只恢复本mod仍安装的字节；不复制上游绝对地址，不全进程搜签名。两个cache独立匹配，失败日志跳过；禁止将任一签名找到视为另一项也成功。禁用只恢复指令，不承诺立刻降低既有cache的RSS。

## 本轮交付与尚未证明的边界

本轮新增的 Entity 收件人生产文件/真实helper/10测试和 manager实现/11测试均已完成独立审查。最终代码在 C++23 下通过 Windows服务端/客户端 release 编译与链接，五组生产helper测试共54项通过，格式与差异空白检查通过；未运行BDS实服或机器人基准。

所有快路径应通过真实服务器对照收件人、split-screen子客户端、入退服、切维度、定位器开关/新玩家及第三方mod组合；在固定世界与距离配置下测全帧MSPT、CPU、RSS和分位延迟。没有测量前不写固定提速百分比，不将负载防护、SDK修复、撤回方案或框架独有优化累加为已移植数量。
