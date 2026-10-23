/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RetailDB2Stores.h"
#include "Config.h"
#include "DB2DatabaseLoader.h"
#include "DB2Metadata.h"
#include "Errors.h"
#include "StringFormat.h"
#include <boost/filesystem/path.hpp>

namespace RetailSystems::DB2
{
namespace
{
constexpr DB2FieldMeta UIChromieTimeExpansionInfoFields[14] =
{
    { .IsSigned = false, .Type = FT_INT,    .Name = "ID" },
    { .IsSigned = false, .Type = FT_STRING, .Name = "Name" },
    { .IsSigned = false, .Type = FT_STRING, .Name = "Description" },
    { .IsSigned = false, .Type = FT_STRING, .Name = "DescriptionAlliance" },
    { .IsSigned = false, .Type = FT_STRING, .Name = "DescriptionHorde" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "SpellID" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "UiTextureAtlasElementLarge" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "UiTextureAtlasElementSmall" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "AlreadyOn" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "ExpansionLevelMask" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "ContentTuningID" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "Completed" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "SortPriority" },
    { .IsSigned = true,  .Type = FT_INT,    .Name = "Recommended" }
};

// Statement is deliberately unused because RetailSystems stores do not call LoadFromDB().
constexpr DB2LoadInfo UIChromieTimeExpansionInfoLoadInfo
{
    UIChromieTimeExpansionInfoFields,
    std::size(UIChromieTimeExpansionInfoFields),
    &UIChromieTimeExpansionInfoMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MapChallengeModeAffixCriteriaFields[6] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = false, .Type = FT_INT, .Name = "MapChallengeModeID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "KeystoneAffixID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "CriteriaCountBonus" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "MinChallengeLevel" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "MaxChallengeLevel" }
};

constexpr DB2LoadInfo MapChallengeModeAffixCriteriaLoadInfo
{
    MapChallengeModeAffixCriteriaFields,
    std::size(MapChallengeModeAffixCriteriaFields),
    &MapChallengeModeAffixCriteriaMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MythicPlusSeasonKeyFloorFields[4] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "KeyFloor" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "PlayerConditionID" },
    { .IsSigned = false, .Type = FT_INT, .Name = "DisplaySeasonID" }
};

constexpr DB2LoadInfo MythicPlusSeasonKeyFloorLoadInfo
{
    MythicPlusSeasonKeyFloorFields,
    std::size(MythicPlusSeasonKeyFloorFields),
    &MythicPlusSeasonKeyFloorMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MythicPlusSeasonRewardLevelsFields[6] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = false, .Type = FT_INT, .Name = "MythicPlusSeasonID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "ActivityTierID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "DifficultyLevel" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "WeeklyRewardLevel" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "EndOfRunRewardLevel" }
};

constexpr DB2LoadInfo MythicPlusSeasonRewardLevelsLoadInfo
{
    MythicPlusSeasonRewardLevelsFields,
    std::size(MythicPlusSeasonRewardLevelsFields),
    &MythicPlusSeasonRewardLevelsMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MythicPlusSeasonTrackedAffixFields[5] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "KeystoneAffixID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "BonusRating" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "Unknown_9_1_0_38511_004" },
    { .IsSigned = false, .Type = FT_INT, .Name = "DisplaySeasonID" }
};

constexpr DB2LoadInfo MythicPlusSeasonTrackedAffixLoadInfo
{
    MythicPlusSeasonTrackedAffixFields,
    std::size(MythicPlusSeasonTrackedAffixFields),
    &MythicPlusSeasonTrackedAffixMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MythicPlusSeasonTrackedMapFields[3] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true,  .Type = FT_INT, .Name = "MapChallengeModeID" },
    { .IsSigned = false, .Type = FT_INT, .Name = "DisplaySeasonID" }
};

constexpr DB2LoadInfo MythicPlusSeasonTrackedMapLoadInfo
{
    MythicPlusSeasonTrackedMapFields,
    std::size(MythicPlusSeasonTrackedMapFields),
    &MythicPlusSeasonTrackedMapMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta CraftingDataFields[16] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Type" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftingDifficultyID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftedItemID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ItemBonusTreeID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftingDifficulty" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "CraftSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "ReCraftSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "InspirationSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "Unknown2" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "Unknown3" },
    { .IsSigned = true , .Type = FT_INT, .Name = "FirstCraftFlagQuestID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "FirstCraftTreasureID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown4" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftedTreasureID" }
};

