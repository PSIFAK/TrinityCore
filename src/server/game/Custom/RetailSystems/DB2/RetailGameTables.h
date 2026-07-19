/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_GAME_TABLES_H
#define TRINITYCORE_RETAIL_GAME_TABLES_H

#include "Define.h"
#include "GameTables.h"
#include <string>

namespace RetailSystems::DB2
{
struct ChallengeModeScalingEntry
{
    float Scalar = 1.0f;
};

extern GameTable<ChallengeModeScalingEntry> ChallengeModeDamageTable;
extern GameTable<ChallengeModeScalingEntry> ChallengeModeHealthTable;

bool LoadGameTables(std::string const& dataPath);
float GetChallengeDamageScalar(uint32 level);
float GetChallengeHealthScalar(uint32 level);
}

#endif // TRINITYCORE_RETAIL_GAME_TABLES_H
