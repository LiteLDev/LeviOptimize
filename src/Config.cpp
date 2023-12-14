#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/Logger.h"
#include "ll/api/base/ErrorInfo.h"


extern ll::Logger logger;

namespace lo {

struct Config globalConfig;

static constexpr std::string_view configPath = R"(plugins\LeviOptimize\config\config.json)";


bool loadLoConfig() {
    try {
        if (!ll::config::loadConfig(globalConfig, configPath)) {
            if (ll::config::saveConfig(globalConfig, configPath)) {
                logger.info("ll.config.rewrite.success");
            } else {
                logger.error("ll.config.rewrite.fail");
                return false;
            }
        }
        return true;
    } catch (...) {
        logger.error("ll.config.load.fail");
        ll::error_info::printCurrentException();
        return false;
    }
}

bool saveLoConfig() {
    bool res{};
    try {
        res = ll::config::saveConfig(globalConfig, configPath);
    } catch (...) {
        logger.error("ll.config.save.fail");
        ll::error_info::printCurrentException();
        return false;
    }
    if (!res) {
        logger.error("ll.config.save.fail");
        return false;
    }
    return true;
}

} // namespace lo