constexpr DB2LoadInfo CraftingDataLoadInfo
{
    CraftingDataFields,
    std::size(CraftingDataFields),
    &CraftingDataMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta CraftingDataItemQualityFields[3] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ItemID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftingDataID" }
};

constexpr DB2LoadInfo CraftingDataItemQualityLoadInfo
{
    CraftingDataItemQualityFields,
    std::size(CraftingDataItemQualityFields),
    &CraftingDataItemQualityMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta CraftingDifficultyFields[8] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "MaxRandomSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "CraftSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "ReCraftSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "InspirationSkillBonusPercent" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ConcentrationSkillCurveID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ConcentrationDifficultyCurveID" }
};

constexpr DB2LoadInfo CraftingDifficultyLoadInfo
{
    CraftingDifficultyFields,
    std::size(CraftingDifficultyFields),
    &CraftingDifficultyMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta CraftingDifficultyQualityFields[6] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Order" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftingQualityID" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "QualityPercentage" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "QualityMultiplier" },
    { .IsSigned = true , .Type = FT_INT, .Name = "CraftingDifficultyID" }
};

constexpr DB2LoadInfo CraftingDifficultyQualityLoadInfo
{
    CraftingDifficultyQualityFields,
    std::size(CraftingDifficultyQualityFields),
    &CraftingDifficultyQualityMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta CraftingReagentQualityFields[8] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "OrderIndex" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ItemID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "MaxDifficultyAdjustment" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "ReagentEffectPct" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown2" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ModifiedCraftingCategoryID" }
};

constexpr DB2LoadInfo CraftingReagentQualityLoadInfo
{
    CraftingReagentQualityFields,
    std::size(CraftingReagentQualityFields),
    &CraftingReagentQualityMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta ModifiedCraftingSpellSlotFields[7] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "SpellID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Slot" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ModifiedCraftingReagentSlotID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ReagentCount" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ReagentReCraftCount" }
};

constexpr DB2LoadInfo ModifiedCraftingSpellSlotLoadInfo
{
    ModifiedCraftingSpellSlotFields,
    std::size(ModifiedCraftingSpellSlotFields),
    &ModifiedCraftingSpellSlotMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta ModifiedCraftingReagentSlotFields[8] =
{
    { .IsSigned = true , .Type = FT_STRING, .Name = "Name" },
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_INT, .Name = "PlayerConditionID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ReagentType" },
    { .IsSigned = false, .Type = FT_BYTE, .Name = "Unknown2" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown3" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "Unknown4" }
};

constexpr DB2LoadInfo ModifiedCraftingReagentSlotLoadInfo
{
    ModifiedCraftingReagentSlotFields,
    std::size(ModifiedCraftingReagentSlotFields),
    &ModifiedCraftingReagentSlotMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta MCRSlotXMCRCategoryFields[4] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ModifiedCraftingCategoryID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Order" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ModifiedCraftingReagentSlotID" }
};

constexpr DB2LoadInfo MCRSlotXMCRCategoryLoadInfo
{
    MCRSlotXMCRCategoryFields,
    std::size(MCRSlotXMCRCategoryFields),
    &MCRSlotXMCRCategoryMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta ModifiedCraftingCategoryFields[6] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_STRING, .Name = "DisplayName" },
    { .IsSigned = true , .Type = FT_STRING, .Name = "Description" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_FLOAT, .Name = "MatQualityWeight" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown2" }
};

constexpr DB2LoadInfo ModifiedCraftingCategoryLoadInfo
{
    ModifiedCraftingCategoryFields,
    std::size(ModifiedCraftingCategoryFields),
    &ModifiedCraftingCategoryMeta::Instance,
    HotfixDatabaseStatements{}
};

