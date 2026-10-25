#pragma once
#include <concepts>
#include <type_traits>

namespace lo::base {

// What a feature's config can be: a plain bool (the whole config is the enable
// flag), or an aggregate with a bool enable member plus optional fields.
template <class Config>
concept FeatureConfig =
    std::copyable<std::remove_cvref_t<Config>>
    && (std::is_same_v<std::remove_cvref_t<Config>, bool>
        || (std::is_aggregate_v<std::remove_cvref_t<Config>>
            && requires(std::remove_cvref_t<Config> const& config) {
                   { config.enable } -> std::convertible_to<bool>;
               }));

// Optional change callback: a feature that wants to react to a reload defines it
// and compares the two configs; a feature that does not is simply left with the
// new value. It has to be public, because the detection happens outside the
// class.
template <class T, class Config>
concept HasConfigHook = requires(T& self, Config const& oldConfig, Config const& nextConfig) {
    self.onConfigChanged(oldConfig, nextConfig);
};

} // namespace lo::base
