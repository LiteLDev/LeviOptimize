#pragma once

#include "Config.h"
#include "ll/api/Logger.h"
#include "ll/api/mod/NativeMod.h"

#include <string_view>

namespace lo {

class LeviOptimize {

public:
    LeviOptimize(ll::mod::NativeMod& self) : mSelf(self) {}

    static LeviOptimize& getInstance();

    bool load();

    bool unload();

    bool enable();

    bool disable();

    ll::Logger& getLogger() const;

    std::string const& getName() const;

    ll::mod::NativeMod& getSelf() const;

    std::filesystem::path getConfigPath() const;

    Config& getConfig();

    bool loadConfig();

    bool saveConfig();

    bool isEnabled() const;

private:
    ll::mod::NativeMod&   mSelf;
    std::optional<Config> mConfig;
};

} // namespace lo
