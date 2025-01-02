#pragma once

#include "Config.h"
#include "ll/api/io/Logger.h"
#include "ll/api/mod/NativeMod.h"


namespace lo {

ll::io::Logger& getLogger();

class LeviOptimize {

public:
    LeviOptimize() : mSelf(*ll::mod::NativeMod::current()) {}

    static LeviOptimize& getInstance();

    bool load();

    bool unload();

    bool enable();

    bool disable();

    ll::io::Logger& getLogger() const;

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
