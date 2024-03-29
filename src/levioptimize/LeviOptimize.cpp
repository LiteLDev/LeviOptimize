#define LL_MEMORY_OPERATORS
#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep

#include "LeviOptimize.h"

#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/utils/ErrorUtils.h"

#include <stdexcept>

namespace lo {

LeviOptimize::LeviOptimize() = default;

LeviOptimize& LeviOptimize::getInstance() {
    static LeviOptimize ins{};
    return ins;
}

bool LeviOptimize::load(ll::plugin::NativePlugin& self) {
    mSelf = &self;
    return true;
}

bool LeviOptimize::unload(ll::plugin::NativePlugin&) {
    mSelf = nullptr;
    return true;
}

bool LeviOptimize::enable(ll::plugin::NativePlugin&) { // NOLINT
    return loadConfig();
}

bool LeviOptimize::disable(ll::plugin::NativePlugin&) { // NOLINT
    saveConfig();
    mConfig.features.fixChunkLeak   = false;
    mConfig.features.fixMovingBlock = false;
    mConfig.features.fixHopperItem  = false;

    mConfig.features.optPushEntity.enable = false;

    mConfig.commands.timingCommand = false;
    return true;
}

ll::Logger& LeviOptimize::getLogger() const { return getSelf().getLogger(); }

std::string const& LeviOptimize::getName() const { return getSelf().getManifest().name; }

ll::plugin::NativePlugin& LeviOptimize::getSelf() const {
    if (!mSelf) {
        throw std::runtime_error("LeviOptimize is called before being loaded or after being unloaded");
    }
    return *mSelf;
}

std::filesystem::path LeviOptimize::getConfigPath() const { return getSelf().getConfigDir() / u8"config.json"; }

Config& LeviOptimize::getConfig() { return mConfig; }

bool LeviOptimize::loadConfig() {
    bool res{};
    try {
        res = ll::config::loadConfig(mConfig, getConfigPath());
    } catch (...) {
        ll::error_utils::printCurrentException(getLogger());
        res = false;
    }
    if (!res) {
        res = ll::config::saveConfig(mConfig, getConfigPath());
    }
    return res;
}

bool LeviOptimize::saveConfig() { return ll::config::saveConfig(mConfig, getConfigPath()); }

bool LeviOptimize::isEnabled() const { return getSelf().getState() == ll::plugin::Plugin::State::Enabled; }

extern "C" {
_declspec(dllexport) bool ll_plugin_load(ll::plugin::NativePlugin& self) {
    return LeviOptimize::getInstance().load(self);
}

_declspec(dllexport) bool ll_plugin_unload(ll::plugin::NativePlugin& self) {
    return LeviOptimize::getInstance().unload(self);
}

_declspec(dllexport) bool ll_plugin_enable(ll::plugin::NativePlugin& self) {
    return LeviOptimize::getInstance().enable(self);
}

_declspec(dllexport) bool ll_plugin_disable(ll::plugin::NativePlugin& self) {
    return LeviOptimize::getInstance().disable(self);
}
}
} // namespace lo
