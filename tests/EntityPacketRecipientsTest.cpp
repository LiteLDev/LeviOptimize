#include "features/EntityPacketRecipientsState.h"

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Recipient {
    int  networkId;
    int  subClient;
    bool operator==(Recipient const&) const = default;
};
using Recipients = lo::entity_packet_recipients_opt::detail::EntityPacketRecipients<Recipient>;

void require(bool condition, char const* expression, int line) {
    if (!condition) throw std::runtime_error("line " + std::to_string(line) + ": " + expression);
}
#define REQUIRE(...) require((__VA_ARGS__), #__VA_ARGS__, __LINE__)

struct Delivery {
    std::vector<Recipient> recipients;
    int                    nativeSends{};
    int                    fallbacks{};

    void finish(Recipients const& state) {
        state.finish(
            [&](auto const& ids) {
                recipients = ids;
                ++nativeSends;
            },
            [&] { ++fallbacks; }
        );
    }
};

void invalidSourceUsesOnlyFallback() {
    Recipients state(false);
    REQUIRE(!state.canCollect());
    int resolved{};
    REQUIRE(!state.visit(false, true, true, true, [&]() -> std::optional<Recipient> {
        ++resolved;
        return Recipient{1, 0};
    }));
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(resolved == 0);
    REQUIRE(delivery.nativeSends == 0);
    REQUIRE(delivery.fallbacks == 1);
}

void excludedPlayerNeverResolvesOrInvalidates() {
    Recipients state(true);
    int        resolved{};
    REQUIRE(state.visit(true, false, true, true, [&]() -> std::optional<Recipient> {
        ++resolved;
        return Recipient{1, 0};
    }));
    REQUIRE(state.canCollect());
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(resolved == 0);
    REQUIRE(delivery.nativeSends == 1);
    REQUIRE(delivery.recipients.empty());
    REQUIRE(delivery.fallbacks == 0);
}

void sameActorIsIncludedWithoutNearbyEntry() {
    Recipients state(true);
    REQUIRE(state.visit(false, true, true, false, [] {
        return std::optional{
            Recipient{11, 0}
        };
    }));
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(
        delivery.recipients
        == std::vector<Recipient>{
            {11, 0}
    }
    );
    REQUIRE(delivery.nativeSends == 1);
    REQUIRE(delivery.fallbacks == 0);
}

void onlyNearbyPlayersResolveIdentifiers() {
    Recipients state(true);
    int        resolved{};
    auto       resolve = [&] {
        ++resolved;
        return std::optional{
            Recipient{12, 2}
        };
    };
    REQUIRE(state.visit(false, true, false, false, resolve));
    REQUIRE(resolved == 0);
    REQUIRE(state.visit(false, true, false, true, resolve));
    REQUIRE(resolved == 1);
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(
        delivery.recipients
        == std::vector<Recipient>{
            {12, 2}
    }
    );
}

void missingIdentifierIsSkipped() {
    Recipients state(true);
    REQUIRE(state.visit(false, true, false, true, [] { return std::optional<Recipient>{}; }));
    REQUIRE(state.visit(false, true, true, false, [] { return std::optional<Recipient>{}; }));
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(delivery.recipients.empty());
    REQUIRE(delivery.nativeSends == 1);
    REQUIRE(delivery.fallbacks == 0);
}

void preservesSubClientsAndIterationOrder() {
    Recipients state(true);
    for (Recipient value : {
             Recipient{20, 0},
             Recipient{20, 1},
             Recipient{21, 3}
    }) {
        REQUIRE(state.visit(false, true, false, true, [=] { return std::optional{value}; }));
    }
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(
        delivery.recipients
        == std::vector<Recipient>{
            {20, 0},
            {20, 1},
            {21, 3}
    }
    );
    REQUIRE(delivery.nativeSends == 1);
    REQUIRE(delivery.fallbacks == 0);
}

void lateInvalidIdDiscardsPartialSelectionAndStops() {
    Recipients state(true);
    int        resolved{};
    REQUIRE(state.visit(false, true, false, true, [&] {
        ++resolved;
        return std::optional{
            Recipient{1, 0}
        };
    }));
    REQUIRE(!state.visit(false, false, false, false, [&] {
        ++resolved;
        return std::optional{
            Recipient{2, 0}
        };
    }));
    REQUIRE(!state.canCollect());
    REQUIRE(!state.visit(false, true, true, true, [&] {
        ++resolved;
        return std::optional{
            Recipient{3, 0}
        };
    }));
    REQUIRE(resolved == 1);
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(delivery.nativeSends == 0);
    REQUIRE(delivery.fallbacks == 1);
    REQUIRE(delivery.recipients.empty());
}

void invalidIdMattersEvenForDistantPlayer() {
    Recipients state(true);
    REQUIRE(!state.visit(false, false, false, false, [] {
        return std::optional{
            Recipient{2, 0}
        };
    }));
    Delivery delivery;
    delivery.finish(state);
    REQUIRE(delivery.nativeSends == 0);
    REQUIRE(delivery.fallbacks == 1);
}

void emptySelectionUsesNativeSender() {
    Recipients state(true);
    Delivery   delivery;
    delivery.finish(state);
    REQUIRE(delivery.nativeSends == 1);
    REQUIRE(delivery.recipients.empty());
    REQUIRE(delivery.fallbacks == 0);
}

void separateSendsShareNoRecipientsOrFailureState() {
    Recipients first(true), second(true);
    REQUIRE(first.visit(false, true, true, false, [] {
        return std::optional{
            Recipient{1, 0}
        };
    }));
    REQUIRE(!first.visit(false, false, false, false, [] { return std::optional<Recipient>{}; }));
    REQUIRE(second.visit(false, true, false, true, [] {
        return std::optional{
            Recipient{2, 1}
        };
    }));
    Delivery firstDelivery, secondDelivery;
    firstDelivery.finish(first);
    secondDelivery.finish(second);
    REQUIRE(firstDelivery.nativeSends == 0);
    REQUIRE(firstDelivery.fallbacks == 1);
    REQUIRE(secondDelivery.nativeSends == 1);
    REQUIRE(secondDelivery.fallbacks == 0);
    REQUIRE(
        secondDelivery.recipients
        == std::vector<Recipient>{
            {2, 1}
    }
    );
}

} // namespace

int main() {
    std::pair<char const*, void (*)()> const tests[]{
        {"invalid source falls back",          invalidSourceUsesOnlyFallback                },
        {"excluded player is ignored",         excludedPlayerNeverResolvesOrInvalidates     },
        {"self without nearby entry",          sameActorIsIncludedWithoutNearbyEntry        },
        {"resolve nearby players only",        onlyNearbyPlayersResolveIdentifiers          },
        {"missing identifier is skipped",      missingIdentifierIsSkipped                   },
        {"retain subclients and order",        preservesSubClientsAndIterationOrder         },
        {"discard partial list on failure",    lateInvalidIdDiscardsPartialSelectionAndStops},
        {"distant invalid player falls back",  invalidIdMattersEvenForDistantPlayer         },
        {"empty selection uses native sender", emptySelectionUsesNativeSender               },
        {"per-send state isolation",           separateSendsShareNoRecipientsOrFailureState },
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
