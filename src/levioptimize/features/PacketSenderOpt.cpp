#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/deps/core/utility/BinaryStream.h"
#include "mc/deps/raknet/AddressOrGUID.h"
#include "mc/deps/raknet/RakPeer.h"
#include "mc/network/BatchedNetworkPeer.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkConnection.h"
#include "mc/client/renderer/rendergraph/Packet.h"
#include "mc/network/NetworkSystem.h"
#include "mc/network/RakNetConnector.h"
#include "mc/deps/raknet/PacketPriority.h"
#include "mc/deps/raknet/PacketReliability.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/deps/raknet/RakPeerInterface.h"
#include "mc/network/NetworkIdentifierWithSubId.h"

namespace lo::packet_sender_opt {

LL_TYPE_INSTANCE_HOOK(
    RakPeerSendPacketHook,
    ll::memory::HookPriority::Normal,
    RakNetConnector::RakNetNetworkPeer,
    &RakNetConnector::RakNetNetworkPeer::$sendPacket,
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
    mRakPeer
        .SendList(datas.data(), lengths.data(), 2, PacketPriority::MediumPriority, raknetRel, 0, mId.mGuid, false, 0);
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
    // packet.writeWithHeader(senderSubId, stream);
    stream.writeUnsignedVarInt(
        std::to_underlying(packet.getId()) | (std::to_underlying(senderSubId) << 10)
            | ((std::to_underlying(packet.mSenderSubId) << 12)),
        nullptr,
        nullptr
    );
    packet.write(stream);
    return _sendInternal(id, packet, stream.mBuffer);
}

LL_TYPE_INSTANCE_HOOK(
    NetworkSystemSendMultiHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClients,
    void,
    std::vector<struct NetworkIdentifierWithSubId> const& ids,
    Packet const&                                         packet
) {
    if (ids.empty()) {
        return;
    }
    auto& networkSystem = mNetwork->toServerNetworkSystem();

    BinaryStream pktstream;
    pktstream.write("\0\0\0\0\0", 5);
    packet.write(pktstream);
    auto res = std::move(pktstream.mBuffer);

    BinaryStream headerstream;
    for (auto& id : ids) {
        // headerstream.reset();
        {
            headerstream.mBuffer.clear();
            headerstream.mBuffer.shrink_to_fit();
            headerstream.mView        = std::string_view(headerstream.mBuffer);
            headerstream.mReadPointer = 0;
        }


        headerstream.writeUnsignedVarInt(
            ((std::to_underlying(packet.mSenderSubId) & 3) << 12) | (std::to_underlying(packet.getId()) & 0x3FF)
                | ((std::to_underlying(id.subClientId) & 3) << 10),
            nullptr,
            nullptr
        );
        auto size = headerstream.mBuffer.size();
        memcpy(res.data() + (5 - size), headerstream.mBuffer.data(), size);
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
        networkSystem._sendInternal(id.id, packet, *(std::string*)(&datas));
    }
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerCtorHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    &BatchedNetworkPeer::$ctor,
    void*,
    std::shared_ptr<class NetworkPeer> peer,
    class Scheduler&                   scheduler
) {
    auto res                                                     = origin(std::move(peer), scheduler);
    *(std::recursive_mutex**)(&mSendQueue->mCachelineFiller[32]) = new std::recursive_mutex;
    return res;
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerDtorHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    &BatchedNetworkPeer::$dtor,
    void
) {
    delete *(std::recursive_mutex**)(&mSendQueue->mCachelineFiller[32]);
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerSendPacketHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    &BatchedNetworkPeer::$sendPacket,
    void,
    std::string const&         data,
    ::NetworkPeer::Reliability reliability,
    ::Compressibility          compressibility
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue->mCachelineFiller[32]);
    origin(data, reliability, compressibility);
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerFlushHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    &BatchedNetworkPeer::$flush,
    void,
    std::function<void()>&& callback
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue->mCachelineFiller[32]);
    origin(std::move(callback));
}

LL_TYPE_INSTANCE_HOOK(
    BatchedPeerUpdateHook,
    ll::memory::HookPriority::Normal,
    BatchedNetworkPeer,
    &BatchedNetworkPeer::$update,
    void
) {
    std::lock_guard l(**(std::recursive_mutex**)&mSendQueue->mCachelineFiller[32]);
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
        // CompressedPeerSendPacketHook,
        // EncryptedPeerSendPacketHook,
        NetworkSystemSendHook,
        NetworkSystemSendMultiHook>
        r;
    std::optional<ll::memory::HookRegistrar<
        BatchedPeerSendPacketHook,
        BatchedPeerFlushHook,
        BatchedPeerUpdateHook,
        BatchedPeerCtorHook,
        BatchedPeerDtorHook>>
        batched;
};

void PacketSenderOpt::call(Config const& config) {
    if (config.enable) {
        if (!impl) impl = std::make_unique<Impl>();
        if (config.multiThreadBatch) {
            if (!impl->batched) impl->batched.emplace();
        } else {
            impl->batched.reset();
        }
    } else {
        impl.reset();
    }
}

PacketSenderOpt::PacketSenderOpt()  = default;
PacketSenderOpt::~PacketSenderOpt() = default;

} // namespace lo::packet_sender_opt
