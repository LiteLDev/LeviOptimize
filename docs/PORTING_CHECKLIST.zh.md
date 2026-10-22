# BDS_PERFORMANCE_PORTING.md 逐项对照

核对日期：2026-10-23。重新读取用户提供的完整原文，按原编号对照当前 PR 源码、Endstone 当前实现/历史和实际使用的 LeviLamina 26.51.7 接口。

**没有全部移植。** 原文第2节共21项：5项已实现、2项部分实现、14项未移植。这里的“已实现”指目标源码适配完成且构建通过；不表示已通过BDS实服验证。支持范围是项目现有的Windows服务端/客户端，实体收件人优化只在服务端生效。七个新增开关也不代表原文21项全部完成：其中一项来自原文基线之后的实验分支，地图清理则增强既有功能。

## 第2节：21项核心改动

| 编号 | 原文项目 | 当前状态 | 代码或未移植原因 |
| --- | --- | --- | --- |
| 2.1 | 无监听器时绕过包事件 | 未移植：框架不适用 | 优化的是Endstone自己的PacketSend/ReceiveEvent包装。LeviOptimize没有这条包装链；LeviLamina的事件发射器按监听器生命周期装卸hook。复制Endstone插件管理器会新增原先没有的成本，不能视为消除原生BDS开销。 |
| 2.2 | 包事件O(1)连接/玩家查找 | 未移植：框架不适用 | `PeerInfo`/subclient弱引用缓存服务于上述Endstone事件路径，目标没有同一条逐包查找链。原生实体广播的独立优化另见2.8，不能混称此项已完成。 |
| 2.3 | libdeflate、peer splice、移除PacketTrace | 未移植：网络链适配未完成 | 需要建立完整压缩peer替换，加入libdeflate并核对raw DEFLATE、Snappy/hybrid协商、大小/坏数据处理、连接关闭和其他mod hook；相关LL方法有MCNAPI，尚无完整的目标符号/生命周期验证。这是待开发的网络子项目，不是单独替换一次压缩调用。 |
| 2.4 | 线程压缩scratch/BDS批处理流复用 | 未移植：依赖2.3 | 必须与peer/批处理重构一起验证buffer消费时机、嵌套发送、异步队列转移和shutdown。当前没有相应的自定义压缩路径，直接插入scratch既不能移除原分配，也可能造成异步引用失效。 |
| 2.5 | 玩家网络处理后只重建一次优先级 | 已实现 | [`PlayerNetworkingOpt.cpp`](../src/features/PlayerNetworkingOpt.cpp)、[`ChunkOptimizationState.h`](../src/features/ChunkOptimizationState.h)，开关`optPlayerNetworking`。按线程/调用栈批处理、去重，处理嵌套与异常。 |
| 2.6 | 暂存尚不可发送的区块队列 | 未移植：队列ABI与生命周期缺验证 | 需要确认原生unordered节点/hash ABI及extract/reinsert，并在区块加载、视野变化、传送、切维度和source替换时恢复/重新订阅。不能把“同提交的优先级批处理已完成”当作这一部分也完成。 |
| 2.7 | 任意任务唤醒生成worker | 未移植：目标字段/变量未确认 | LL的WorkerPool backlog成员仍未命名，`MinecraftWorkerPool::ASYNC`为MCNAPI。没有已验证的26.51目标偏移/变量符号；只在别版地址上看到`0.05F`不足以证明可写。 |
| 2.8 | 活动玩家快照与实体广播 | **部分实现** | [`EntityPacketRecipientsOpt.cpp`](../src/features/EntityPacketRecipientsOpt.cpp)，`optEntityPacketRecipients`完成单次广播actor ID复用、nearby集合查找、subclient收件人批量发送。保留原生`forEachPlayer`；**未替换活动用户快照、forEachPlayer或getActivePlayerCount**，这些仍需验证weakref/registry销毁、移除玩家、嵌套回调和维度过滤。直接实现vanilla relevance还需检查其他mod兼容性。 |
| 2.9 | 自动保存随机区块快照采样 | 已实现 | [`RandomChunkOpt.cpp`](../src/features/RandomChunkOpt.cpp)、[`ChunkOptimizationState.h`](../src/features/ChunkOptimizationState.h)，`optRandomChunk`。400次重建、单次有界RNG、查活跃区块，销毁/禁用清理；采样返回拥有值的optional。 |
| 2.10 | 扩大混合/存在性缓存 | 已实现 | [`ChunkCachesOpt.cpp`](../src/features/ChunkCachesOpt.cpp)、[`VerifiedBytePatch.h`](../src/features/VerifiedBytePatch.h)，`optChunkCaches`。两个函数独立验证唯一签名和完整expected bytes，条件恢复；增加保留内存。未恢复旧memoization。 |
| 2.11 | 随机刻cache-line预取 | 未移植：原生seed未安全定位 | 命名SubChunk/存储API已经可用，真正剩余约束是未暴露的`LevelChunk::tickImpl::seed`静态变量。需要预测同一批原生随机样本，不能额外消耗RNG或使用Endstone另一BDS版本的地址。旧seed解锁patch已被撤回。 |
| 2.12 | locator成对缓存/manager/waypoint | **部分实现** | [`VanillaWaypointsOpt.cpp`](../src/features/VanillaWaypointsOpt.cpp)，`optVanillaWaypoints`只实现manager成员检查，开关变化/缺组/缺存活玩家走原生。**未实现成对slot矩阵和ServerWaypoint::update替换**：前者需回调分类、ID复用/移除清理，后者的WorldPosition成员在26.51.7仍未命名，缺布局证据。 |
| 2.13 | 无自定义地图渲染时跳过像素重绘 | 未移植：框架不适用 | 跳过的是Endstone的MapView/renderer和包修补成本。LeviOptimize没有插件地图渲染层，不能把新增renderer包装后再旁路当作原生优化。 |
| 2.14 | Python线程复用PyThreadState | 未移植：运行时不适用 | LeviOptimize是原生C++模组，没有Python解释器、GIL或相同callback链。 |
| 2.15 | Endstone调度器空闲阻塞 | 未移植：调度器不适用 | 目标没有Endstone ThreadPoolExecutor的10ms轮询。替换LeviLamina公共调度器属于不同依赖项目，不能凭该提交假定其有同一问题。 |
| 2.16 | Linux全进程mimalloc preload | 未移植：平台/启动时机不适用 | 项目当前只构建Windows，没有Linux wheel/启动器；运行时模组不是进程启动前LD_PRELOAD的等价位置。LL已管理自己的分配器依赖，也不能据此声称已实现全进程替换。 |
| 2.17 | actor到dimension包装器直接查找 | 未移植：框架不适用 | EndstoneActor/EndstoneDimension绑定层不存在于LeviOptimize；目标没有这次按名称二次查找。 |
| 2.18 | 全帧MSPT/TPS口径、status/devtools | 未移植：测量/工具系统不同 | 目标`timing`提供ECS采样，不是Endstone完整server-frame统计；尚未新增等价全帧采集器。Endstone的/status与隐藏窗口渲染线程在目标也不存在。后续收益需要外部全帧profiler或独立实现测量器，不能将现有ECS计时当成等价验证。 |
| 2.19 | 移除断线玩家地图tracker | 已实现：增强既有功能 | [`ChunkLeakFix.cpp`](../src/features/ChunkLeakFix.cpp)，沿用`fixChunkLeak`。复制稳定玩家ID/匹配key，调用原生`_removeTrackedMapEntity`清除tracker和装饰并处理dirty状态；需要长时实服验证事件时机和区块释放。 |
| 2.20 | 红石失效关联清理 | 已实现，并纠正原文描述 | [`CircuitSceneGraphOpt.cpp`](../src/features/CircuitSceneGraphOpt.cpp)，`optCircuitSceneGraph`。原文“删除pending_updates条目”不准确：实际上按pending updates找到受影响的**power association列表，删除列表中的失效Item**，保留待加入组件并调用原生update。 |
| 2.21 | instaticking时旁路BlockFormEvent | 未移植：框架不适用 | 旁路的是Endstone额外增加的同步事件/日志；目标没有这条事件包装。不能为了减少事件开销而拦截BDS正常固化逻辑。 |

