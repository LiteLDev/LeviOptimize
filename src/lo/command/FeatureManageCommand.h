#pragma once
#include "lo/diagnostics/FullFrameTiming.h"
#include "lo/features/IFeature.h"
#include "lo/manager/FeatureManager.h"
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/command/CommandHandle.h>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace lo::command {

class ServerFeatureManageCommand;
#ifdef LL_PLAT_C
class ClientFeatureManageCommand;
#endif

template <class T>
struct CommandInfo;

template <>
struct CommandInfo<ServerFeatureManageCommand> {
    static constexpr bool                   isClientSide{false};
    static constexpr features::FeatureSide            featureSide{features::FeatureSide::server};
    static constexpr std::string_view       name{"lo"};
    static constexpr std::string_view       description{"Manage LeviOptimize features"};
    static constexpr CommandPermissionLevel requirement{CommandPermissionLevel::GameDirectors};

    enum class FeatureNames;
};

#ifdef LL_PLAT_C
template <>
struct CommandInfo<ClientFeatureManageCommand> {
    static constexpr bool                   isClientSide{true};
    static constexpr features::FeatureSide            featureSide{features::FeatureSide::client};
    static constexpr std::string_view       name{"clilo"};
    static constexpr std::string_view       description{"Manage LeviOptimize features"};
    static constexpr CommandPermissionLevel requirement{CommandPermissionLevel::Any};

    enum class FeatureNames;
};
#endif

struct ListActionParams {
    enum class ListAction { List, Triggers, ReloadAll, Profile } action;
};

template <class T>
struct FeatureParams {
    enum class FeatureAction { Reload, Enable, Disable } action;
    ll::command::SoftEnum<typename CommandInfo<T>::FeatureNames> feature;
};

template <class T>
class FeatureManageCommand : public features::Feature<T> {
public:
    static constexpr features::FeatureSide side = CommandInfo<T>::featureSide;
    static constexpr std::string_view      section{"commands"};
    using Config = bool;

protected:
    ll::Expected<> onEnable() override {
        registerCommand();
        return {};
    }
    void onDisable() override {}

    static void registerCommand() {
        using Info = CommandInfo<T>;

        auto& registrar = ll::command::CommandRegistrar::getInstance(Info::isClientSide);

        registrar.tryRegisterSoftEnum(
            std::string{ll::command::enum_name_v<typename Info::FeatureNames>},
            manager::FeatureManager::getInstance().featureNames(Info::featureSide) | std::ranges::to<std::vector<std::string>>()
        );

        auto& command = registrar.getOrCreateCommand(
            std::string{Info::name},
            std::string{ll::i18n::getInstance().get(Info::description, {})},
            Info::requirement
        );

        command.template overload<ListActionParams>().required("action").execute([](CommandOrigin const&    origin,
                                                                                    CommandOutput&          output,
                                                                                    ListActionParams const& param) {
            using namespace fmt::literals;
            using namespace ll::i18n_literals;

            auto& manager = manager::FeatureManager::getInstance();

            switch (param.action) {
            case ListActionParams::ListAction::List:
                for (auto name : manager.featureNames(Info::featureSide)) {
                    output.success(
                        "feature ({feature}): {state}, config {config}"_trl(
                            origin.getLocaleCode(),
                            "feature"_a = name,
                            "state"_a   = manager.isRunning(name) ? "running" : "stopped",
                            "config"_a  = manager.isEnabled(name) ? "enabled" : "disabled"
                        )
                    );
                }
                return;

            case ListActionParams::ListAction::Profile: {
                auto timing = manager.getFeature<diagnostics::FullFrameTiming>();
                if (!timing) {
                    output.error("full-frame timing is not available"_trl(origin.getLocaleCode()));
                    return;
                }
                auto snapshot = timing->snapshot();
                if (snapshot.mSamples == 0) {
                    output.success("full-frame timing: no samples yet"_trl(origin.getLocaleCode()));
                    return;
                }
                output.success(
                    "full-frame timing: {samples} samples, current {current} ms ({currentTps} tps), average {average} ms ({averageTps} tps)"_trl(
                        origin.getLocaleCode(),
                        "samples"_a    = snapshot.mSamples,
                        "current"_a    = snapshot.mCurrentMspt,
                        "currentTps"_a = snapshot.mCurrentTps,
                        "average"_a    = snapshot.mAverageMspt,
                        "averageTps"_a = snapshot.mAverageTps
                    )
                );
                return;
            }

            case ListActionParams::ListAction::Triggers:
                for (auto name : manager.triggerNames()) {
                    output.success("trigger ({trigger})"_trl(origin.getLocaleCode(), "trigger"_a = name));
                }
                return;

            case ListActionParams::ListAction::ReloadAll:
                if (auto result = manager.reload(); !result) {
                    output.error(
                        "failed to reload: {error}"_trl(origin.getLocaleCode(), "error"_a = result.error().message())
                    );
                    result.error().log(output, origin.getLocaleCode());
                    return;
                }

                output.success("reloaded the whole configuration"_trl(origin.getLocaleCode()));
                return;
            }

            LL_UNREACHABLE;
        });

        command.template overload<FeatureParams<T>>().required("action").required("feature").execute(
            [](CommandOrigin const& origin, CommandOutput& output, FeatureParams<T> const& param) {
                using namespace fmt::literals;
                using namespace ll::i18n_literals;

                auto& manager = manager::FeatureManager::getInstance();

                if (!manager.isApplicable(param.feature, Info::featureSide)) {
                    output.error(
                        "feature ({feature}) is not available on this side"_trl(
                            origin.getLocaleCode(),
                            "feature"_a = param.feature
                        )
                    );
                    return;
                }

                switch (param.action) {
                case FeatureParams<T>::FeatureAction::Reload:
                    if (auto result = manager.reload(param.feature); !result) {
                        output.error(
                            "failed to reload feature ({feature}): {error}"_trl(
                                origin.getLocaleCode(),
                                "feature"_a = param.feature,
                                "error"_a   = result.error().message()
                            )
                        );
                        result.error().log(output, origin.getLocaleCode());
                        return;
                    }

                    output.success(
                        "reloaded feature ({feature})"_trl(origin.getLocaleCode(), "feature"_a = param.feature)
                    );
                    return;

                case FeatureParams<T>::FeatureAction::Enable:
                case FeatureParams<T>::FeatureAction::Disable: {
                    auto enable = param.action == FeatureParams<T>::FeatureAction::Enable;
                    if (auto result = manager.setEnabled(param.feature, enable); !result) {
                        output.error(
                            "failed to {operation} feature ({feature}): {error}"_trl(
                                origin.getLocaleCode(),
                                "operation"_a = enable ? "enable" : "disable",
                                "feature"_a   = param.feature,
                                "error"_a     = result.error().message()
                            )
                        );
                        result.error().log(output, origin.getLocaleCode());
                        return;
                    }

                    output.success(
                        enable ? "enabled feature ({feature})"_trl(origin.getLocaleCode(), "feature"_a = param.feature)
                               : "disabled feature ({feature})"_trl(origin.getLocaleCode(), "feature"_a = param.feature)
                    );
                    return;
                }
                }

                LL_UNREACHABLE;
            }
        );
    }
};

} // namespace lo::command
