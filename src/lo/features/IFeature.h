#pragma once
#include "lo/base/Concepts.h"
#include "lo/base/Macro.h"
#include "lo/base/Reflection.h"
#include "lo/utils/JsonUtils.h"
#include "lo/features/FeatureTraits.h"
#include <ll/api/base/StdInt.h>
#include <ll/api/Expected.h>
#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace lo::features {

enum class FeatureSide : uint8 {
    server,
    client,
    both,
};

class FeatureBase;

using FeatureFactory = std::unique_ptr<FeatureBase> (*)();

class FeatureBase {
public:
    [[nodiscard]] static bool registerFeature(
        std::string_view name,
        FeatureSide      side,
        std::string_view section,
        std::string_view trigger,
        std::string_view stopTrigger,
        FeatureFactory   factory
    );

    using SaveFn    = ll::Expected<utils::Json> (*)(FeatureBase const&);
    using LoadFn    = ll::Expected<> (*)(FeatureBase&, utils::Json const&);
    using EnabledFn = bool (*)(FeatureBase const&);
    using ClearFn   = void (*)(FeatureBase&) noexcept;

    FeatureBase(FeatureBase const&)            = delete;
    FeatureBase& operator=(FeatureBase const&) = delete;
    virtual ~FeatureBase()                     = default;

    [[nodiscard]] static std::shared_ptr<FeatureBase> find(std::string_view name) noexcept;

    [[nodiscard]] bool running() const noexcept { return mRunning; }
    [[nodiscard]] bool enabled() const noexcept { return mEnabled(*this); }

    [[nodiscard]] ll::Expected<utils::Json> saveConfig() const { return mSave(*this); }
    [[nodiscard]] ll::Expected<>            loadConfig(utils::Json const& section) { return mLoad(*this, section); }

    void clearInstanceCache() noexcept {
        if (mClear) mClear(*this);
    }

    [[nodiscard]] virtual ll::Expected<> start()         = 0;
    virtual void                         stop() noexcept = 0;

protected:
    FeatureBase() = default;

    void bindCodec(SaveFn save, LoadFn load, EnabledFn enabled, ClearFn clear) noexcept {
        mSave    = save;
        mLoad    = load;
        mEnabled = enabled;
        mClear   = clear;
    }

    bool mRunning{};

private:
    SaveFn    mSave{};
    LoadFn    mLoad{};
    EnabledFn mEnabled{};
    ClearFn   mClear{};
};

template <class T, class C = void>
class Feature : public FeatureBase {
public:
    Feature(Feature const&)            = delete;
    Feature& operator=(Feature const&) = delete;
    Feature(Feature&&)                 = delete;
    Feature& operator=(Feature&&)      = delete;

    virtual ~Feature() = default;

    [[nodiscard]] static std::string_view getName() noexcept { return base::detail::reflectedName<T>(); }

    [[nodiscard]] static T* get() noexcept { return sInstance.load(std::memory_order_acquire); }

    [[nodiscard]] static std::shared_ptr<T> getShared() noexcept {
        return std::static_pointer_cast<T>(find(T::getName()));
    }

    template <class D = T>
    [[nodiscard]] auto const& getConfig() const noexcept {
        if constexpr (std::is_void_v<C>) {
            return *static_cast<typename D::Config const*>(mStorage.get());
        } else {
            return mStorage;
        }
    }

    [[nodiscard]] ll::Expected<> start() override {
        if (mRunning) return {};
        if (auto result = onEnable(); !result) {
            onDisable();
            return result;
        }
        mRunning = true;
        return {};
    }

    void stop() noexcept override {
        if (!mRunning) return;
        onDisable();
        mRunning = false;
    }


    [[nodiscard]] static bool registerSelf() {
        using Config = ConfigType<T>;

        static_assert(std::is_base_of_v<Feature, T>, "a feature must derive from lo::features::Feature<T>");
        static_assert(!std::is_abstract_v<T>, "a feature must implement onEnable() and onDisable()");
        static_assert(std::is_default_constructible_v<T>, "a feature must be default constructible");
        static_assert(
            base::FeatureConfig<Config>,
            "the feature config must be bool or an aggregate with a bool enable field"
        );

        return FeatureBase::registerFeature(
            getName(),
            [] {
                if constexpr (requires { T::side; }) {
                    return T::side;
                } else {
                    return FeatureSide::both;
                }
            }(),
            detail::sectionOf<T>(),
            detail::triggerOf<T>(),
            detail::stopTriggerOf<T>(),
            []() -> std::unique_ptr<FeatureBase> {
                auto instance = std::make_unique<T>();
                sInstance.store(instance.get(), std::memory_order_release);
                return instance;
            }
        );
    }

protected:
    Feature() {
        if constexpr (std::is_void_v<C>) {
            if constexpr (std::is_same_v<typename T::Config, bool>) {
                mStorage = std::make_shared<bool>(true);
            } else {
                mStorage = std::make_shared<typename T::Config>();
            }
        }
        bindCodec(&Feature::saveImpl, &Feature::loadImpl, &Feature::enabledImpl, &Feature::clearImpl);
    }

