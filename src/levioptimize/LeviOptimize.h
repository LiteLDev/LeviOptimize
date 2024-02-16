#pragma once

#include "Config.h"
#include "ll/api/Logger.h"
#include "ll/api/plugin/NativePlugin.h"

#include <string_view>

namespace lo {

class LeviOptimize {
    LeviOptimize();

public:
    static LeviOptimize& getInstance();

    bool load(ll::plugin::NativePlugin&);
    bool unload(ll::plugin::NativePlugin&);
    bool enable(ll::plugin::NativePlugin&);
    bool disable(ll::plugin::NativePlugin&);

    ll::Logger& getLogger() const;

    std::string const& getName() const;

    ll::plugin::NativePlugin& getSelf() const;

    std::filesystem::path getConfigPath() const;

    Config& getConfig();

    bool loadConfig();

    bool saveConfig();

    bool isEnabled() const;

private:
    ll::plugin::NativePlugin* mSelf = nullptr;
    Config                    mConfig;
};

} // namespace lo
