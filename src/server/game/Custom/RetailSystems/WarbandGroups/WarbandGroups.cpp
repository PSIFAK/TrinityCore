/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WarbandGroups.h"
#include "DatabaseEnv.h"
#include "PacketOperators.h"
#include "PacketUtilities.h"
#include "WorldSession.h"
#include <unordered_map>

namespace RetailSystems::WarbandGroups
{
// client limits: groups shown on the character screen, characters per group
static constexpr uint32 MaxGroups = 31;
static constexpr uint32 MaxMembers = 64;

std::vector<Group> Load(uint32 accountId)
{
    std::vector<Group> groups;
    std::unordered_map<uint64, size_t> indexById;

    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT `group_id`, `order_index`, `warband_scene_id`, `flags`, `content_set_id`, `name` FROM `retail_account_warband_group` "
        "WHERE `account_id` = {} ORDER BY `order_index`", accountId))
    {
        do
        {
            Field* fields = result->Fetch();
            Group& group = groups.emplace_back();
            group.GroupID = fields[0].GetUInt64();
            group.OrderIndex = fields[1].GetUInt8();
            group.WarbandSceneID = fields[2].GetUInt32();
            group.Flags = fields[3].GetUInt32();
            group.ContentSetID = fields[4].GetInt32();
            group.Name = fields[5].GetString();
            indexById[group.GroupID] = groups.size() - 1;
        } while (result->NextRow());
    }

    if (groups.empty())
        return groups;

    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT `group_id`, `placement_id`, `type`, `content_set_id`, `guid` FROM `retail_account_warband_group_member` "
        "WHERE `account_id` = {} ORDER BY `group_id`, `member_index`", accountId))
    {
        do
        {
            Field* fields = result->Fetch();
            auto itr = indexById.find(fields[0].GetUInt64());
            if (itr == indexById.end())
                continue;

            Member& member = groups[itr->second].Members.emplace_back();
            member.WarbandScenePlacementID = fields[1].GetUInt32();
            member.Type = fields[2].GetInt32();
            member.ContentSetID = fields[3].GetInt32();
            if (member.Type == 0)
                member.Guid = ObjectGuid::Create<HighGuid::Player>(fields[4].GetUInt64());
        } while (result->NextRow());
    }

    return groups;
}

void Save(uint32 accountId, std::vector<Group> const& groups)
{
    CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
    transaction->PAppend("DELETE FROM `retail_account_warband_group_member` WHERE `account_id` = {}", accountId);
    transaction->PAppend("DELETE FROM `retail_account_warband_group` WHERE `account_id` = {}", accountId);

    for (Group const& group : groups)
    {
        std::string name = group.Name;
        CharacterDatabase.EscapeString(name);
        transaction->PAppend(
            "INSERT INTO `retail_account_warband_group` (`account_id`, `group_id`, `order_index`, `warband_scene_id`, `flags`, "
            "`content_set_id`, `name`) VALUES ({}, {}, {}, {}, {}, {}, '{}')",
            accountId, group.GroupID, group.OrderIndex, group.WarbandSceneID, group.Flags, group.ContentSetID, name);

        for (size_t i = 0; i < group.Members.size(); ++i)
        {
            Member const& member = group.Members[i];
            transaction->PAppend(
                "INSERT INTO `retail_account_warband_group_member` (`account_id`, `group_id`, `member_index`, `placement_id`, "
                "`type`, `content_set_id`, `guid`) VALUES ({}, {}, {}, {}, {}, {}, {})",
                accountId, group.GroupID, i, member.WarbandScenePlacementID, member.Type, member.ContentSetID,
                member.Type == 0 ? member.Guid.GetCounter() : 0);
        }
    }

    CharacterDatabase.CommitTransaction(transaction);
}

void RemoveCharacter(ObjectGuid const& guid)
{
    CharacterDatabase.PExecute("DELETE FROM `retail_account_warband_group_member` WHERE `type` = 0 AND `guid` = {}", guid.GetCounter());
}
}

namespace WorldPackets::WarbandGroups
{
using RetailSystems::WarbandGroups::MaxGroups;
using RetailSystems::WarbandGroups::MaxMembers;

void SetupWarbandGroups::Read()
{
    uint32 groupCount = _worldPacket.ReadBits(5);
    _worldPacket.ResetBitPos();
    if (groupCount > MaxGroups)
        throw PacketArrayMaxCapacityException(groupCount, MaxGroups);

    Groups.resize(groupCount);
    for (RetailSystems::WarbandGroups::Group& group : Groups)
    {
        _worldPacket >> group.GroupID;
        _worldPacket >> group.OrderIndex;
        _worldPacket >> group.WarbandSceneID;
        _worldPacket >> group.Flags;
        _worldPacket >> group.ContentSetID;

        uint32 memberCount = _worldPacket.read<uint32>();
        if (memberCount > MaxMembers)
            throw PacketArrayMaxCapacityException(memberCount, MaxMembers);

        group.Members.resize(memberCount);
        for (RetailSystems::WarbandGroups::Member& member : group.Members)
        {
            _worldPacket >> member.WarbandScenePlacementID;
            _worldPacket >> member.Type;
            _worldPacket >> member.ContentSetID;
            if (member.Type == 0)
                _worldPacket >> member.Guid;
        }

        _worldPacket >> SizedString::BitsSize<9>(group.Name);
        _worldPacket.ResetBitPos();
        _worldPacket >> SizedString::Data(group.Name);
    }
}
}

void WorldSession::HandleSetupWarbandGroups(WorldPackets::WarbandGroups::SetupWarbandGroups& packet)
{
    // only characters of this account may be put into its groups
    for (RetailSystems::WarbandGroups::Group& group : packet.Groups)
        std::erase_if(group.Members, [this](RetailSystems::WarbandGroups::Member const& member)
        {
            return member.Type == 0 && !IsLegitCharacterForAccount(member.Guid);
        });

    RetailSystems::WarbandGroups::Save(GetAccountId(), packet.Groups);
}
