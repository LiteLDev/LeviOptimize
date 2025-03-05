#pragma once

#include <memory>

namespace lo {

namespace moving_block_opt {
struct MovingBlockOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    MovingBlockOpt();
    ~MovingBlockOpt();
};
} // namespace moving_block_opt

namespace hopper_item_opt {
struct HopperItemOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    HopperItemOpt();
    ~HopperItemOpt();
};
} // namespace hopper_item_opt

namespace chunk_leak_fix {
struct ChunkLeakFix {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    ChunkLeakFix();
    ~ChunkLeakFix();
};
} // namespace chunk_leak_fix

namespace push_entity_opt {
struct Config {
    bool enable              = true;
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
};
} // namespace push_entity_opt

namespace seen_percent_opt {
struct SeenPercentOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    SeenPercentOpt();
    ~SeenPercentOpt();
};
} // namespace seen_percent_opt

namespace packet_sender_opt {
struct Config {
    bool enable           = true;
    bool multiThreadBatch = false;
};
struct PacketSenderOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(Config const&);
    PacketSenderOpt();
    ~PacketSenderOpt();
};
} // namespace packet_sender_opt

} // namespace lo
