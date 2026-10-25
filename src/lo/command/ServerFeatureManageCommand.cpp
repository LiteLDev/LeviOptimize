#include "lo/command/FeatureManageCommand.h"
#include "lo/triggers/ServerTriggers.h"

namespace lo::command {

class ServerFeatureManageCommand final : public FeatureManageCommand<ServerFeatureManageCommand> {
public:
    using Trigger = triggers::ServerStarted;
};

LO_REGISTER_FEATURE(ServerFeatureManageCommand);

} // namespace lo::command
