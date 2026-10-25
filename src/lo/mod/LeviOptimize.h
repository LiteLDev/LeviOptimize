#pragma once
#include <ll/api/mod/NativeMod.h>

namespace lo::mod {

class LeviOptimize {
public:
    LeviOptimize() : mSelf(*ll::mod::NativeMod::current()) {}

    static LeviOptimize& getInstance();

    ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool unload();
    bool enable();
    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace lo::mod
