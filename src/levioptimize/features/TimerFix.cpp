#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/util/Timer.h"

#include <algorithm>

// namespace std {
// template <class T, class U, int = 0>
//     requires(::std::is_same_v<T, Timer>)
// MCAPI ::std::unique_ptr<T> make_unique(U&& u);
// }

namespace lo::timer_fix {
using namespace ::ll::literals;

// class FixedTimer : public Timer {
// public:
//     FixedTimer(
//         float                    ticksPerSecond,
//         ::std::function<int64()> getTimeMSCallback = []() -> int64 { return Timer::getMillisecondsSinceLaunch(); }

//     )
//     : Timer(ticksPerSecond, getTimeMSCallback) {}

//     double mLastTimeSecondsFixed = mLastMs * 0.001;
// };

// LL_STATIC_HOOK(
//     TimerCtorHook,
//     ll::memory::HookPriority::Highest,
//     // &::std::make_unique,
//     "??$make_unique@VTimer@@AEBH$0A@@std@@YA?AV?$unique_ptr@VTimer@@U?$default_delete@VTimer@@@std@@@0@AEBH@Z"_sym,
//     ::std::unique_ptr<Timer>,
//     int const& ticksPerSecond
// ) {
//     return std::make_unique<FixedTimer>((float)ticksPerSecond);
// }

// LL_TYPE_INSTANCE_HOOK(
//     TimerUpdateHook,
//     ll::memory::HookPriority::Highest,
//     FixedTimer,
//     &Timer::advanceTime,
//     void,
//     float preferredFrameStep
// ) {
//     if (mSteppingTick >= 0) {
//         if (mSteppingTick) {
//             mTicks = 1;
//             --mSteppingTick;
//         } else {
//             mTicks = 0;
//             mAlpha = 0.0f;
//         }
//         return;
//     }
//     int64 nowMs    = mGetTimeMSCallback.get()();
//     int64 passedMs = nowMs - mLastMs;
//     if (passedMs > 1000) {
//         int64 passedMsSysTime = nowMs - mLastMsSysTime;
//         if (passedMsSysTime == 0) {
//             passedMsSysTime = 1;
//             passedMs        = 1;
//         }
//         double adjustTimeT  = (double)passedMs / (double)passedMsSysTime;
//         mAdjustTime        += (float)((adjustTimeT - mAdjustTime) * 0.2);
//         mLastMs             = nowMs;
//         mLastMsSysTime      = nowMs;
//     }
//     if (passedMs < 0) {
//         mLastMs        = nowMs;
//         mLastMsSysTime = nowMs;
//     }
//     double passedSeconds  = (nowMs * 0.001 - mLastTimeSecondsFixed) * mAdjustTime; // the key modification
//     mLastTimeSecondsFixed = nowMs * 0.001;
//     mLastTimeSeconds      = (float)mLastTimeSecondsFixed;
//     if (preferredFrameStep > 0.0f) {
//         float newFrameStepAlignmentRemainder  = (float
//         )std::clamp(mFrameStepAlignmentRemainder + preferredFrameStep - passedSeconds, 0.0, 4.0 *
//         preferredFrameStep); passedSeconds                        -= mFrameStepAlignmentRemainder -
//         newFrameStepAlignmentRemainder; mFrameStepAlignmentRemainder          = newFrameStepAlignmentRemainder;
//     }
//     if (passedSeconds < 0.0) passedSeconds = 0.0;
//     if (passedSeconds > 0.1) passedSeconds = 0.1;
//     mLastTimestep  = (float)passedSeconds;
//     mPassedTime    = (float)(mPassedTime + passedSeconds * mTimeScale * mTicksPerSecond);
//     mTicks         = (int)mPassedTime;
//     mPassedTime   -= mTicks;
//     if (mTicks > 10) mTicks = 10;
//     mAlpha = mPassedTime;
// }

struct TimerFix::Impl {
    //  ll::memory::HookRegistrar<TimerCtorHook, TimerUpdateHook> r;
};

void TimerFix::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

TimerFix::TimerFix()  = default;
TimerFix::~TimerFix() = default;

} // namespace lo::timer_fix
