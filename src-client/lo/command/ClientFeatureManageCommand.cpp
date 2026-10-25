#include "lo/command/FeatureManageCommand.h"
#include "lo/triggers/ClientTriggers.h"

namespace lo::command {

class ClientFeatureManageCommand final : public FeatureManageCommand<ClientFeatureManageCommand> {
public:
    using Trigger = triggers::ClientJoinedWorld;
};

LO_REGISTER_FEATURE(ClientFeatureManageCommand);

} // namespace lo::command
