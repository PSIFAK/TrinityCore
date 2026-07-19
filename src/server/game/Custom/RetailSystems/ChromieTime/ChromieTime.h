/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_CHROMIE_TIME_H
#define TRINITYCORE_RETAIL_CHROMIE_TIME_H

#include "Define.h"

class Player;

namespace RetailSystems::ChromieTime
{
TC_GAME_API bool Apply(Player* player, uint32 expansionId, bool persist);
TC_GAME_API bool Select(Player* player, uint32 expansionId);
TC_GAME_API void Restore(Player* player);
TC_GAME_API void RemoveCharacterState(uint64 characterGuid);
}

#endif // TRINITYCORE_RETAIL_CHROMIE_TIME_H
