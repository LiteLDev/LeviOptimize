#include "lo/triggers/ITrigger.h"
#include "lo/manager/FeatureManager.h"

namespace lo::triggers {

bool TriggerBase::registerTrigger(std::string_view name, TriggerFactory factory) {
    return manager::FeatureManager::getInstance().addTrigger(name, factory);
}

void TriggerBase::apply() noexcept { manager::FeatureManager::getInstance().applyTrigger(mName); }

} // namespace lo::triggers
