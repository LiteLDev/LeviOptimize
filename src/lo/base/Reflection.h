#pragma once
#include "lo/utils/StringUtils.h"
#include <ll/api/reflection/TypeName.h>
#include <string>
#include <string_view>

namespace lo::base::detail {

template <class T>
[[nodiscard]] std::string_view reflectedName() noexcept {
    static std::string const value = utils::snakeCase(ll::reflection::type_stem_name_v<T>);
    return value;
}

} // namespace lo::base::detail