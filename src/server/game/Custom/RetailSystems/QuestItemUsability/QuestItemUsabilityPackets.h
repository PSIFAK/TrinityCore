/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_QUEST_ITEM_USABILITY_PACKETS_H
#define TRINITYCORE_QUEST_ITEM_USABILITY_PACKETS_H

#include "ObjectGuid.h"
#include "Packet.h"
#include <vector>

namespace WorldPackets::QuestItemUsability
{
// sent by the client when the cursor is over a creature while quest items with a use spell are in the bags
class QueryQuestItemUsability final : public ClientPacket
{
public:
    explicit QueryQuestItemUsability(WorldPacket&& packet) : ClientPacket(CMSG_QUERY_QUEST_ITEM_USABILITY, std::move(packet)) { }

    void Read() override;

    ObjectGuid CreatureGUID;
    std::vector<ObjectGuid> ItemGUIDs;
};

// layout from the retail sniff (12.1.5.70688): guid, two counts, the item guids, one bit per item
class QuestItemUsabilityResponse final : public ServerPacket
{
public:
    QuestItemUsabilityResponse() : ServerPacket(SMSG_QUEST_ITEM_USABILITY_RESPONSE, 16 + 4 + 4) { }

    WorldPacket const* Write() override;

    ObjectGuid CreatureGUID;
    std::vector<ObjectGuid> ItemGUIDs;
    std::vector<bool> IsUsable;
};
}

#endif // TRINITYCORE_QUEST_ITEM_USABILITY_PACKETS_H
