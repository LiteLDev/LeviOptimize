#include "lo/features/IFeature.h"
#include "lo/manager/FeatureManager.h"

namespace lo::features {

std::shared_ptr<FeatureBase> FeatureBase::find(std::string_view name) noexcept {
    return manager::FeatureManager::getInstance().findFeature(name);
}

bool FeatureBase::registerFeature(
    std::string_view name,
    FeatureSide      side,
    std::string_view section,
    std::string_view trigger,
    std::string_view stopTrigger,
    FeatureFactory   factory
) {
    return manager::FeatureManager::getInstance().addFeature(name, side, section, trigger, stopTrigger, factory);
}

} // namespace lo::features
