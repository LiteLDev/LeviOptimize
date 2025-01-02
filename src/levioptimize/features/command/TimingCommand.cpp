// #include <algorithm>
// #include <chrono>
// #include <cstddef>
// #include <mutex>
// #include <string>
// #include <thread>

// #include "ll/api/io/Logger.h"
// #include "ll/api/io/LoggerRegistry.h"
// #include "ll/api/base/StdInt.h"
// #include "ll/api/command/CommandHandle.h"
// #include "ll/api/command/CommandRegistrar.h"
// #include "ll/api/thread/TickSyncSleep.h"
// #include "mc/entity/systems/DefaultEntitySystemsCollection.h"
// #include "mc/server/commands/CommandFlag.h"
// #include "mc/server/commands/CommandOrigin.h"
// #include "mc/server/commands/CommandOutput.h"
// #include "mc/server/commands/CommandPermissionLevel.h"

// #include "ll/api/service/Bedrock.h"
// #include "mc/world/level/Level.h"

// #include "ll/api/service/Bedrock.h"
// #include "mc/entity/systems/EntitySystems.h"
// #include "mc/world/level/Level.h"
// #include <ranges>
// #include <vector>

// #include "ll/api/chrono/GameChrono.h"
// #include "parallel_hashmap/phmap.h"

namespace lo::command {

void registerTimingCommand() {}
} // namespace lo::command
  //     auto logger = ll::io::LoggerRegistry::getInstance().getOrCreate("timing");

//     constexpr static size_t  counttick = 100;
//     static std::atomic<bool> isCommandRunning(false);
//     auto&                    cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
//         "timing",
//         "Obtain server TPS and ECS counts.",
//         CommandPermissionLevel::Host,
//         CommandFlagValue::None
//     );
//     cmd.overload().execute([logger](CommandOrigin const&, CommandOutput&) {
//         bool expected = false;
//         if (!isCommandRunning.compare_exchange_strong(expected, true)) {
//             logger->warn("Command is already running. Execution aborted.");
//             return;
//         }

//         auto thread = std::thread([logger] {
//             auto& system = ll::service::getLevel()->getEntitySystems();

//             auto& collection = system.mSystems;
//             {
//                 std::lock_guard lock(collection.mTimingMutex);
//                 system.mEnableTimingCapture = true;
//                 logger->warn("EnableTimingCapture");
//             }

//             phmap::parallel_flat_hash_map<uint, DefaultEntitySystemsCollection::ECSTiming> timings{};
//             using namespace ll::chrono;
//             using namespace ll::chrono_literals;
//             ll::thread::TickSyncSleep<GameTickClock> sleeper;
//             auto                                     begin = std::chrono::steady_clock::now();
//             for (size_t i = 0; i < counttick; i++) {
//                 sleeper.sleepFor(1_tick);
//                 {
//                     std::lock_guard lock(collection.mTimingMutex);
//                     for (auto& collectCategory : collection.mTickingSystemCategories) {
//                         auto& tickTimings = collectCategory.mTimings;
//                         for (size_t j = 0; j < tickTimings.size(); j++) {
//                             auto& timing    = timings[collectCategory.mSystems.at(j)];
//                             timing.mCount  += tickTimings.at(j).mCount;
//                             timing.mMsTime += tickTimings.at(j).mMsTime;
//                         }
//                     }
//                 }
//             }
//             auto end = std::chrono::steady_clock::now();
//             {
//                 std::lock_guard lock(collection.mTimingMutex);
//                 system.mEnableTimingCapture = false;
//             }
//             struct TimingData {
//                 uint   id;
//                 double avg;
//                 uint   count;
//             };

//             std::vector<TimingData> orderdTiming;
//             orderdTiming.reserve(timings.size());
//             double allTime = 0.0;
//             for (auto& [systemId, timing] : timings) {
//                 orderdTiming.emplace_back(systemId, double(timing.mMsTime) / counttick, timing.mCount);
//                 allTime += double(timing.mMsTime) / counttick;
//             }

//             std::ranges::sort(orderdTiming, [](TimingData const& a, TimingData const& b) { return a.avg > b.avg; });

//             logger.warn("TPS: {:.5f}", double(counttick) / std::chrono::duration<double>(end - begin).count());
//             logger.warn("{} ECS cost {:.5f}ms per tick", timings.size(), allTime);

//             for (size_t i = 0; i < orderdTiming.size() /*&& i < 20*/; i++) {
//                 auto& data = orderdTiming[i];
//                 logger.warn(
//                     "  | {:.5f}ms {} for {:0>3} {}",
//                     data.avg,
//                     double(data.count) / counttick,
//                     data.id,
//                     collection.mAllSystemsInfo[data.id].mName
//                 );
//             }
//             isCommandRunning = false;
//         });
//         thread.detach();
//     });
// }
// } // namespace lo::command