/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_CHALLENGE_MODE_SERVICE_H
#define TRINITYCORE_RETAIL_CHALLENGE_MODE_SERVICE_H

#include "Define.h"
#include "ObjectGuid.h"

class Map;
class Player;
class Unit;

namespace RetailSystems::ChallengeMode
{
void Initialize();
void Update(uint32 diff);

bool Start(Player* player, uint8 bag, uint32 slot, ObjectGuid const& receptacleGuid);
void Reset(Player* player, bool cheat);

void RestoreKeystones(Player* player);
void RemoveCharacterState(uint64 characterGuid);
void SendActiveRun(Player* player);

void SendCurrentAffixes(Player* player);
void SendSeasonData(Player* player);
void SendMapStats(Player* player);
void SendLeaders(Player* player, uint32 mapId, uint32 challengeId, uint64 lastGuildUpdate, uint64 lastRealmUpdate);

bool IsActive(Map const* map);
uint32 GetLevel(Map const* map);
void ScaleDamage(Unit* attacker, Unit* victim, uint32& damage);
}

#endif // TRINITYCORE_RETAIL_CHALLENGE_MODE_SERVICE_H
