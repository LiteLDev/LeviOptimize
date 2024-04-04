#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/deps/raknet/AddressOrGUID.h"
#include "mc/deps/raknet/RakPeer.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/RakNetConnector.h"
#include "mc/resources/PacketPriority.h"
#include "mc/resources/PacketReliability.h"

#include "ll/api/utils/StacktraceUtils.h"

namespace lo::packet_sender_opt {

LL_TYPE_INSTANCE_HOOK(
    RakPeerSendPacketHook,
    ll::memory::HookPriority::Normal,
    RakNetConnector::RakNetNetworkPeer,
    // &RakNetConnector::RakNetNetworkPeer::sendPacket,
    "?sendPacket@RakNetNetworkPeer@RakNetConnector@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@W4Reliability@NetworkPeer@@W4Compressibility@@@Z",
    void,
    std::string const&         data,
    ::NetworkPeer::Reliability reliability,
    ::Compressibility /*compressibility*/
) {
    PacketReliability raknetRel{};

    switch (reliability) {
    case NetworkPeer::Reliability::Reliable:
        raknetRel = PacketReliability::Reliable;
        break;
    default:
    case NetworkPeer::Reliability::ReliableOrdered:
        raknetRel = PacketReliability::ReliableOrdered;
        break;
    case NetworkPeer::Reliability::Unreliable:
        raknetRel = PacketReliability::Unreliable;
        break;
    case NetworkPeer::Reliability::UnreliableSequenced:
        raknetRel = PacketReliability::UnreliableSequenced;
        break;
    }

    std::array<char const*, 2> datas{"\xFE", data.c_str()};
    std::array<int, 2>         lengths{1, (int)data.size()};

    ll::memory::virtualCall<
        uint,
        char const**,
        int const*,
        int,
        ::PacketPriority,
        ::PacketReliability,
        char,
        struct RakNet::AddressOrGUID,
        bool,
        uint>(
        ll::memory::dAccess<RakNet::RakPeer*>(this, 0x18),
        24, // sendList
        datas.data(),
        lengths.data(),
        2,
        PacketPriority::Medium,
        raknetRel,
        0,
        ll::memory::dAccess<NetworkIdentifier>(this, 0x20).mGuid,
        0,
        0
    );
}

struct PacketSenderOpt::Impl {
    ll::memory::HookRegistrar<RakPeerSendPacketHook> r;
};

void PacketSenderOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

PacketSenderOpt::PacketSenderOpt()  = default;
PacketSenderOpt::~PacketSenderOpt() = default;

} // namespace lo::packet_sender_opt
