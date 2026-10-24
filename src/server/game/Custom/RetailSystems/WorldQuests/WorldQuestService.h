/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_WORLD_QUEST_SERVICE_H
#define TRINITYCORE_RETAIL_WORLD_QUEST_SERVICE_H

#include "Define.h"

class Player;

namespace WorldPackets::Quest
{
class WorldQuestUpdateResponse;
}

// World quests / task quests driven by `retail_world_quest` (world DB):
// - the quest is visible while realm world state WorldStateID == WorldStateValue
// - each quest runs in cycles of Duration seconds, phased by the captured retail StartTime
// - task quests (QuestType 3) are added while the player is inside one of the quest's quest_poi polygons
//   and removed again when the player leaves before completing them
// - a quest rewarded in an older cycle becomes available again after the reset
namespace RetailSystems::WorldQuests
{
TC_GAME_API void Initialize();
TC_GAME_API void Update(uint32 diff);
TC_GAME_API void FillUpdateResponse(WorldPackets::Quest::WorldQuestUpdateResponse& response);

TC_GAME_API void OnLogin(Player* player);
TC_GAME_API void OnLogout(Player* player);
TC_GAME_API void OnQuestStatusChange(Player* player, uint32 questId);
TC_GAME_API void RemoveCharacterState(uint64 characterGuid);
}

#endif // TRINITYCORE_RETAIL_WORLD_QUEST_SERVICE_H
