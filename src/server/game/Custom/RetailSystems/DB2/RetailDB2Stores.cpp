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

bool LoadStores(std::string const& db2Path, LocaleConstant defaultLocale, std::vector<std::string>& errors)
{
    ASSERT(UIChromieTimeExpansionInfoMeta::Instance.GetRecordSize() == sizeof(UIChromieTimeExpansionInfoEntry));
    ASSERT(MapChallengeModeAffixCriteriaMeta::Instance.GetRecordSize() == sizeof(MapChallengeModeAffixCriteriaEntry));
    ASSERT(MythicPlusSeasonKeyFloorMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonKeyFloorEntry));
    ASSERT(MythicPlusSeasonRewardLevelsMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonRewardLevelsEntry));
    ASSERT(MythicPlusSeasonTrackedAffixMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonTrackedAffixEntry));
    ASSERT(MythicPlusSeasonTrackedMapMeta::Instance.GetRecordSize() == sizeof(MythicPlusSeasonTrackedMapEntry));

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
    return loaded;
}
}
