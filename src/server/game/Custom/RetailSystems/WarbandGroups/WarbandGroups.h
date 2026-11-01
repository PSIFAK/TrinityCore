/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_WARBAND_GROUPS_H
#define TRINITYCORE_RETAIL_WARBAND_GROUPS_H

#include "Define.h"
#include "ObjectGuid.h"
#include "Packet.h"
#include <string>
#include <vector>

// Warband groups: the character groups of the character select screen ("Favorites" and the ones the player makes,
// each with its warband scene). The client sends the whole layout (CMSG_SETUP_WARBAND_GROUPS) whenever it changes;
// the server keeps it per account and returns it in SMSG_ENUM_CHARACTERS_RESULT.
namespace RetailSystems::WarbandGroups
{
struct Member
{
    uint32 WarbandScenePlacementID = 0;
    int32 Type = 0;           // 0 = character (Guid set)
    int32 ContentSetID = 0;
    ObjectGuid Guid;
};

struct Group
{
    uint64 GroupID = 0;
    uint8 OrderIndex = 0;
    uint32 WarbandSceneID = 0;
    uint32 Flags = 0;         // 1 = collapsed
    int32 ContentSetID = 0;
    std::vector<Member> Members;
    std::string Name;
};

TC_GAME_API std::vector<Group> Load(uint32 accountId);
TC_GAME_API void Save(uint32 accountId, std::vector<Group> const& groups);
// a deleted character leaves every group
TC_GAME_API void RemoveCharacter(ObjectGuid const& guid);
}

namespace WorldPackets::WarbandGroups
{
// layout from the retail sniff (12.1.5.70688): 5 bit group count, then the groups as in the character list
class SetupWarbandGroups final : public ClientPacket
{
public:
    explicit SetupWarbandGroups(WorldPacket&& packet) : ClientPacket(CMSG_SETUP_WARBAND_GROUPS, std::move(packet)) { }

    void Read() override;

    std::vector<RetailSystems::WarbandGroups::Group> Groups;
};
}

#endif // TRINITYCORE_RETAIL_WARBAND_GROUPS_H