计数复核：已实现=2.5/2.9/2.10/2.19/2.20；部分实现=2.8/2.12；其余14项未移植。其中7项是Endstone专属框架路径，5项仍需原生网络/ABI适配，1项是Linux启动器，1项是测量/工具设施。

## 第1、3—8节与中间版本

| 原文范围 | 处理情况与原因 |
| --- | --- |
| 1：公开基准 | 作为上游证据，不移作本项目成绩。100人测试和上游20 TPS不能证明当前移植的收益。 |
| 3：符号/虚表与构建依赖 | 采用LL26.51.7命名导入和HookRegistrar；内联容量/任务补丁另作精确函数边界和字节校验，不复制Endstone绝对地址。尚未移植网络/allocator依赖，也不构建Linux。编译通过不等于全部目标二进制ABI已实测。 |
| 4：验证矩阵 | 两个Windows目标构建和54项helper测试通过；0/1/100真人/机器人、资源包、地图/红石长测、split-screen、传送切维度、CPU/RSS/全帧MSPT等实服矩阵仍未完成。 |
| 5：旧memoization、seed解锁、网络旧路径 | 依照真实替代/撤销关系排除；已采用2.10的cache-cap最终方案，不恢复中间方案，也不重复注册移动过的网络hook。 |
| 6.1：树叶/液体无监听器旁路 | 未移植，原因与2.1/2.21相同：Endstone事件层成本，不是目标已有逻辑。 |
| 6.2：历史async-batch网络架构 | 未移植。是完整异步编解码/加密/strand/主线程队列/背压/关闭架构，需要完整设计与协议回归，不能与当前同步peer splice随意混合；该旧远端分支现已不在advertised refs中。 |
| 6.3：编译期优化 | 未逐项移植Endstone的头文件/Entt/模板构建调整，目标构建系统不同，且不是BDS运行时TPS优化。本轮C++23现代化属于目标自身改进，单独提交。 |
| 6.4：早期libdeflate | 未移植。与2.3的当前实现是不同网络方案；当前网络子项目本身尚未完成，不能叠加旧的每连接compressor方案。 |
| 7：视距、tick距离、客户端生成、allocator | 属于宿主配置/启动方案，不自动改服主设置；需要保持相同参数重新基准，不能算作代码移植。 |
| 8：原始资料 | 作为审计来源，结合当前clone历史核实；不是额外功能。 |

## 原文之外的新增核查

- `7049af4`生成任务上限已实现为默认关闭的实验选项`optChunkGenDispatcher`；不能与未移植的worker backlog混为一谈。
- 更新分支的`5696530`候选评分仍延期：部分构造FakePolicy、跨调用地址缓存和抽样对比不能证明目标安全。
- 原索引遗漏的ActorUniqueID hash在实际LL26.51.7已经修复；新广播路径只检查已知SDK差异，不重复修改BDS哈希。
- 踢出连接收尾、物品输入负载保护、原生packet limiter配置、RakNet已修复的重复过滤和NetherNet设置，详见[第二轮历史审计](ENDSTONE_SECOND_AUDIT.md)。未验证目标原生缺陷时不增加替代反序列化/断连流程，也不静默写宿主配置。

具体上游来源、风险和实服检查场景见[移植审计](ENDSTONE_PORTING.md)。所有七个新增优化默认关闭，PR继续保持草稿。
