// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted from Endstone commit fa7b3d4eb488c5180bf722d7777b12536e35cc50.
// Modifications: keep selection state local to one send and discard the complete
// recipient list before falling back, so no player receives a partial duplicate.

#pragma once

#include <utility>
#include <vector>

namespace lo::entity_packet_recipients_opt::detail {

template <class Recipient>
class EntityPacketRecipients {
    std::vector<Recipient> mRecipients;
    bool                   mFallback;

public:
    explicit EntityPacketRecipients(bool sourceIdValid) : mFallback(!sourceIdValid) {}

    [[nodiscard]] bool canCollect() const { return !mFallback; }

    // Resolve the user's network identifier only for a selected player. The
    // resolver returns an optional recipient; a missing identifier is skipped.
    template <class Resolve>
    bool visit(bool excluded, bool playerIdValid, bool sameActor, bool nearby, Resolve&& resolve) {
        if (mFallback) return false;
        if (excluded) return true;
        if (!playerIdValid) {
            mRecipients.clear();
            mFallback = true;
            return false;
        }
        if (!sameActor && !nearby) return true;
        if (auto recipient = resolve()) mRecipients.push_back(std::move(*recipient));
        return true;
    }

    template <class Send, class Fallback>
    void finish(Send&& send, Fallback&& fallback) const {
        if (mFallback) {
            fallback();
        } else {
            send(mRecipients);
        }
    }
};

} // namespace lo::entity_packet_recipients_opt::detail