constexpr DB2FieldMeta ModifiedCraftingReagentItemFields[7] =
{
    { .IsSigned = false, .Type = FT_INT, .Name = "ID" },
    { .IsSigned = true , .Type = FT_STRING, .Name = "Description" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ModifiedCraftingCategoryID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ItemBonusTreeID" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Flags" },
    { .IsSigned = true , .Type = FT_INT, .Name = "Unknown1" },
    { .IsSigned = true , .Type = FT_INT, .Name = "ItemContextOffset" }
};

constexpr DB2LoadInfo ModifiedCraftingReagentItemLoadInfo
{
    ModifiedCraftingReagentItemFields,
    std::size(ModifiedCraftingReagentItemFields),
    &ModifiedCraftingReagentItemMeta::Instance,
    HotfixDatabaseStatements{}
};

template <typename T>
bool LoadStore(DB2Storage<T>& store, std::string const& db2Path, LocaleConstant defaultLocale, std::vector<std::string>& errors)
{
    try
    {
        store.Load(db2Path + localeNames[defaultLocale] + '/', defaultLocale);
        return true;
    }
    catch (std::exception const& exception)
    {
        errors.emplace_back(Trinity::StringFormat("RetailSystems DB2: failed to load {}/{}: {}",
            localeNames[defaultLocale], store.GetFileName(), exception.what()));
        return false;
    }
}
}

DB2Storage<UIChromieTimeExpansionInfoEntry> UIChromieTimeExpansionInfoStore
{
    "UIChromieTimeExpansionInfo.db2",
    &UIChromieTimeExpansionInfoLoadInfo
};

DB2Storage<MapChallengeModeAffixCriteriaEntry> MapChallengeModeAffixCriteriaStore
{
    "MapChallengeModeAffixCriteria.db2",
    &MapChallengeModeAffixCriteriaLoadInfo
};

DB2Storage<MythicPlusSeasonKeyFloorEntry> MythicPlusSeasonKeyFloorStore
{
    "MythicPlusSeasonKeyFloor.db2",
    &MythicPlusSeasonKeyFloorLoadInfo
};

DB2Storage<MythicPlusSeasonRewardLevelsEntry> MythicPlusSeasonRewardLevelsStore
{
    "MythicPlusSeasonRewardLevels.db2",
    &MythicPlusSeasonRewardLevelsLoadInfo
};

DB2Storage<MythicPlusSeasonTrackedAffixEntry> MythicPlusSeasonTrackedAffixStore
{
    "MythicPlusSeasonTrackedAffix.db2",
    &MythicPlusSeasonTrackedAffixLoadInfo
};

DB2Storage<MythicPlusSeasonTrackedMapEntry> MythicPlusSeasonTrackedMapStore
{
    "MythicPlusSeasonTrackedMap.db2",
    &MythicPlusSeasonTrackedMapLoadInfo
};

DB2Storage<CraftingDataEntry> CraftingDataStore
{
    "CraftingData.db2",
    &CraftingDataLoadInfo
};

DB2Storage<CraftingDataItemQualityEntry> CraftingDataItemQualityStore
{
    "CraftingDataItemQuality.db2",
    &CraftingDataItemQualityLoadInfo
};

DB2Storage<CraftingDifficultyEntry> CraftingDifficultyStore
{
    "CraftingDifficulty.db2",
    &CraftingDifficultyLoadInfo
};

DB2Storage<CraftingDifficultyQualityEntry> CraftingDifficultyQualityStore
{
    "CraftingDifficultyQuality.db2",
    &CraftingDifficultyQualityLoadInfo
};

DB2Storage<CraftingReagentQualityEntry> CraftingReagentQualityStore
{
    "CraftingReagentQuality.db2",
    &CraftingReagentQualityLoadInfo
};

DB2Storage<ModifiedCraftingSpellSlotEntry> ModifiedCraftingSpellSlotStore
{
    "ModifiedCraftingSpellSlot.db2",
    &ModifiedCraftingSpellSlotLoadInfo
};

DB2Storage<ModifiedCraftingReagentSlotEntry> ModifiedCraftingReagentSlotStore
{
    "ModifiedCraftingReagentSlot.db2",
    &ModifiedCraftingReagentSlotLoadInfo
};

