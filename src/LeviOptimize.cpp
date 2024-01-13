#include "LeviOptimize.h"

#include <stdexcept>
#include <string_view>

#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/base/ErrorInfo.h"
#include "ll/api/i18n/I18nAPI.h"

namespace lo {

using namespace ll::i18n_literals;

LeviOptimize::LeviOptimize() = default;

LeviOptimize& LeviOptimize::getInstance() {
    static LeviOptimize ins{};
    return ins;
}

bool LeviOptimize::load(ll::plugin::NativePlugin& self) {
    mSelf = &self;
    ll::i18n::load(self.getPluginDir() / u8"lang");
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
    mConfig.features.fixMovingBlock = false;
    mConfig.features.fixHopperItem  = false;
    mConfig.features.optPushEntity  = false;
    mConfig.features.optRandomTick  = false;
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
    try {
        if (!ll::config::loadConfig(mConfig, getConfigPath())) {
            if (ll::config::saveConfig(mConfig, getConfigPath())) {
                getLogger().info("lo.config.rewrite.success"_tr);
            } else {
                getLogger().error("lo.config.rewrite.fail"_tr);
                return false;
            }
        }
        return true;
    } catch (...) {
        getLogger().error("lo.config.load.fail"_tr);
        ll::error_info::printCurrentException();
        return false;
    }
}

bool LeviOptimize::saveConfig() {
    bool res{};
    try {
        res = ll::config::saveConfig(mConfig, getConfigPath());
    } catch (...) {
        getLogger().error("lo.config.save.fail"_tr);
        ll::error_info::printCurrentException();
        return false;
    }
    if (!res) {
        getLogger().error("lo.config.save.fail"_tr);
        return false;
    }
    return true;
}

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
