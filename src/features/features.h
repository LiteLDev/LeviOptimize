#pragma once

#include <memory>

namespace lo {

namespace chunk_caches_opt {
struct ChunkCachesOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    ChunkCachesOpt();
    ~ChunkCachesOpt();
    ChunkCachesOpt(ChunkCachesOpt&&) noexcept;
    ChunkCachesOpt& operator=(ChunkCachesOpt&&) noexcept;
};
} // namespace chunk_caches_opt

namespace player_networking_opt {
struct PlayerNetworkingOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    PlayerNetworkingOpt();
    ~PlayerNetworkingOpt();
    PlayerNetworkingOpt(PlayerNetworkingOpt&&) noexcept;
    PlayerNetworkingOpt& operator=(PlayerNetworkingOpt&&) noexcept;
};
} // namespace player_networking_opt

namespace random_chunk_opt {
struct RandomChunkOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    RandomChunkOpt();
    ~RandomChunkOpt();
    RandomChunkOpt(RandomChunkOpt&&) noexcept;
    RandomChunkOpt& operator=(RandomChunkOpt&&) noexcept;
};
} // namespace random_chunk_opt

namespace circuit_scene_graph_opt {
struct CircuitSceneGraphOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    CircuitSceneGraphOpt();
    ~CircuitSceneGraphOpt();
    CircuitSceneGraphOpt(CircuitSceneGraphOpt&&) noexcept;
    CircuitSceneGraphOpt& operator=(CircuitSceneGraphOpt&&) noexcept;
};
} // namespace circuit_scene_graph_opt

namespace chunk_gen_dispatcher_opt {
struct ChunkGenDispatcherOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    ChunkGenDispatcherOpt();
    ~ChunkGenDispatcherOpt();
    ChunkGenDispatcherOpt(ChunkGenDispatcherOpt&&) noexcept;
    ChunkGenDispatcherOpt& operator=(ChunkGenDispatcherOpt&&) noexcept;
};
} // namespace chunk_gen_dispatcher_opt

namespace moving_block_opt {
struct MovingBlockOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    MovingBlockOpt();
    ~MovingBlockOpt();
    MovingBlockOpt(MovingBlockOpt&&) noexcept;
    MovingBlockOpt& operator=(MovingBlockOpt&&) noexcept;
};
} // namespace moving_block_opt

namespace chunk_leak_fix {
struct ChunkLeakFix {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    ChunkLeakFix();
    ~ChunkLeakFix();
    ChunkLeakFix(ChunkLeakFix&&) noexcept;
    ChunkLeakFix& operator=(ChunkLeakFix&&) noexcept;
};
} // namespace chunk_leak_fix

namespace push_entity_opt {
struct Config {
    bool enable              = false;
    bool disableVec0Push     = true;
    int  maxPushTimes        = -1;
    bool unlimitedPlayerPush = false;
};

struct PushEntityOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(Config const&);
    PushEntityOpt();
    ~PushEntityOpt();
    PushEntityOpt(PushEntityOpt&&) noexcept;
    PushEntityOpt& operator=(PushEntityOpt&&) noexcept;
};
} // namespace push_entity_opt

namespace seen_percent_opt {
struct SeenPercentOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    SeenPercentOpt();
    ~SeenPercentOpt();
    SeenPercentOpt(SeenPercentOpt&&) noexcept;
    SeenPercentOpt& operator=(SeenPercentOpt&&) noexcept;
};
} // namespace seen_percent_opt
} // namespace lo