DB2Storage<MCRSlotXMCRCategoryEntry> MCRSlotXMCRCategoryStore
{
    "MCRSlotXMCRCategory.db2",
    &MCRSlotXMCRCategoryLoadInfo
};

DB2Storage<ModifiedCraftingCategoryEntry> ModifiedCraftingCategoryStore
{
    "ModifiedCraftingCategory.db2",
    &ModifiedCraftingCategoryLoadInfo
};

DB2Storage<ModifiedCraftingReagentItemEntry> ModifiedCraftingReagentItemStore
{
    "ModifiedCraftingReagentItem.db2",
    &ModifiedCraftingReagentItemLoadInfo
};

bool LoadStores(std::string const& db2Path, LocaleConstant defaultLocale, std::vector<std::string>& errors)
{
    ASSERT(UIChromieTimeExpansionInfoMeta::Instance.GetRecordSize() == sizeof(UIChromieTimeExpansionInfoEntry));
    ASSERT(MapChallengeModeAffixCriteriaMeta::Instance.GetRecordSize() == sizeof(MapChallengeModeAffixCriteriaEntry));
    ASSERT(MythicPlusSeasonKeyFloorMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonKeyFloorEntry));
    ASSERT(MythicPlusSeasonRewardLevelsMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonRewardLevelsEntry));
    ASSERT(MythicPlusSeasonTrackedAffixMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonTrackedAffixEntry));
    ASSERT(MythicPlusSeasonTrackedMapMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonTrackedMapEntry));
    ASSERT(CraftingDataMeta::Instance.GetRecordSize() == sizeof(CraftingDataEntry));
    ASSERT(CraftingDataItemQualityMeta::Instance.GetRecordSize() == sizeof(CraftingDataItemQualityEntry));
    ASSERT(CraftingDifficultyMeta::Instance.GetRecordSize() == sizeof(CraftingDifficultyEntry));
    ASSERT(CraftingDifficultyQualityMeta::Instance.GetRecordSize() == sizeof(CraftingDifficultyQualityEntry));
    ASSERT(CraftingReagentQualityMeta::Instance.GetRecordSize() == sizeof(CraftingReagentQualityEntry));
    ASSERT(ModifiedCraftingSpellSlotMeta::Instance.GetRecordSize() == sizeof(ModifiedCraftingSpellSlotEntry));
    ASSERT(ModifiedCraftingReagentSlotMeta::Instance.GetRecordSize() == sizeof(ModifiedCraftingReagentSlotEntry));
    ASSERT(MCRSlotXMCRCategoryMeta::Instance.GetRecordSize() == sizeof(MCRSlotXMCRCategoryEntry));
    ASSERT(ModifiedCraftingCategoryMeta::Instance.GetRecordSize() == sizeof(ModifiedCraftingCategoryEntry));
    ASSERT(ModifiedCraftingReagentItemMeta::Instance.GetRecordSize() == sizeof(ModifiedCraftingReagentItemEntry));

    std::string effectivePath = db2Path;
    if (std::string clientDataDir = sConfigMgr->GetStringDefault("RetailSystems.ClientDataDir", "", true); !clientDataDir.empty())
    {
        boost::filesystem::path path(clientDataDir);
        path /= "dbc";
        effectivePath = path.generic_string() + '/';
    }

    bool loaded = true;
    loaded &= LoadStore(UIChromieTimeExpansionInfoStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MapChallengeModeAffixCriteriaStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MythicPlusSeasonKeyFloorStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MythicPlusSeasonRewardLevelsStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MythicPlusSeasonTrackedAffixStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MythicPlusSeasonTrackedMapStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(CraftingDataStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(CraftingDataItemQualityStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(CraftingDifficultyStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(CraftingDifficultyQualityStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(CraftingReagentQualityStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(ModifiedCraftingSpellSlotStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(ModifiedCraftingReagentSlotStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(MCRSlotXMCRCategoryStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(ModifiedCraftingCategoryStore, effectivePath, defaultLocale, errors);
    loaded &= LoadStore(ModifiedCraftingReagentItemStore, effectivePath, defaultLocale, errors);
    return loaded;
}
}
