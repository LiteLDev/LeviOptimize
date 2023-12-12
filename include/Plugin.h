#pragma once

#include <string_view>

#include <ll/api/Logger.h>
#include <ll/api/plugin/Plugin.h>

namespace plugins {

class Plugin {
public:
    Plugin() = default;

    ~Plugin() = default;

    /// @brief Loads the plugin.
    /// @param self The plugin handle.
    /// @return True if the plugin was loaded successfully.
    bool load(ll::plugin::Plugin& self);

    /// @brief Unloads the plugin.
    /// @param self The plugin handle.
    /// @return True if the plugin was unloaded successfully.
    bool unload(ll::plugin::Plugin& self);

    /// @brief Enables the plugin.
    /// @param self The plugin handle.
    /// @return True if the plugin was enabled successfully.
    bool enable(ll::plugin::Plugin& self);

    /// @brief Disables the plugin.
    /// @param self The plugin handle.
    /// @return True if the plugin was disabled successfully.
    bool disable(ll::plugin::Plugin& self);

    /// @brief Gets the logger.
    /// @return The logger.
    ll::Logger const& getLogger() const;

    /// @brief Gets the plugin name.
    /// @return The plugin name.
    std::string_view getName() const;

    /// @brief Gets the LeviLamina plugin instance.
    /// @return The LeviLamina plugin instance.
    ll::plugin::Plugin& getSelf() const;

    /// @brief Checks if the plugin is enabled.
    /// @return True if the plugin is enabled.
    bool isEnabled() const;

    // More plugin methods here...

private:
    bool mIsEnabled = false;
    ll::plugin::Plugin* mSelf = nullptr;
};

} // namespace plugins
