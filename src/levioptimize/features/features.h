#pragma once
#include <memory>

namespace lo {

namespace moving_block_fix {
struct MovingBlockFix {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    MovingBlockFix();
    ~MovingBlockFix();
};
} // namespace moving_block_fix

namespace hopper_item_fix {
struct HopperItemFix {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
    HopperItemFix();
    ~HopperItemFix();
};
} // namespace hopper_item_fix

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
struct PushEntityOpt {
    struct Impl;
    std::unique_ptr<Impl> impl;

    void call(bool);
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

} // namespace lo
