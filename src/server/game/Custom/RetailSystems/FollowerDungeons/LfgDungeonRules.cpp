/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LfgDungeonRules.h"
#include "Config.h"
#include "DB2Stores.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "World.h"

namespace RetailSystems::FollowerDungeons
{
namespace
{
constexpr uint32 MAP_EXILES_REACH = 2175;
constexpr uint32 MAP_DARKMAUL_CITADEL = 2236;
constexpr uint32 LFG_DUNGEON_DARKMAUL_CITADEL = 2043;

// lock reason retail sends for the dungeons a new player can't queue for yet (not named in LfgLockStatusType)
constexpr uint32 LFG_LOCKSTATUS_NEW_PLAYER_EXPERIENCE = 17;

LfgMatchRules BuildRules(LFGDungeonsEntry const* dungeon)
{
    LfgMatchRules rules;
    uint8 count = dungeon->CountTank + dungeon->CountHealer + dungeon->CountDamage;
    uint8 minCount = dungeon->MinCountTank + dungeon->MinCountHealer + dungeon->MinCountDamage;
    if (!count || count > MAX_GROUP_SIZE)
        count = MAX_GROUP_SIZE;

    rules.MinPlayers = count;
    rules.MaxPlayers = count;

    if (uint32(dungeon->DifficultyID) == DIFFICULTY_FOLLOWER)
    {
        rules.Mode = LfgMatchMode::Follower;
        rules.MinPlayers = 1;
    }
    else if (dungeon->TypeID != lfg::LFG_TYPE_RAID && minCount >= 1 && minCount < count)
    {
        rules.Mode = LfgMatchMode::Flexible;
        rules.MinPlayers = minCount;
    }
    else if (sWorld->getBoolConfig(CONFIG_LFG_SOLO_MODE))
        rules.MinPlayers = 1;

    return rules;
}
}

LfgMatchRules GetMatchRules(uint32 dungeonId)
{
    if (LFGDungeonsEntry const* dungeon = sLFGDungeonsStore.LookupEntry(dungeonId))
        return BuildRules(dungeon);

    return LfgMatchRules();
}

LfgMatchRules GetMatchRules(lfg::LfgDungeonSet const& dungeons)
{
    for (uint32 dungeonId : dungeons)
        if (LFGDungeonsEntry const* dungeon = sLFGDungeonsStore.LookupEntry(dungeonId))
            return BuildRules(dungeon);

    return LfgMatchRules();
}

bool IsFollowerDungeon(uint32 dungeonId)
{
    LFGDungeonsEntry const* dungeon = sLFGDungeonsStore.LookupEntry(dungeonId);
    return dungeon && uint32(dungeon->DifficultyID) == DIFFICULTY_FOLLOWER;
}

bool IsFollowerMap(Map const* map)
{
    return map && uint32(map->GetDifficultyID()) == DIFFICULTY_FOLLOWER;
}

bool IsInNewPlayerExperience(Player const* player)
{
    return player->GetMapId() == MAP_EXILES_REACH || player->GetMapId() == MAP_DARKMAUL_CITADEL;
}

bool IsNewPlayerRestrictedAccount(int32 maxCharacterLevel)
{
    switch (sConfigMgr->GetIntDefault("RetailSystems.NewPlayerExperience.RestrictedAccount", 1))
    {
        case 0:
            return false;
        case 1:
            return maxCharacterLevel < 10;
        default:
            return true;
    }
}

bool IsNewPlayerExperienceCharacter(uint8 level, int32 mapId)
{
    return level < 10 && (mapId == int32(MAP_EXILES_REACH) || mapId == int32(MAP_DARKMAUL_CITADEL));
}

void AddNewPlayerExperienceLocks(Player const* player, lfg::LfgLockMap& locks)
{
    if (!IsInNewPlayerExperience(player))
        return;

    for (LFGDungeonsEntry const* dungeon : sLFGDungeonsStore)
        if (dungeon->ID != LFG_DUNGEON_DARKMAUL_CITADEL)
            locks[dungeon->Entry()] = lfg::LfgLockInfoData(LFG_LOCKSTATUS_NEW_PLAYER_EXPERIENCE);
}

void LogMatchRules()
{
    uint32 flexible = 0;
    uint32 follower = 0;
    for (LFGDungeonsEntry const* dungeon : sLFGDungeonsStore)
    {
        // only the dungeons the dungeon finder can queue for
        if (!sLFGMgr->GetLFGDungeonEntry(dungeon->ID))
            continue;

        switch (BuildRules(dungeon).Mode)
        {
            case LfgMatchMode::Flexible:
                ++flexible;
                TC_LOG_DEBUG("server.loading", "RetailSystems: LFG dungeon {} ({}) can start with fewer players",
                    dungeon->ID, dungeon->Name[sWorld->GetDefaultDbcLocale()]);
                break;
            case LfgMatchMode::Follower:
                ++follower;
                TC_LOG_DEBUG("server.loading", "RetailSystems: LFG dungeon {} ({}) is a follower dungeon",
                    dungeon->ID, dungeon->Name[sWorld->GetDefaultDbcLocale()]);
                break;
            default:
                break;
        }
    }

    TC_LOG_INFO("server.loading", ">> RetailSystems: {} LFG dungeons can start with fewer players, {} follower dungeons", flexible, follower);
}
}
