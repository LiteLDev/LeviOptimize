#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>

namespace lo::diagnostics::detail {

struct TimingSnapshot {
    size_t mSamples{};
    double mCurrentMspt{};
    double mAverageMspt{};
    double mCurrentTps{};
    double mAverageTps{};
};

class FullFrameTimingState {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static constexpr size_t Window = 20;

private:
    std::array<double, Window> mSamples{};
    size_t                mCount{};
    size_t                mNext{};
    double                     mCurrent{};
    std::optional<TimePoint>   mStart;
    bool                       mTickPending{};

    void endPending(TimePoint now) {
        if (!mStart || !mTickPending) return;
        if (now >= *mStart) {
            mCurrent        = std::chrono::duration<double, std::milli>(now - *mStart).count();
            mSamples[mNext] = mCurrent;
            mNext           = (mNext + 1) % Window;
            mCount          = std::min(mCount + 1, Window);
        }
        mStart       = now;
        mTickPending = false;
    }

public:
    // Resetting a missing/aborted frame never turns its idle gap into a sample.
    void beginFrame(TimePoint now) noexcept {
        mStart       = now;
        mTickPending = false;
    }

    // A catch-up frame may run multiple Level ticks. The next tick's start
    // finishes the previous sample; the first includes pre-Level network work.
    void beginTick(TimePoint now) { endPending(now); }
    void completeTick() noexcept { mTickPending = mStart.has_value(); }
    void endFrame(TimePoint now) {
        endPending(now);
        cancelFrame();
    }
    void cancelFrame() noexcept {
        mStart.reset();
        mTickPending = false;
    }
    void reset() noexcept { *this = {}; }

    [[nodiscard]] static constexpr double ticksPerSecond(double mspt) noexcept {
        return std::min(20.0, 1000.0 / std::max(1.0, mspt));
    }

    [[nodiscard]] TimingSnapshot snapshot() const noexcept {
        TimingSnapshot result{.mSamples = mCount};
        if (mCount == 0) return result;

        result.mCurrentMspt = mCurrent;
        result.mCurrentTps  = ticksPerSecond(mCurrent);
        for (size_t index = 0; index < mCount; ++index) {
            result.mAverageMspt += mSamples[index];
            result.mAverageTps  += ticksPerSecond(mSamples[index]);
        }
        result.mAverageMspt /= static_cast<double>(mCount);
        result.mAverageTps  /= static_cast<double>(mCount);
        return result;
    }
};

} // namespace lo::diagnostics::detail
