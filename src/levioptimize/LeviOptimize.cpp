#define LL_MEMORY_OPERATORS
#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

#include "LeviOptimize.h"

#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/ErrorUtils.h"


namespace lo {
namespace command {
extern void registerTimingCommand();
}

LeviOptimize& LeviOptimize::getInstance() {
    static LeviOptimize instance;
    return instance;
}

bool LeviOptimize::load() { return loadConfig(); }

bool LeviOptimize::unload() { return true; }

bool LeviOptimize::enable() { // NOLINT
    if (!mConfig && !loadConfig()) {
        return false;
    }
    if (getConfig().commands.timingCommand) {
        command::registerTimingCommand();
    }
    return true;
}

bool LeviOptimize::disable() { // NOLINT
    saveConfig();
    mConfig.reset();
    return true;
}

ll::io::Logger& LeviOptimize::getLogger() const { return getSelf().getLogger(); }

ll::io::Logger& getLogger() { return LeviOptimize::getInstance().getLogger(); }

std::string const& LeviOptimize::getName() const { return getSelf().getManifest().name; }

ll::mod::NativeMod& LeviOptimize::getSelf() const { return mSelf; }

std::filesystem::path LeviOptimize::getConfigPath() const { return getSelf().getConfigDir() / u8"config.json"; }

Config& LeviOptimize::getConfig() { return mConfig.value(); }

bool LeviOptimize::loadConfig() {
    bool res{};
    mConfig.emplace();
    try {
        res = ll::config::loadConfig(*mConfig, getConfigPath());
        if (mConfig->features.optItemStackUserData.storage && !mConfig->features.patchInventoryTransaction.storage) {
            mConfig->features.patchInventoryTransaction = true;
        }
    } catch (...) {
        ll::error_utils::printCurrentException(getLogger());
        res = false;
    }
    if (!res) {
        res = ll::config::saveConfig(*mConfig, getConfigPath());
    }
    return res;
}

bool LeviOptimize::saveConfig() { return ll::config::saveConfig(mConfig.value(), getConfigPath()); }

bool LeviOptimize::isEnabled() const { return getSelf().isEnabled(); }
} // namespace lo

LL_REGISTER_MOD(lo::LeviOptimize, lo::LeviOptimize::getInstance());
