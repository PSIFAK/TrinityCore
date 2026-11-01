/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_FOLLOWER_DUNGEON_SERVICE_H
#define TRINITYCORE_RETAIL_FOLLOWER_DUNGEON_SERVICE_H

#include "Define.h"
#include "ObjectGuid.h"

class Creature;
class Group;
class Player;

namespace WorldPackets::Party
{
class PartyMemberFullState;
class PartyUpdate;
}

// Follower dungeons (difficulty 205): the dungeon finder group gets NPC members for the roles no player took.
// The NPCs are kept in a runtime list per group and only added to what the client sees of the party, so the
// player-only group code (database, leader, votes, loot) never meets them.
namespace RetailSystems::FollowerDungeons
{
TC_GAME_API void Initialize();

// spawns the missing NPCs when a player enters a follower dungeon, forgets them when the group left it
TC_GAME_API void OnPlayerMapChanged(Player* player);
TC_GAME_API void OnPlayerLogout(Player* player);
TC_GAME_API void OnGroupDisband(Group const* group);

// a follower NPC that is no longer part of a party despawns itself
TC_GAME_API bool IsPartyMember(Creature const* creature);

// party packets: the NPC members after the players, and their state for the party frames
TC_GAME_API void AppendPartyMembers(Group const& group, WorldPackets::Party::PartyUpdate& partyUpdate);
TC_GAME_API bool FillMemberState(Player const* requester, ObjectGuid const& target, WorldPackets::Party::PartyMemberFullState& state);
}

#endif // TRINITYCORE_RETAIL_FOLLOWER_DUNGEON_SERVICE_H
