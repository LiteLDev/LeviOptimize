#include "features/ChunkCacheSignatures.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using lo::chunk_caches_opt::detail::BlendingCache;
using lo::chunk_caches_opt::detail::ExistenceCache;

// Independent fixtures for Endstone's two Windows instruction patterns, with
// explicit nonzero displacement bytes. Only those displacement bytes may vary.
constexpr std::array<unsigned char, 10> BlendingCode{0x48, 0x81, 0x7E, 0x38, 0x10, 0x27, 0x00, 0x00, 0x0F, 0x82};
constexpr std::array<unsigned char, 13>
    ExistenceCode{0x49, 0x81, 0xBD, 0xA8, 0x02, 0x00, 0x00, 0x30, 0x75, 0x00, 0x00, 0x0F, 0x82};

void require(bool condition, char const* expression, int line) {
    if (!condition) throw std::runtime_error("line " + std::to_string(line) + ": " + expression);
}
#define REQUIRE(...) require((__VA_ARGS__), #__VA_ARGS__, __LINE__)

void bothSignatures(auto&& test) {
    test(BlendingCache, BlendingCode, std::array<size_t, 1>{3});
    test(ExistenceCache, ExistenceCode, std::array<size_t, 4>{3, 4, 5, 6});
}

void acceptsBothFixturesAtZero() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const&) {
        REQUIRE(signature.matches(fixture));
        auto match = signature.find(fixture);
        REQUIRE(match.has_value());
        REQUIRE(*match == 0);
    });
}

void displacementWildcardsAcceptEveryByte() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const& wildcardPositions) {
        for (auto byte : wildcardPositions) {
            for (unsigned int value = 0; value <= 255; ++value) {
                auto code  = fixture;
                code[byte] = static_cast<unsigned char>(value);
                REQUIRE(signature.matches(code));
                REQUIRE(signature.find(code) == 0);
            }
        }
    });
}

void fixedOpcodesAndImmediateBytesMustMatch() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const& wildcardPositions) {
        for (size_t byte = 0; byte < fixture.size(); ++byte) {
            bool wildcard{};
            for (auto position : wildcardPositions) wildcard = wildcard || byte == position;
            if (wildcard) continue;
            auto code   = fixture;
            code[byte] ^= 0xFF;
            REQUIRE(!signature.matches(code));
            REQUIRE(!signature.find(code));
        }
    });
}

void expectedPatchByteRejectsEveryOtherValue() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const&) {
        REQUIRE(signature.expected == 0);
        REQUIRE(signature.replacement == 0x10);
        for (unsigned int value = 1; value <= 255; ++value) {
            auto code                  = fixture;
            code[signature.byteOffset] = static_cast<unsigned char>(value);
            REQUIRE(!signature.matches(code));
            REQUIRE(!signature.find(code));
        }
    });
}

void rejectsEveryTruncatedLength() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const&) {
        for (size_t length = 0; length < fixture.size(); ++length) {
            auto code = std::span<unsigned char const>{fixture}.first(length);
            REQUIRE(!signature.matches(code));
            REQUIRE(!signature.find(code));
        }
        std::vector<unsigned char> tail(11, 0x90);
        tail.insert(tail.end(), fixture.begin(), fixture.end() - 1);
        REQUIRE(!signature.find(tail));
    });
}

void findsUniqueMatchAtEitherFunctionBoundary() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const&) {
        for (size_t prefix : {size_t{0}, size_t{1}, size_t{37}}) {
            for (size_t suffix : {size_t{0}, size_t{1}, size_t{17}}) {
                std::vector<unsigned char> code(prefix, 0x90);
                code.insert(code.end(), fixture.begin(), fixture.end());
                code.insert(code.end(), suffix, 0x90);
                auto match = signature.find(code);
                REQUIRE(match.has_value());
                REQUIRE(*match == prefix);
            }
        }
    });
}

