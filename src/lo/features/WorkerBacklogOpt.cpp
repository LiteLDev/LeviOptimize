// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/server_instance.cpp#L180-L191

#include "lo/features/WorkerBacklogOpt.h"
#include <mc/deps/core/threading/WorkerPool.h>
#include <ll/api/i18n/I18n.h>
#include <mc/deps/core/minecraft/threading/MinecraftWorkerPool.h>
#include <utility>

namespace lo::features {

using namespace fmt::literals;

WorkerBacklogOpt::WorkerBacklogOpt()  = default;
WorkerBacklogOpt::~WorkerBacklogOpt() = default;

ll::Expected<> WorkerBacklogOpt::onEnable() {
    auto& singleton = MinecraftWorkerPool::ASYNC();
    auto& field     = singleton->mUnk853032.as<float>();

    auto patch = ipatch::Patch::at(&field).expect<float>(0.05F).write<float>(0.0F).apply();
    if (!patch) {
        return ll::makeI18nStringError<"worker backlog: {error}">("error"_a = patch.error().format());
    }

    mPatch.emplace(std::move(*patch));
    return {};
}

void WorkerBacklogOpt::onDisable() { mPatch.reset(); }

LO_REGISTER_FEATURE(WorkerBacklogOpt);

} // namespace lo::features
