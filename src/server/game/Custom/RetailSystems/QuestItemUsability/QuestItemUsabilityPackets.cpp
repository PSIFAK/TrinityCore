/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "QuestItemUsabilityPackets.h"
#include "PacketOperators.h"
#include "PacketUtilities.h"

namespace WorldPackets::QuestItemUsability
{
// the client never sends more than its bags can hold
static constexpr uint32 MaxQueriedItems = 64;

void QueryQuestItemUsability::Read()
{
    _worldPacket >> CreatureGUID;
    uint32 count = _worldPacket.read<uint32>();
    if (count > MaxQueriedItems)
        throw PacketArrayMaxCapacityException(count, MaxQueriedItems);

    ItemGUIDs.resize(count);
    for (ObjectGuid& itemGuid : ItemGUIDs)
        _worldPacket >> itemGuid;
}

WorldPacket const* QuestItemUsabilityResponse::Write()
{
    _worldPacket << CreatureGUID;
    _worldPacket << uint32(ItemGUIDs.size());
    _worldPacket << uint32(IsUsable.size());
    for (ObjectGuid const& itemGuid : ItemGUIDs)
        _worldPacket << itemGuid;

    for (bool usable : IsUsable)
        _worldPacket << Bits<1>(usable);

    _worldPacket.FlushBits();

    return &_worldPacket;
}
}
