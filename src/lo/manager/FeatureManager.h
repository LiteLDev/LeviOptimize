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

// Owns the feature and trigger objects and the configuration that belongs to the
// features.
//
// Each registration adds one slot: name, bound triggers and a factory. Features
// are constructed by load(); triggers are constructed by enable(), after every
// feature, and destroyed by disable(). A feature object is its own features::FeatureBase;
// no object is a singleton.
//
// Triggers have no configuration and no state: they are listeners that tell the
// manager "I applied" through triggers::TriggerBase::apply(). The manager reacts immediately -
// starts the features bound as Trigger, stops the ones bound as StopTrigger - and
// remembers nothing, so a reload can simply rebuild the triggers and let them
// apply the state they observe again.
//
// config.json is assembled from the registered feature slots, not from a struct,
// and every key is snake_case:
//
//   {
//       "version": 1,
//       "features": {
//           "chunk_leak_fix": { "enable": true, "max_trackers": 8 },
//           "chunk_caches_opt": true
//       },
//       "commands": { "server_feature_manage_command": true },
//       "diagnostics": { "full_frame_timing": true }
//   }
//
// A feature entry is keyed by the class name reflected from the type
// (ChunkLeakFix -> chunk_leak_fix) and contains exactly that type's Config, whose
// members are converted the same way (maxTrackers -> max_trackers).
// FeatureManager never knows a field name: each feature object generates its own
// codec from its Config, so the manager only passes the raw object to it and
// receives the raw object back. Adding a feature therefore needs no change here,
// no central list and no central config struct; removing one leaves an entry that
// is preserved verbatim.
//
// Lifecycle, driven by the mod entry points:
//
//   load()     - read config.json, construct the features, apply every feature's
//                own section, write the file back. Changes are applied in place;
//                running features are notified through their optional change
//                callback.
//   enable()   - construct the triggers (features already exist), then start the
//                features that have no trigger. A trigger that observes an
//                already-true state applies itself from its constructor.
//
// The mod calls both from its own load(), before the game starts, so features
// that hook or patch have already been installed when the first tick runs; the
// LL enable callback just calls enable() again and is a no-op.
//   reload()   - drop and rebuild the triggers, load() again and refresh.
//   disable()  - stop every feature and destroy the triggers.
//   shutdown() - disable() and destroy the features.
//
// A feature that defines "void onConfigChanged(Config const& old, Config const&
// next)" is notified after the new struct was committed and updates, field by
// field, whatever it cares about. A feature without the callback just receives
// the new struct.
//
// A feature binds its open trigger with "using Trigger = X;" and its close
// trigger with "using StopTrigger = Y;"; both are optional and independent. A
// trigger may serve as the open trigger of one feature and the close trigger of
// another. When a feature binds the same trigger as both, the close role wins.
//
// Threading: the registries are concurrent maps, so reading them from the
// client thread and the server thread at the same time is safe. The mutating
// entry points (load/save/reload/enable/disable) are expected not to overlap.
class FeatureManager {
public:
    static constexpr int kConfigVersion = 1;

public:
    FeatureManager(FeatureManager const&)            = delete;
    FeatureManager& operator=(FeatureManager const&) = delete;
    FeatureManager(FeatureManager&&)                 = delete;
    FeatureManager& operator=(FeatureManager&&)      = delete;

    [[nodiscard]] static FeatureManager& getInstance();

    // Reads config.json (an absent file means "all defaults"), constructs the
    // missing features, applies every section and writes the completed document
    // back. A section that fails to parse only fails that feature: its previous
    // value is kept, the error is logged, and the other features are still
    // applied. Returns an error only when the file itself cannot be read or is
    // not a JSON object.
    [[nodiscard]] ll::Expected<> load();

    // Rebuilds the triggers (their constructors apply the state they observe),
    // calls load() and refreshes.
    [[nodiscard]] ll::Expected<> reload();

