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

// Crafting (quality crafting via SPELL_EFFECT_CRAFT_ITEM), layouts verified against client 12.0.7.68453
struct CraftingDataEntry
{
    uint32 ID;
    int32 Type;
    int32 CraftingDifficultyID;
    int32 CraftedItemID;
    int32 ItemBonusTreeID;
    int32 CraftingDifficulty;
    float Unknown1;
    float CraftSkillBonusPercent;
    float ReCraftSkillBonusPercent;
    float InspirationSkillBonusPercent;
    float Unknown2;
    float Unknown3;
    int32 FirstCraftFlagQuestID;
    int32 FirstCraftTreasureID;
    int32 Unknown4;
    int32 CraftedTreasureID;
};

struct CraftingDataItemQualityEntry
{
    uint32 ID;
    int32 ItemID;
    int32 CraftingDataID;
};

struct CraftingDifficultyEntry
{
    uint32 ID;
    float MaxRandomSkillBonusPercent;
    float CraftSkillBonusPercent;
    float ReCraftSkillBonusPercent;
    float InspirationSkillBonusPercent;
    float Unknown1;
    int32 ConcentrationSkillCurveID;
    int32 ConcentrationDifficultyCurveID;
};

struct CraftingDifficultyQualityEntry
{
    uint32 ID;
    int32 Order;
    int32 CraftingQualityID;
    float QualityPercentage;
    float QualityMultiplier;
    int32 CraftingDifficultyID;
};

struct CraftingReagentQualityEntry
{
    uint32 ID;
    int32 OrderIndex;
    int32 ItemID;
    int32 Unknown1;
    float MaxDifficultyAdjustment;
    float ReagentEffectPct;
    int32 Unknown2;
    int32 ModifiedCraftingCategoryID;
};

struct ModifiedCraftingSpellSlotEntry
{
    uint32 ID;
    int32 SpellID;
    int32 Slot;
    int32 ModifiedCraftingReagentSlotID;
    int32 Unknown1;
    int32 ReagentCount;
    int32 ReagentReCraftCount;
};

struct ModifiedCraftingReagentSlotEntry
{
    LocalizedString Name;
    uint32 ID;
    int32 Unknown1;
    int32 PlayerConditionID;
    int32 ReagentType;
    uint8 Unknown2;
    int32 Unknown3;
    float Unknown4;
};

struct MCRSlotXMCRCategoryEntry
{
    uint32 ID;
    int32 ModifiedCraftingCategoryID;
    int32 Order;
    int32 ModifiedCraftingReagentSlotID;
};

struct ModifiedCraftingCategoryEntry
{
    uint32 ID;
    LocalizedString DisplayName;
    LocalizedString Description;
    int32 Unknown1;
    float MatQualityWeight;
    int32 Unknown2;
};

struct ModifiedCraftingReagentItemEntry
{
    uint32 ID;
    LocalizedString Description;
    int32 ModifiedCraftingCategoryID;
    int32 ItemBonusTreeID;
    int32 Flags;
    int32 Unknown1;
    int32 ItemContextOffset;
};

#pragma pack(pop)

extern DB2Storage<UIChromieTimeExpansionInfoEntry> UIChromieTimeExpansionInfoStore;
extern DB2Storage<MapChallengeModeAffixCriteriaEntry> MapChallengeModeAffixCriteriaStore;
extern DB2Storage<MythicPlusSeasonKeyFloorEntry> MythicPlusSeasonKeyFloorStore;
extern DB2Storage<MythicPlusSeasonRewardLevelsEntry> MythicPlusSeasonRewardLevelsStore;
extern DB2Storage<MythicPlusSeasonTrackedAffixEntry> MythicPlusSeasonTrackedAffixStore;
extern DB2Storage<MythicPlusSeasonTrackedMapEntry> MythicPlusSeasonTrackedMapStore;
extern DB2Storage<CraftingDataEntry> CraftingDataStore;
extern DB2Storage<CraftingDataItemQualityEntry> CraftingDataItemQualityStore;
extern DB2Storage<CraftingDifficultyEntry> CraftingDifficultyStore;
extern DB2Storage<CraftingDifficultyQualityEntry> CraftingDifficultyQualityStore;
extern DB2Storage<CraftingReagentQualityEntry> CraftingReagentQualityStore;
extern DB2Storage<ModifiedCraftingSpellSlotEntry> ModifiedCraftingSpellSlotStore;
extern DB2Storage<ModifiedCraftingReagentSlotEntry> ModifiedCraftingReagentSlotStore;
extern DB2Storage<MCRSlotXMCRCategoryEntry> MCRSlotXMCRCategoryStore;
extern DB2Storage<ModifiedCraftingCategoryEntry> ModifiedCraftingCategoryStore;
extern DB2Storage<ModifiedCraftingReagentItemEntry> ModifiedCraftingReagentItemStore;

// These stores are intentionally loaded from client files only. Custom SQL data remains
// separate from TrinityCore's generated hotfix schema, which keeps upstream rebases small.
bool LoadStores(std::string const& db2Path, LocaleConstant defaultLocale, std::vector<std::string>& errors);
}

#endif // TRINITYCORE_RETAIL_DB2_STORES_H