void rejectsMultipleMatchesWithDifferentDisplacements() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const& wildcardPositions) {
        for (size_t gap : {size_t{0}, size_t{1}, size_t{29}}) {
            std::vector<unsigned char> code(fixture.begin(), fixture.end());
            code.insert(code.end(), gap, 0x90);
            auto second = fixture;
            for (auto byte : wildcardPositions) second[byte] ^= 0xFF;
            code.insert(code.end(), second.begin(), second.end());
            REQUIRE(!signature.find(code));
        }
    });
}

void nearMatchesAndPartialTailsDoNotCreateAmbiguity() {
    bothSignatures([](auto const& signature, auto const& fixture, auto const&) {
        auto invalid = fixture;
        invalid[0]   = 0x90;
        std::vector<unsigned char> code(invalid.begin(), invalid.end());
        code.insert(code.end(), fixture.begin(), fixture.end());
        code.insert(code.end(), fixture.begin(), fixture.end() - 1);
        REQUIRE(signature.find(code) == fixture.size());
    });
}

void distinguishesTheTwoCachePatterns() {
    REQUIRE(!BlendingCache.find(ExistenceCode));
    REQUIRE(!ExistenceCache.find(BlendingCode));
    std::vector<unsigned char> code(BlendingCode.begin(), BlendingCode.end());
    code.insert(code.end(), ExistenceCode.begin(), ExistenceCode.end());
    REQUIRE(BlendingCache.find(code) == 0);
    REQUIRE(ExistenceCache.find(code) == BlendingCode.size());
}

uint32_t readImmediate(std::span<unsigned char const> code, size_t offset) {
    uint32_t value{};
    for (size_t byte = 0; byte < 4; ++byte) value |= uint32_t{code[offset + byte]} << (8 * byte);
    return value;
}

void patchMetadataRaisesOnlyTheCacheCap() {
    auto blending  = BlendingCode;
    auto existence = ExistenceCode;
    REQUIRE(BlendingCache.byteOffset == 6);
    REQUIRE(ExistenceCache.byteOffset == 9);
    REQUIRE(readImmediate(blending, 4) == 10000);
    REQUIRE(readImmediate(existence, 7) == 30000);
    blending[BlendingCache.byteOffset]   = BlendingCache.replacement;
    existence[ExistenceCache.byteOffset] = ExistenceCache.replacement;
    REQUIRE(readImmediate(blending, 4) == 1058576);
    REQUIRE(readImmediate(existence, 7) == 1078576);
    // The expected-byte guard must also reject a patch another mod installed.
    REQUIRE(!BlendingCache.find(blending));
    REQUIRE(!ExistenceCache.find(existence));
}

void unrelatedCodeIsRejected() {
    std::array<unsigned char, 512> code;
    code.fill(0x90);
    REQUIRE(!BlendingCache.find(code));
    REQUIRE(!ExistenceCache.find(code));
    code.fill(0xCC);
    REQUIRE(!BlendingCache.find(code));
    REQUIRE(!ExistenceCache.find(code));
}

} // namespace

int main() {
    std::pair<char const*, void (*)()> const tests[]{
        {"both exact fixtures at zero",           acceptsBothFixturesAtZero                       },
        {"displacement wildcards",                displacementWildcardsAcceptEveryByte            },
        {"reject every changed fixed byte",       fixedOpcodesAndImmediateBytesMustMatch          },
        {"expected patch byte",                   expectedPatchByteRejectsEveryOtherValue         },
        {"truncated signatures",                  rejectsEveryTruncatedLength                     },
        {"unique match at function boundaries",   findsUniqueMatchAtEitherFunctionBoundary        },
        {"reject multiple matches",               rejectsMultipleMatchesWithDifferentDisplacements},
        {"ignore near matches and partial tails", nearMatchesAndPartialTailsDoNotCreateAmbiguity  },
        {"distinguish cache signatures",          distinguishesTheTwoCachePatterns                },
        {"cache cap patch metadata",              patchMetadataRaisesOnlyTheCacheCap              },
        {"reject unrelated code",                 unrelatedCodeIsRejected                         },
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
