#pragma once
#include "lo/base/Macro.h"
#include "lo/base/Reflection.h"
#include <memory>
#include <string_view>
#include <type_traits>

namespace lo::triggers {

class TriggerBase;
using TriggerFactory = std::unique_ptr<TriggerBase> (*)();

class TriggerBase {
public:
    [[nodiscard]] static bool registerTrigger(std::string_view name, TriggerFactory factory);

    TriggerBase(TriggerBase const&)            = delete;
    TriggerBase& operator=(TriggerBase const&) = delete;
    virtual ~TriggerBase()                     = default;

    void apply() noexcept;

    // Whether the condition this trigger represents currently holds. The manager
    // asks before it starts or stops a feature at runtime (for example after
    // /lo enable), so a trigger whose moment has already passed keeps its
    // features stopped instead of starting them late. Transient triggers
    // (ServerStopping, ClientLeftWorld) keep the default.
    [[nodiscard]] virtual bool active() const noexcept { return false; }

protected:
    explicit TriggerBase(std::string_view name) noexcept : mName(name) {}

private:
    std::string_view mName;
};

template <class T>
class Trigger : public TriggerBase {
public:
    Trigger(Trigger const&)            = delete;
    Trigger& operator=(Trigger const&) = delete;
    Trigger(Trigger&&)                 = delete;
    Trigger& operator=(Trigger&&)      = delete;

    virtual ~Trigger() = default;

    [[nodiscard]] static std::string_view getName() noexcept { return base::detail::reflectedName<T>(); }

    [[nodiscard]] static bool registerSelf() {
        static_assert(std::is_base_of_v<Trigger<T>, T>, "a trigger must derive from lo::Trigger<T>");
        static_assert(!std::is_abstract_v<T>, "a trigger must be a concrete type");
        static_assert(std::is_default_constructible_v<T>, "a trigger must be default constructible");

        return TriggerBase::registerTrigger(
            getName(),
            []() -> std::unique_ptr<TriggerBase> { return std::make_unique<T>(); }
        );
    }

protected:
    Trigger() noexcept : TriggerBase(getName()) {}
};

} // namespace lo::triggers

#define LO_REGISTER_TRIGGER_IMPL(counter, T)                                                                           \
    [[maybe_unused]] static bool const LO_CONCAT(lo_trigger_registration_, counter) = T::registerSelf();

#define LO_REGISTER_TRIGGER(T) LO_REGISTER_TRIGGER_IMPL(__COUNTER__, T)