    virtual ll::Expected<> onEnable()  = 0;
    virtual void           onDisable() = 0;

private:
    using Storage = std::conditional_t<std::is_void_v<C>, std::shared_ptr<void>, C>;

    inline static std::atomic<T*> sInstance{};

    static void clearImpl(FeatureBase& base) noexcept {
        auto* expected = static_cast<T*>(&base);
        sInstance.compare_exchange_strong(expected, nullptr, std::memory_order_release, std::memory_order_relaxed);
    }

    template <class D = T>
    using ConfigType = typename detail::FeatureConfigOf<D, C>::type;

    template <class D = T>
    [[nodiscard]] static ConfigType<D>& configOf(Storage& storage) noexcept {
        if constexpr (std::is_void_v<C>) {
            return *static_cast<typename D::Config*>(storage.get());
        } else {
            return storage;
        }
    }

    template <class D = T>
    [[nodiscard]] static ConfigType<D> const& configOf(Storage const& storage) noexcept {
        if constexpr (std::is_void_v<C>) {
            return *static_cast<typename D::Config const*>(storage.get());
        } else {
            return storage;
        }
    }

    static ll::Expected<utils::Json> saveImpl(FeatureBase const& base) {
        auto const& config = configOf(static_cast<Feature const&>(base).mStorage);
        if constexpr (std::is_same_v<ConfigType<T>, bool>) {
            return utils::Json(config);
        } else {
            return utils::toJson(config);
        }
    }

    static ll::Expected<> loadImpl(FeatureBase& base, utils::Json const& section) {
        using Config = ConfigType<T>;
        auto& self   = static_cast<Feature&>(base);

        auto& current = configOf(self.mStorage);

        if constexpr (std::is_same_v<Config, bool>) {
            auto next = current;
            if (section.is_boolean()) {
                next = section.get<bool>();
            } else if (section.is_object()) {
                if (auto it = section.find("enable"); it != section.end() && it->is_boolean()) {
                    next = it->get<bool>();
                }
            }
            auto previous = current;
            current       = next;

            if (!self.running() || !current || previous == current) return {};
            if constexpr (base::HasConfigHook<T, Config>) {
                static_cast<T&>(self).onConfigChanged(previous, current);
            }
            return {};
        } else {
            auto defaultsJson = utils::toJson(Config{});
            if (!defaultsJson) return ll::forwardError(defaultsJson.error());

            auto sectionJson = section;
            if (section.is_boolean()) {
                sectionJson           = utils::Json::object();
                sectionJson["enable"] = section.get<bool>();
            }

            auto next = utils::fromJson<Config>(utils::merge(*defaultsJson, sectionJson));
            if (!next) return ll::forwardError(next.error());

            Config previous = std::move(current);
            current         = std::move(*next);

            if (!self.running() || !current.enable) return {};

            auto previousJson = utils::toJson(previous);
            auto currentJson  = utils::toJson(current);
            if (previousJson && currentJson && *previousJson == *currentJson) return {};

            if constexpr (base::HasConfigHook<T, Config>) {
                static_cast<T&>(self).onConfigChanged(previous, current);
            }
            return {};
        }
    }

    static bool enabledImpl(FeatureBase const& base) noexcept {
        auto const& config = configOf(static_cast<Feature const&>(base).mStorage);
        if constexpr (std::is_same_v<ConfigType<T>, bool>) {
            return config;
        } else {
            return config.enable;
        }
    }

    Storage mStorage{};
};

} // namespace lo::features

#define LO_REGISTER_FEATURE_IMPL(counter, T)                                                                           \
    [[maybe_unused]] static bool const LO_CONCAT(lo_feature_registration_, counter) = T::registerSelf();

#define LO_REGISTER_FEATURE(T) LO_REGISTER_FEATURE_IMPL(__COUNTER__, T)