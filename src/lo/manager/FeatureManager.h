#pragma once
#include "lo/utils/JsonUtils.h"
#include "lo/features/IFeature.h"
#include "lo/triggers/ITrigger.h"
#include <ll/api/Expected.h>
#include <ll/api/base/Containers.h>
#include <ll/api/coro/Generator.h>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace lo::manager {

class FeatureManager {
public:
    static constexpr int kConfigVersion = 1;

public:
    FeatureManager(FeatureManager const&)            = delete;
    FeatureManager& operator=(FeatureManager const&) = delete;
    FeatureManager(FeatureManager&&)                 = delete;
    FeatureManager& operator=(FeatureManager&&)      = delete;

    [[nodiscard]] static FeatureManager& getInstance();

    [[nodiscard]] ll::Expected<> load();
    [[nodiscard]] ll::Expected<> reload();
    [[nodiscard]] ll::Expected<> reload(std::string_view feature);
    [[nodiscard]] ll::Expected<> setEnabled(std::string_view feature, bool enabled);
    [[nodiscard]] bool           save() const;
    void                         enable();
    void                         disable();
    void                         shutdown();

    [[nodiscard]] ll::coro::Generator<std::string_view> featureNames() const;
    [[nodiscard]] ll::coro::Generator<std::string_view> featureNames(features::FeatureSide side) const;

    [[nodiscard]] ll::coro::Generator<std::string_view> triggerNames() const;
    [[nodiscard]] bool                                  isRunning(std::string_view feature) const noexcept;
    [[nodiscard]] bool                                  isEnabled(std::string_view feature) const noexcept;
    [[nodiscard]] bool isApplicable(std::string_view feature, features::FeatureSide side) const noexcept;

    [[nodiscard]] std::shared_ptr<features::FeatureBase> findFeature(std::string_view feature) const noexcept;

    template <class T>
    [[nodiscard]] std::shared_ptr<T> getFeature() const noexcept {
        return std::static_pointer_cast<T>(findFeature(T::getName()));
    }

    [[nodiscard]] bool enabled() const noexcept { return mEnabled.load(); }
    [[nodiscard]] bool loaded() const noexcept { return mLoaded.load(); }

    [[nodiscard]] std::filesystem::path configPath() const;

private:
    FeatureManager();
    ~FeatureManager();

    friend class features::FeatureBase;
    friend class triggers::TriggerBase;

    void applyTrigger(std::string_view trigger) noexcept;

    bool addFeature(
        std::string_view         name,
        features::FeatureSide    side,
        std::string_view         section,
        std::string_view         trigger,
        std::string_view         stopTrigger,
        features::FeatureFactory factory
    );
    bool addTrigger(std::string_view name, triggers::TriggerFactory factory);

    void createFeatures();
    void createTriggers();

    void destroyFeatures() noexcept;
    void destroyTriggers() noexcept;

    void refresh() noexcept;

    void warnMissingTriggers();

    void preserveUnknown(utils::Json& root) const;

    struct FeatureRef {
        std::string_view                       mName;
        std::string_view                       mSection;
        std::string_view                       mTrigger;
        std::string_view                       mStopTrigger;
        std::shared_ptr<features::FeatureBase> mInstance;
    };
    [[nodiscard]] std::vector<FeatureRef> featureSnapshot() const;

    [[nodiscard]] bool triggerActive(std::string_view trigger) const noexcept;

    struct FeatureSlot {
        features::FeatureSide                  mSide;
        std::string_view                       mSection;
        std::string_view                       mTrigger;
        std::string_view                       mStopTrigger;
        features::FeatureFactory               mFactory;
        std::shared_ptr<features::FeatureBase> mInstance; // null until created
    };

    struct TriggerSlot {
        triggers::TriggerFactory               mFactory;
        std::unique_ptr<triggers::TriggerBase> mInstance; // null until created, owns the listeners
    };

    ll::ConcurrentDenseMap<std::string_view, FeatureSlot> mFeatures;
    ll::ConcurrentDenseMap<std::string_view, TriggerSlot> mTriggers;
    mutable utils::Json                                   mDocument = utils::Json::object();
    std::atomic_bool                                      mLoaded{};
    std::atomic_bool                                      mEnabled{};
    std::atomic_bool                                      mBindingsChecked{};
};

} // namespace lo::manager