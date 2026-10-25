#pragma once
#include <ll/api/base/StdInt.h>
#include <cctype>
#include <string>
#include <string_view>
#include <type_traits>
#include <magic_enum/magic_enum.hpp>

namespace lo::utils {

[[nodiscard]] inline std::string snakeCase(std::string_view input) {
    if (input.empty()) return std::string{input};

    std::string_view sv = input;
    if (sv.front() == 'm' && sv.size() >= 2 && std::isupper(static_cast<uint8>(sv[1]))) {
        sv.remove_prefix(1);
    }

    std::string result;
    result.reserve(sv.size() * 2);

    for (size_t i = 0; i < sv.size(); ++i) {
        auto c = static_cast<uint8>(sv[i]);
        if (std::isupper(c)) {
            if (i != 0) {
                auto prev = static_cast<uint8>(sv[i - 1]);
                if (std::islower(prev) || std::isdigit(prev)) {
                    result += '_';
                } else if (std::isupper(prev) && i + 1 < sv.size() && std::islower(static_cast<uint8>(sv[i + 1]))) {
                    result += '_';
                }
            }
            result += static_cast<int8>(std::tolower(c));
        } else {
            result += static_cast<int8>(c);
        }
    }
    return result;
}

struct ConfigKeyFormatter {
    std::string operator()(std::string_view input) const { return snakeCase(input); }

    template <class Enum>
        requires std::is_enum_v<Enum>
    std::string operator()(Enum value) const {
        return operator()(magic_enum::enum_name(value));
    }
};

} // namespace lo::utils
