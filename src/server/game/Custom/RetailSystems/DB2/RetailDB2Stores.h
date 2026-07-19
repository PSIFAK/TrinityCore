/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_DB2_STORES_H
#define TRINITYCORE_RETAIL_DB2_STORES_H

#include "Common.h"
#include "DB2Store.h"
#include <string>
#include <vector>

namespace RetailSystems::DB2
{
#pragma pack(push, 1)

struct UIChromieTimeExpansionInfoEntry
{
    uint32 ID;
    LocalizedString Name;
    LocalizedString Description;
    LocalizedString DescriptionAlliance;
    LocalizedString DescriptionHorde;
    int32 SpellID;
    int32 UiTextureAtlasElementLarge;
    int32 UiTextureAtlasElementSmall;
    int32 AlreadyOn;
    int32 ExpansionLevelMask;
    int32 ContentTuningID;
    int32 Completed;
    int32 SortPriority;
    int32 Recommended;
};

struct MapChallengeModeAffixCriteriaEntry
{
    uint32 ID;
    uint32 MapChallengeModeID;
    int32 KeystoneAffixID;
    int32 CriteriaCountBonus;
    int32 MinChallengeLevel;
    int32 MaxChallengeLevel;
};

struct MythicPlusSeasonKeyFloorEntry
{
    uint32 ID;
    int32 KeyFloor;
    int32 PlayerConditionID;
    uint32 DisplaySeasonID;
};

struct MythicPlusSeasonRewardLevelsEntry
{
    uint32 ID;
    uint32 MythicPlusSeasonID;
    int32 ActivityTierID;
    int32 DifficultyLevel;
    int32 WeeklyRewardLevel;
    int32 EndOfRunRewardLevel;
};

struct MythicPlusSeasonTrackedAffixEntry
{
    uint32 ID;
    int32 KeystoneAffixID;
    int32 BonusRating;
    int32 Unknown_9_1_0_38511_004;
    uint32 DisplaySeasonID;
};

struct MythicPlusSeasonTrackedMapEntry
{
    uint32 ID;
    int32 MapChallengeModeID;
    uint32 DisplaySeasonID;
};

#pragma pack(pop)

extern DB2Storage<UIChromieTimeExpansionInfoEntry> UIChromieTimeExpansionInfoStore;
extern DB2Storage<MapChallengeModeAffixCriteriaEntry> MapChallengeModeAffixCriteriaStore;
extern DB2Storage<MythicPlusSeasonKeyFloorEntry> MythicPlusSeasonKeyFloorStore;
extern DB2Storage<MythicPlusSeasonRewardLevelsEntry> MythicPlusSeasonRewardLevelsStore;
extern DB2Storage<MythicPlusSeasonTrackedAffixEntry> MythicPlusSeasonTrackedAffixStore;
extern DB2Storage<MythicPlusSeasonTrackedMapEntry> MythicPlusSeasonTrackedMapStore;

// These stores are intentionally loaded from client files only. Custom SQL data remains
// separate from TrinityCore's generated hotfix schema, which keeps upstream rebases small.
bool LoadStores(std::string const& db2Path, LocaleConstant defaultLocale, std::vector<std::string>& errors);
}

#endif // TRINITYCORE_RETAIL_DB2_STORES_H
