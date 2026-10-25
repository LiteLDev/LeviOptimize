#include "lo/mod/LeviOptimize.h"
#include "lo/manager/FeatureManager.h"
#include <ll/api/i18n/I18n.h>
#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>

namespace lo::mod {

using namespace fmt::literals;
using namespace ll::i18n_literals;

LeviOptimize& LeviOptimize::getInstance() {
    static LeviOptimize instance;
    return instance;
}

bool LeviOptimize::load() {
    auto& featureManager = manager::FeatureManager::getInstance();

    if (auto result = featureManager.load(); !result) {
        getSelf().getLogger().error(
            "failed to load the configuration: {error}"_tr("error"_a = result.error().message())
        );
        return false;
    }

    // Enable here, not in the LL enable callback: features that install hooks or
    // patch memory have to be running before the game starts. The callback stays
    // idempotent for a later enable/disable cycle.
    featureManager.enable();
    return true;
}

bool LeviOptimize::enable() {
    manager::FeatureManager::getInstance().enable();
    return true;
}

bool LeviOptimize::disable() {
    manager::FeatureManager::getInstance().disable();
    return true;
}

bool LeviOptimize::unload() {
    manager::FeatureManager::getInstance().shutdown();
    return true;
}

} // namespace lo::mod

LL_REGISTER_MOD(lo::mod::LeviOptimize, lo::mod::LeviOptimize::getInstance());