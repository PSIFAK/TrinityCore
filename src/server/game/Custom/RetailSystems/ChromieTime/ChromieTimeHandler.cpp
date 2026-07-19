/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChromieTime.h"
#include "ChromieTimePackets.h"
#include "GossipDef.h"
#include "Player.h"
#include "WorldSession.h"

void WorldSession::HandleChromieTimeSelectExpansion(WorldPackets::ChromieTime::SelectExpansion& packet)
{
    Player* player = GetPlayer();
    if (!player->PlayerTalkClass->GetInteractionData().IsInteractingWith(packet.NpcGUID, PlayerInteractionType::ChromieTime))
        return;

    if (!player->GetNPCIfCanInteractWith(packet.NpcGUID, UNIT_NPC_FLAG_NONE, UNIT_NPC_FLAG_2_NONE))
        return;

    if (!RetailSystems::ChromieTime::Select(player, packet.ExpansionID))
        return;

    WorldPackets::ChromieTime::SelectExpansionSuccess success;
    SendPacket(success.Write());
}
