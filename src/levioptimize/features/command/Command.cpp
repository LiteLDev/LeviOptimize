#include "levioptimize/LeviOptimize.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "mc/server/commands/ServerCommands.h"
#include <string>


namespace lo::command {
extern void registerTimingCommand();

LL_STATIC_HOOK(
    RegisterCommandsHook,
    ll::memory::HookPriority::Normal,
    ServerCommands::setupStandardServer,
    void,
    Minecraft&         server,
    std::string const& networkCommands,
    std::string const& networkTestCommands,
    PermissionsFile*   permissionsFile
) {
    origin(server, networkCommands, networkTestCommands, permissionsFile);
    if (LeviOptimize::getInstance().getConfig().commands.timingCommand) {
        registerTimingCommand();
    }
}

static ll::memory::HookRegistrar<RegisterCommandsHook> hooks{};
} // namespace lo::command
