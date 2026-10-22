// SPDX-License-Identifier: GPL-3.0-or-later

#include "features/ChunkGenDispatcherSignature.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using lo::chunk_gen_dispatcher_opt::detail::findWorkerCountInstructions;
using lo::chunk_gen_dispatcher_opt::detail::matchesWorkerCountInstructions;

// Independent fixture from Endstone's Windows signature. The runtime helper
// being tested is also the helper used by ChunkGenDispatcherOpt before writing.
constexpr std::array<unsigned char, 25> ValidCode{0x48, 0x8B, 0x05, 0xA1, 0x82, 0x03, 0xF4, 0x48, 0x8B,
                                                  0x78, 0x28, 0x48, 0x2B, 0x78, 0x20, 0x48, 0xC1, 0xEF,
                                                  0x03, 0x01, 0xFF, 0x44, 0x8D, 0x34, 0x7F};

void require(bool condition, char const* expression, int line) {
    if (!condition) throw std::runtime_error("line " + std::to_string(line) + ": " + expression);
}
#define REQUIRE(...) require((__VA_ARGS__), #__VA_ARGS__, __LINE__)

void exactMatchIncludesOffsetZero() {
    REQUIRE(matchesWorkerCountInstructions(ValidCode));
    auto match = findWorkerCountInstructions(ValidCode);
    REQUIRE(match.has_value());
    REQUIRE(*match == 0);
    REQUIRE(ValidCode[lo::chunk_gen_dispatcher_opt::detail::ShiftImmediate] == 3);
}

void acceptsAnyRipDisplacement() {
    for (size_t byte = 3; byte < 7; ++byte) {
        for (unsigned int value = 0; value <= 255; ++value) {
            auto code  = ValidCode;
            code[byte] = static_cast<unsigned char>(value);
            REQUIRE(matchesWorkerCountInstructions(code));
            REQUIRE(findWorkerCountInstructions(code) == 0);
        }
    }
}

void rejectsEveryChangedFixedByte() {
    // Includes opcode, register, worker-field displacement and arithmetic bytes;
    // the only unconstrained bytes are the RIP displacement tested above.
    for (size_t byte = 0; byte < ValidCode.size(); ++byte) {
        if (byte >= 3 && byte < 7) continue;
        auto code   = ValidCode;
        code[byte] ^= 0xFF;
        REQUIRE(!matchesWorkerCountInstructions(code));
        REQUIRE(!findWorkerCountInstructions(code));
    }
}

void rejectsAlreadyPatchedOrUnexpectedShift() {
    for (unsigned int shift = 0; shift <= 255; ++shift) {
        if (shift == 3) continue;
        auto code                                                  = ValidCode;
        code[lo::chunk_gen_dispatcher_opt::detail::ShiftImmediate] = static_cast<unsigned char>(shift);
        REQUIRE(!matchesWorkerCountInstructions(code));
        REQUIRE(!findWorkerCountInstructions(code));
    }
}

void rejectsEmptyAndIncompleteInstructions() {
    for (size_t length = 0; length < ValidCode.size(); ++length) {
        auto code = std::span<unsigned char const>{ValidCode}.first(length);
        REQUIRE(!matchesWorkerCountInstructions(code));
        REQUIRE(!findWorkerCountInstructions(code));
    }
    std::vector<unsigned char> tail(13, 0x90);
    tail.insert(tail.end(), ValidCode.begin(), ValidCode.end() - 1);
    REQUIRE(!findWorkerCountInstructions(tail));
}

void findsUniqueMatchAtEitherBoundary() {
    // The ending match exercises the final valid start index in the bounded scan.
    for (size_t prefix : {size_t{0}, size_t{1}, size_t{17}}) {
        for (size_t suffix : {size_t{0}, size_t{1}, size_t{19}}) {
            std::vector<unsigned char> code(prefix, 0x90);
            code.insert(code.end(), ValidCode.begin(), ValidCode.end());
            code.insert(code.end(), suffix, 0x90);
            auto match = findWorkerCountInstructions(code);
            REQUIRE(match.has_value());
            REQUIRE(*match == prefix);
        }
    }
}

void ignoresNearMatchAndIncompleteTail() {
    auto nearMatch = ValidCode;
    nearMatch[17]  = 0xE9;
    std::vector<unsigned char> code(nearMatch.begin(), nearMatch.end());
    code.insert(code.end(), ValidCode.begin(), ValidCode.end());
    code.insert(code.end(), ValidCode.begin(), ValidCode.end() - 1);
    auto match = findWorkerCountInstructions(code);
    REQUIRE(match.has_value());
    REQUIRE(*match == nearMatch.size());
}

void rejectsMultipleMatches() {
    for (size_t gap : {size_t{0}, size_t{1}, size_t{23}}) {
        std::vector<unsigned char> code(ValidCode.begin(), ValidCode.end());
        code.insert(code.end(), gap, 0x90);
        auto second  = ValidCode;
        second[3]   ^= 0xFF;
        code.insert(code.end(), second.begin(), second.end());
        REQUIRE(!findWorkerCountInstructions(code));
    }
}

} // namespace

int main() {
    std::pair<char const*, void (*)()> const tests[]{
        {"exact match at zero",                exactMatchIncludesOffsetZero          },
        {"RIP displacement wildcard",          acceptsAnyRipDisplacement             },
        {"reject changed fixed bytes",         rejectsEveryChangedFixedByte          },
        {"reject changed shift",               rejectsAlreadyPatchedOrUnexpectedShift},
        {"reject incomplete instructions",     rejectsEmptyAndIncompleteInstructions },
        {"unique matches at scan boundaries",  findsUniqueMatchAtEitherBoundary      },
        {"ignore near match and partial tail", ignoresNearMatchAndIncompleteTail     },
        {"reject ambiguous matches",           rejectsMultipleMatches                },
    };
    int failed{};
    for (auto const& [name, run] : tests) {
        try {
            run();
            std::cout << "PASS " << name << '\n';
        } catch (std::exception const& error) {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
    std::cout << std::size(tests) - failed << '/' << std::size(tests) << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
