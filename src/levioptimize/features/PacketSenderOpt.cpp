#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/deps/raknet/AddressOrGUID.h"
#include "mc/deps/raknet/RakPeer.h"
#include "mc/network/BatchedNetworkPeer.h"
#include "mc/network/CompressedNetworkPeer.h"
#include "mc/network/EncryptedNetworkPeer.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkSystem.h"
#include "mc/network/RakNetConnector.h"
#include "mc/network/packet/Packet.h"
#include "mc/resources/PacketPriority.h"
#include "mc/resources/PacketReliability.h"

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
        ll::memory::dAccess<RakNet::RakPeerInterface*>(this, 0x18),
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

LL_TYPE_INSTANCE_HOOK(
    NetworkSystemSendHook,
    ll::memory::HookPriority::Normal,
    NetworkSystem,
    &NetworkSystem::send,
    void,
    NetworkIdentifier const& id,
    Packet const&            packet,
    ::SubClientId            senderSubId
) {
    BinaryStream stream;
    packet.writeWithHeader(senderSubId, stream);
    return _sendInternal(id, packet, stream.getAndReleaseData());
}

LL_TYPE_INSTANCE_HOOK(
    NetworkSystemSendMultiHook,
    ll::memory::HookPriority::Normal,
    NetworkSystem,
    &NetworkSystem::sendToMultiple,
    void,
    std::vector<struct NetworkIdentifierWithSubId> const& ids,
    Packet const&                                         packet
) {
    if (ids.empty()) {
        return;
    }
    BinaryStream pktstream;
    pktstream.write("\0\0\0\0\0", 5);
    packet.write(pktstream);
    auto res = pktstream.getAndReleaseData();

    BinaryStream headerstream;
    for (auto& id : ids) {
        headerstream.reset();
        headerstream.writeUnsignedVarInt(
            ((std::to_underlying(packet.mClientSubId) & 3) << 12) | (std::to_underlying(packet.getId()) & 0x3FF)
            | ((std::to_underlying(id.mSubClientId) & 3) << 10)
        );
        auto size = headerstream.mBuffer->size();
        memcpy(res.data() + (5 - size), headerstream.mBuffer->data(), size);
        struct {
            char const* data;
            void*       filler{};
            size_t      size;
            size_t      cap;
        } datas{
            .data = res.data() + (5 - size),
            .size = res.size() - 5 + size,
            .cap  = res.capacity() > 16 ? res.capacity() : 16
        };
        _sendInternal(id.mIdentifier, packet, *(std::string*)(&datas));
    }
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerCtorHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    "??0BatchedNetworkPeer@@QEAA@V?$shared_ptr@VNetworkPeer@@@std@@AEAVScheduler@@@Z",
    BatchedNetworkPeer*,
    std::shared_ptr<class NetworkPeer> peer,
    class Scheduler&                   scheduler
) {
    auto res                                                = origin(std::move(peer), scheduler);
    *(std::recursive_mutex**)(&mSendQueue.mCachelineFiller[32]) = new std::recursive_mutex;
    return res;
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerDtorHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    "??_GBatchedNetworkPeer@@UEAAPEAXI@Z",
    void
) {
    delete *(std::recursive_mutex**)(&mSendQueue.mCachelineFiller[32]);
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerSendPacketHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    "?sendPacket@BatchedNetworkPeer@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "W4Reliability@NetworkPeer@@W4Compressibility@@@Z",
    void,
    std::string const&         data,
    ::NetworkPeer::Reliability reliability,
    ::Compressibility          compressibility
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue.mCachelineFiller[32]);
    origin(data, reliability, compressibility);
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerFlushHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    "?flush@BatchedNetworkPeer@@UEAAX$$QEAV?$function@$$A6AXXZ@std@@@Z",
    void,
    std::function<void()>&& callback
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue.mCachelineFiller[32]);
    origin(std::move(callback));
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerUpdateHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    "?update@BatchedNetworkPeer@@UEAAXXZ",
    void
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue.mCachelineFiller[32]);
    origin();
}

// LL_TYPE_INSTANCE_HOOK(
//     CompressedPeerSendPacketHook,
//     ll::memory::HookPriority::Normal,
//     CompressedNetworkPeer,
//     "?sendPacket@CompressedNetworkPeer@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
//     "W4Reliability@NetworkPeer@@W4Compressibility@@@Z",
//     void,
//     std::string const&         data,
//     ::NetworkPeer::Reliability reliability,
//     ::Compressibility          compressibility
// ) {
//     origin(data, reliability, compressibility);
// }

// LL_TYPE_INSTANCE_HOOK(
//     EncryptedPeerSendPacketHook,
//     ll::memory::HookPriority::Normal,
//     EncryptedNetworkPeer,
//     "?sendPacket@EncryptedNetworkPeer@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
//     "W4Reliability@NetworkPeer@@W4Compressibility@@@Z",
//     void,
//     std::string const&         data,
//     ::NetworkPeer::Reliability reliability,
//     ::Compressibility          compressibility
// ) {
//     origin(data, reliability, compressibility);
// }

struct PacketSenderOpt::Impl {
    ll::memory::HookRegistrar<
        RakPeerSendPacketHook,
        BatchedPeerSendPacketHook,
        BatchedPeerFlushHook,
        BatchedPeerUpdateHook,
        BatchedPeerCtorHook,
        BatchedPeerDtorHook,
        // CompressedPeerSendPacketHook,
        // EncryptedPeerSendPacketHook,
        NetworkSystemSendHook,
        NetworkSystemSendMultiHook>
        r;
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