    // Re-reads config.json and applies only this feature's section. A running
    // feature is updated in place; a stopped one follows refresh().
    [[nodiscard]] ll::Expected<> reload(std::string_view feature);

    // Forces one feature on or off: writes Config::enable, applies it, starts or
    // stops the object and saves the file.
    [[nodiscard]] ll::Expected<> setEnabled(std::string_view feature, bool enabled);

    // Writes the whole document: every feature object with its current value,
    // plus the unknown feature entries and top-level keys found in the file that
    // was loaded last.
    [[nodiscard]] bool save() const;

    // Constructs the missing features and triggers, starts the no-trigger
    // features. Idempotent.
    void enable();

    // Stops every feature and destroys the triggers. Feature objects stay alive.
    // Idempotent.
    void disable();

    // disable() and destroy the features too. Called from the mod's unload.
    void shutdown();

    [[nodiscard]] ll::coro::Generator<std::string_view> featureNames() const;

    // Only the features that exist on that side: a server-only feature is not
    // offered by the client command, and the other way around.
    [[nodiscard]] ll::coro::Generator<std::string_view> featureNames(features::FeatureSide side) const;

    [[nodiscard]] ll::coro::Generator<std::string_view> triggerNames() const;
    [[nodiscard]] bool                                  isRunning(std::string_view feature) const noexcept;
    [[nodiscard]] bool                                  isEnabled(std::string_view feature) const noexcept;
    [[nodiscard]] bool isApplicable(std::string_view feature, features::FeatureSide side) const noexcept;

    // The object FeatureManager constructed for a feature, or an empty shared_ptr
    // when it is not constructed (before load, or after shutdown). The shared_ptr
    // keeps the object alive while a hook detour still uses it.
    [[nodiscard]] std::shared_ptr<features::FeatureBase> findFeature(std::string_view feature) const noexcept;

    // Typed spelling for feature code and hook detours:
    //
    //   auto self = SeenPercentOpt::get();
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

    // The implementation behind triggers::TriggerBase::apply(): starts the features bound
    // to that trigger as Trigger and stops the ones bound to it as StopTrigger.
    // Nothing is recorded.
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

    // Constructs the slot objects that are still null.
    void createFeatures();
    void createTriggers();

    // Resets the slot objects; a trigger destructor unsubscribes its listeners.
    void destroyFeatures() noexcept;
    void destroyTriggers() noexcept;

    // Starts / stops the features that have no trigger. Features with a trigger
    // are driven by applyTrigger(); both paths still stop a feature when the
    // manager or its config disables it.
    void refresh() noexcept;

    // Warns once about features that bind a trigger name no trigger registered.
    void warnMissingTriggers();

    // Copies feature entries and top-level keys that this build does not know
    // from the last loaded document into the document about to be written.
    void preserveUnknown(utils::Json& root) const;

    // Taken under the map's shard locks and used outside them: a feature callback
    // may call back into the manager, and the same shard must not be locked twice.
    struct FeatureRef {
        std::string_view                       mName;
        std::string_view                       mSection;
        std::string_view                       mTrigger;
        std::string_view                       mStopTrigger;
        std::shared_ptr<features::FeatureBase> mInstance;
    };
    [[nodiscard]] std::vector<FeatureRef> featureSnapshot() const;

    // Whether the trigger currently holds. Features bound to a trigger start only
    // when it does, so enabling one at the wrong time leaves it stopped.
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

    // Keyed by the snake_case name reflected from the type. A client runs the
    // client and its integrated server in one process: the client command comes
    // from the client thread while triggers and the server command run on the
    // server thread, so the registries are concurrent maps.
    ll::ConcurrentDenseMap<std::string_view, FeatureSlot> mFeatures;
    ll::ConcurrentDenseMap<std::string_view, TriggerSlot> mTriggers;
    mutable utils::Json                                   mDocument = utils::Json::object();
    std::atomic_bool                                      mLoaded{};
    std::atomic_bool                                      mEnabled{};
    std::atomic_bool                                      mBindingsChecked{};
};

} // namespace lo::manager