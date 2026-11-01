/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_LFG_DUNGEON_RULES_H
#define TRINITYCORE_RETAIL_LFG_DUNGEON_RULES_H

#include "Define.h"
#include "LFG.h"

class Map;
class Player;

namespace RetailSystems::FollowerDungeons
{
// Difficulty.db2 205 "Follower": the dungeon finder fills the missing roles with NPC party members
constexpr uint32 DIFFICULTY_FOLLOWER = 205;

enum class LfgMatchMode : uint8
{
    Standard,   // a full group as LFGDungeons.db2 counts it (DungeonFinder.SoloMode lets smaller groups in)
    Flexible,   // LFGDungeons.db2 needs fewer players than it counts (e.g. Darkmaul Citadel, scenarios): a queue pops on its own
    Follower    // follower dungeon: a queue pops on its own, NPCs take the missing roles
};

struct LfgMatchRules
{
    LfgMatchMode Mode = LfgMatchMode::Standard;
    uint8 MinPlayers = 5;
    uint8 MaxPlayers = 5;
};

TC_GAME_API LfgMatchRules GetMatchRules(uint32 dungeonId);
// rules of the first dungeon of the set that LFGDungeons.db2 knows
TC_GAME_API LfgMatchRules GetMatchRules(lfg::LfgDungeonSet const& dungeons);
TC_GAME_API bool IsFollowerDungeon(uint32 dungeonId);
TC_GAME_API bool IsFollowerMap(Map const* map);

// New player experience (Exile's Reach): retail offers no random dungeon and locks every dungeon but Darkmaul Citadel
TC_GAME_API bool IsInNewPlayerExperience(Player const* player);
TC_GAME_API void AddNewPlayerExperienceLocks(Player const* player, lfg::LfgLockMap& locks);
// the "new player restricted" bit of the character list: retail sets it for the new account of the 12.1.5 sniff, and the
// client only loads its new player tutorial (C_PlayerInfo.IsPlayerNPERestricted) for such accounts
TC_GAME_API bool IsNewPlayerRestrictedAccount(int32 maxCharacterLevel);
// a character still in the new player experience (below level 10 on Exile's Reach or Darkmaul Citadel): retail marks
// it with restriction bit 23 in SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA, which turns the client's tutorial on
TC_GAME_API bool IsNewPlayerExperienceCharacter(uint8 level, int32 mapId);

// logs how many dungeons each mode has (called once at startup)
TC_GAME_API void LogMatchRules();
}

#endif // TRINITYCORE_RETAIL_LFG_DUNGEON_RULES_H
