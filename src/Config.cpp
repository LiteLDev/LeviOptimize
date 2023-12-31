#include "Config.h"
#include "ll/api/Config.h"
#include "ll/api/Logger.h"
#include "ll/api/base/ErrorInfo.h"
#include "ll/api/i18n/I18nAPI.h"

extern ll::Logger logger;
using namespace ll::i18n_literals;

namespace lo {

struct Config globalConfig;

static constexpr std::string_view configPath = R"(plugins\LeviOptimize\config\config.json)";


bool loadLoConfig() {
    ll::i18n::load(u8"plugins/LeviLamina/LangPack");
    try {
        if (!ll::config::loadConfig(globalConfig, configPath)) {
            if (ll::config::saveConfig(globalConfig, configPath)) {
                logger.info("ll.config.rewrite.success"_tr);
            } else {
                logger.error("ll.config.rewrite.fail"_tr);
                return false;
            }
        }
        return true;
    } catch (...) {
        logger.error("ll.config.load.fail"_tr);
        ll::error_info::printCurrentException();
        return false;
    }
}

bool saveLoConfig() {
    bool res{};
    try {
        res = ll::config::saveConfig(globalConfig, configPath);
    } catch (...) {
        logger.error("ll.config.save.fail"_tr);
        ll::error_info::printCurrentException();
        return false;
    }
    if (!res) {
        logger.error("ll.config.save.fail"_tr);
        return false;
    }
    return true;
}

} // namespace lo
