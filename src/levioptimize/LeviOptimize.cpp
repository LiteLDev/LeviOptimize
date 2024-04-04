#define LL_MEMORY_OPERATORS
#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

#include "LeviOptimize.h"

#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/plugin/RegisterHelper.h"
#include "ll/api/utils/ErrorUtils.h"

#include <stdexcept>

namespace lo {
namespace command {
extern void registerTimingCommand();
}
static std::unique_ptr<LeviOptimize> instance;

LeviOptimize& LeviOptimize::getInstance() { return *instance; }

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

ll::Logger& LeviOptimize::getLogger() const { return getSelf().getLogger(); }

std::string const& LeviOptimize::getName() const { return getSelf().getManifest().name; }

ll::plugin::NativePlugin& LeviOptimize::getSelf() const { return mSelf; }

std::filesystem::path LeviOptimize::getConfigPath() const { return getSelf().getConfigDir() / u8"config.json"; }

Config& LeviOptimize::getConfig() { return mConfig.value(); }

bool LeviOptimize::loadConfig() {
    bool res{};
    mConfig.emplace();
    try {
        res = ll::config::loadConfig(*mConfig, getConfigPath());
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

LL_REGISTER_PLUGIN(lo::LeviOptimize, lo::instance);
