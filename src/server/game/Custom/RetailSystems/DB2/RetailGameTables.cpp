/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RetailGameTables.h"
#include "Errors.h"
#include "Log.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Util.h"
#include <boost/filesystem/path.hpp>
#include <cmath>
#include <fstream>

namespace RetailSystems::DB2
{
GameTable<ChallengeModeScalingEntry> ChallengeModeDamageTable;
GameTable<ChallengeModeScalingEntry> ChallengeModeHealthTable;

namespace
{
bool LoadTable(GameTable<ChallengeModeScalingEntry>& storage, boost::filesystem::path const& path)
{
    std::ifstream stream(path.string());
    if (!stream)
    {
        TC_LOG_WARN("server.loading", "RetailSystems: optional GameTable {} is missing; Mythic+ will use its fallback scaling", path.string());
        return false;
    }

    std::string headers;
    if (!std::getline(stream, headers))
    {
        TC_LOG_ERROR("server.loading", "RetailSystems: GameTable {} is empty", path.string());
        return false;
    }

    std::vector<std::string_view> columnDefs = Trinity::Tokenize(headers, '\t', false);
    if (columnDefs.size() != 2)
    {
        TC_LOG_ERROR("server.loading", "RetailSystems: GameTable {} has {} columns, expected 2", path.string(), columnDefs.size());
        return false;
    }

    std::vector<ChallengeModeScalingEntry> data(1);
    std::string line;
    while (std::getline(stream, line))
    {
        RemoveCRLF(line);
        std::vector<std::string_view> values = Trinity::Tokenize(line, '\t', true);
        if (values.size() < 2 || values[0].empty())
            break;

        data.push_back({ Trinity::StringTo<float>(values[1], 10).value_or(1.0f) });
    }

    storage.SetData(std::move(data));
    TC_LOG_INFO("server.loading", "RetailSystems: loaded {}", path.string());
    return true;
}

float GetScalar(GameTable<ChallengeModeScalingEntry> const& table, uint32 level)
{
    if (ChallengeModeScalingEntry const* row = table.GetRow(level))
        if (row->Scalar > 0.0f)
            return row->Scalar;

    // The fallback is only used when the extractor has not produced the two client GameTables.
    return std::pow(1.10f, float(level > 2 ? level - 2 : 0));
}
}

bool LoadGameTables(std::string const& dataPath)
{
    boost::filesystem::path root(dataPath);
    root /= "gt";
    bool const damageLoaded = LoadTable(ChallengeModeDamageTable, root / "ChallengeModeDamage.txt");
    bool const healthLoaded = LoadTable(ChallengeModeHealthTable, root / "ChallengeModeHealth.txt");
    return damageLoaded && healthLoaded;
}

float GetChallengeDamageScalar(uint32 level)
{
    return GetScalar(ChallengeModeDamageTable, level);
}

float GetChallengeHealthScalar(uint32 level)
{
    return GetScalar(ChallengeModeHealthTable, level);
}
}
