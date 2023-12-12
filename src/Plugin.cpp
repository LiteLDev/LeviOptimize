#include <Plugin.h>

#include <stdexcept>
#include <string_view>

#include <ll/api/plugin/Plugin.h>

namespace plugins {

bool Plugin::load(ll::plugin::Plugin& self) {
    if (this->mSelf != nullptr) {
        throw std::runtime_error("plugin is loaded twice");
    }

    this->mSelf = &self;

    // Code for loading the plugin goes here.

    return true;
}

bool Plugin::unload(ll::plugin::Plugin& self) {
    if (this->mSelf == nullptr) {
        throw std::runtime_error("plugin is unloaded twice");
    }
    if (this->mSelf != &self) {
        throw std::runtime_error("plugin is unloaded by a different instance");
    }

    // Code for unloading the plugin goes here.

    this->mSelf = nullptr;
    return true;
}

bool Plugin::enable(ll::plugin::Plugin& self) {
    if (this->mSelf == nullptr) {
        throw std::runtime_error("plugin is enabled before being loaded or after being unloaded");
    }
    if (this->mSelf != &self) {
        throw std::runtime_error("plugin is enabled by a different instance");
    }

    // Code for enabling the plugin goes here.

    this->mIsEnabled = true;
    return true;
}

bool Plugin::disable(ll::plugin::Plugin& self) {
    if (this->mSelf == nullptr) {
        throw std::runtime_error("plugin is disabled before being loaded or after being unloaded");
    }
    if (this->mSelf != &self) {
        throw std::runtime_error("plugin is disabled by a different instance");
    }

    // Code for disabling the plugin goes here.

    this->mIsEnabled = false;
    return true;
}

ll::Logger const& Plugin::getLogger() const { return this->getSelf().getLogger(); }

std::string_view Plugin::getName() const { return this->getSelf().getManifest().name; }

ll::plugin::Plugin& Plugin::getSelf() const {
    if (this->mSelf == nullptr) {
        throw std::runtime_error("plugin is called before being loaded or after being unloaded");
    }

    return *this->mSelf;
}

bool Plugin::isEnabled() const { return this->mIsEnabled; }

} // namespace plugins
