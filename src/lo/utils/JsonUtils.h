#pragma once
#include "lo/utils/StringUtils.h"
#include <ll/api/i18n/I18n.h>
#include <ll/api/io/FileUtils.h>
#include <ll/api/reflection/Deserialization.h>
#include <ll/api/reflection/Serialization.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <system_error>

namespace lo::utils {

using Json = nlohmann::ordered_json;

template <class T>
[[nodiscard]] ll::Expected<Json> toJson(T const& value) {
    return ll::reflection::serialize<Json>(value, ConfigKeyFormatter{});
}

template <class T>
[[nodiscard]] ll::Expected<T> fromJson(Json const& value) {
    return ll::reflection::deserialize_to<T>(value, ConfigKeyFormatter{});
}

// defaults: { "enable": true, "max_trackers": 8 }
// user:     { "max_trackers": 4 }
// merged:   { "enable": true, "max_trackers": 4 }
[[nodiscard]] inline Json merge(Json const& defaults, Json const& user) {
    if (user.is_null()) return defaults;
    if (!defaults.is_object() || !user.is_object()) return user;

    auto result = defaults;
    for (auto const& [key, value] : user.items()) {
        if (auto it = result.find(key); it != result.end() && it->is_object() && value.is_object()) {
            *it = merge(*it, value);
        } else {
            result[key] = value;
        }
    }
    return result;
}

[[nodiscard]] inline ll::Expected<Json> readJson(std::filesystem::path const& path) {
    using namespace fmt::literals;

    auto content = ll::file_utils::readFile(path);
    if (!content) {
        return ll::makeI18nStringError<"failed to read configuration file ({path})">(
            "path"_a = path.lexically_normal()
        );
    }

    auto value = Json::parse(*content, nullptr, false, true);
    if (value.is_discarded()) {
        return ll::makeI18nStringError<"configuration file ({path}) contains invalid JSON">(
            "path"_a = path.lexically_normal()
        );
    }
    if (!value.is_object()) {
        return ll::makeI18nStringError<"configuration file ({path}) must contain a JSON object">(
            "path"_a = path.lexically_normal()
        );
    }
    return value;
}

[[nodiscard]] inline ll::Expected<> writeJson(std::filesystem::path const& path, Json const& value) {
    using namespace fmt::literals;

    auto parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return ll::makeI18nStringError<"failed to create configuration directory ({path}): {error}">(
                "path"_a  = parent.lexically_normal(),
                "error"_a = ec.message()
            );
        }
    }
    if (!ll::file_utils::writeFile(path, value.dump(4))) {
        return ll::makeI18nStringError<"failed to write configuration file ({path})">(
            "path"_a = path.lexically_normal()
        );
    }
    return {};
}

} // namespace lo::utils