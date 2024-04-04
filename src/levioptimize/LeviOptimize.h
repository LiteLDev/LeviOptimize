#pragma once

#include "Config.h"
#include "ll/api/Logger.h"
#include "ll/api/plugin/NativePlugin.h"

#include <string_view>

namespace lo {

class LeviOptimize {

public:
    LeviOptimize(ll::plugin::NativePlugin& self) : mSelf(self) {}

    static LeviOptimize& getInstance();

    bool load();

    bool unload();

    bool enable();

    bool disable();

    ll::Logger& getLogger() const;

    std::string const& getName() const;

    ll::plugin::NativePlugin& getSelf() const;

    std::filesystem::path getConfigPath() const;

    Config& getConfig();

    bool loadConfig();

    bool saveConfig();

    bool isEnabled() const;

private:
    ll::plugin::NativePlugin& mSelf;
    std::optional<Config>     mConfig;
};

} // namespace lo
