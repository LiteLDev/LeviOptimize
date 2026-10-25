#pragma once
#include <string_view>
#include <type_traits>

namespace lo::features::detail {

// The config type of a feature: C when the user passed one explicitly, T::Config
// when Config is nested and C is void. Only the selected specialization names
// T::Config, so an explicit C does not require a nested one.
template <class T, class C, bool = std::is_void_v<C>>
struct FeatureConfigOf {
    using type = C;
};

template <class T, class C>
struct FeatureConfigOf<T, C, true> {
    using type = typename T::Config;
};

// The config section T is stored under; commands and diagnostics use their own
// top-level objects instead of everything sharing "features".
template <class T>
[[nodiscard]] constexpr std::string_view sectionOf() noexcept {
    if constexpr (requires { T::section; }) {
        return T::section;
    } else {
        return "features";
    }
}

// The trigger T binds to for opening, or an empty view for an always-on feature.
template <class T>
[[nodiscard]] std::string_view triggerOf() noexcept {
    if constexpr (requires { typename T::Trigger; }) {
        using TriggerType = typename T::Trigger;
        return TriggerType::getName();
    } else {
        return {};
    }
}

// The trigger T binds to for closing, or an empty view when it has none.
template <class T>
[[nodiscard]] std::string_view stopTriggerOf() noexcept {
    if constexpr (requires { typename T::StopTrigger; }) {
        using TriggerType = typename T::StopTrigger;
        return TriggerType::getName();
    } else {
        return {};
    }
}

} // namespace lo::features::detail