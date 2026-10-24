/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WorldQuestService.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "MapUtils.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include "Timer.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldStateMgr.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RetailSystems::WorldQuests
{
namespace
{
constexpr uint32 AREA_CHECK_INTERVAL = 1000;

struct Polygon
{
    std::vector<std::pair<float, float>> Points;

    bool Contains(float x, float y) const
    {
        bool inside = false;
        for (std::size_t i = 0, j = Points.size() - 1; i < Points.size(); j = i++)
        {
            auto const& [xi, yi] = Points[i];
            auto const& [xj, yj] = Points[j];
            if ((yi > y) != (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi)
                inside = !inside;
        }

        return inside;
    }
};

struct WorldQuest
{
    uint32 QuestID = 0;
    uint32 Duration = 0;
    int32 WorldStateID = 0;
    int32 WorldStateValue = 0;
    int64 StartTime = 0;
    Quest const* Template = nullptr;
    uint32 MapID = 0;
    std::vector<Polygon> Areas;

    time_t GetCycleStart(time_t now) const
    {
        if (!Duration)
            return time_t(StartTime);

        int64 elapsed = int64(now) - StartTime;
        int64 cycles = elapsed >= 0 ? elapsed / Duration : -((-elapsed + Duration - 1) / Duration);
        return time_t(StartTime + cycles * int64(Duration));
    }

    bool IsInArea(Player const* player) const
    {
        if (player->GetMapId() != MapID)
            return false;

        for (Polygon const& area : Areas)
            if (area.Contains(player->GetPositionX(), player->GetPositionY()))
                return true;

        return false;
    }
};

struct CharacterState
{
    std::unordered_map<uint32 /*questId*/, int64 /*cycleStart*/> CompletedCycles;
    std::unordered_set<uint32> AddedByArea;
};

bool Enabled = false;
std::unordered_map<uint32, WorldQuest> WorldQuests;
std::unordered_map<uint32 /*mapId*/, std::vector<WorldQuest const*>> AreaTasksByMap;
std::unordered_map<uint64, CharacterState> CharacterStates;
uint32 AreaCheckTimer = 0;

void LoadAreas(WorldQuest& worldQuest)
{
    QuestPOIData const* poiData = sObjectMgr->GetQuestPOIData(worldQuest.QuestID);
    if (!poiData)
        return;

    for (QuestPOIBlobData const& blob : poiData->Blobs)
    {
        if (blob.Points.size() < 3)
            continue;

        // all polygons of a task quest share its map; keep the first one
        if (worldQuest.Areas.empty())
            worldQuest.MapID = uint32(blob.MapID);
        else if (worldQuest.MapID != uint32(blob.MapID))
            continue;

        Polygon& polygon = worldQuest.Areas.emplace_back();
        for (QuestPOIBlobPoint const& point : blob.Points)
            polygon.Points.emplace_back(float(point.X), float(point.Y));
    }
}

void RemoveAreaTask(Player* player, CharacterState& state, uint32 questId)
{
    state.AddedByArea.erase(questId);

    if (player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
        return;

    player->RemoveActiveQuest(questId);
    player->TakeQuestSourceItem(questId, true);
    player->AbandonQuest(questId);
    player->DespawnPersonalSummonsForQuest(questId);
}

void UpdateAreaTasks(Player* player, CharacterState& state, time_t now)
{
    // leave the areas of quests added earlier
    for (auto itr = state.AddedByArea.begin(); itr != state.AddedByArea.end();)
    {
        uint32 questId = *itr++;
        WorldQuest const* worldQuest = Trinity::Containers::MapGetValuePtr(WorldQuests, questId);
        if (!worldQuest || !worldQuest->IsInArea(player))
            RemoveAreaTask(player, state, questId);
    }

    std::vector<WorldQuest const*> const* areaTasks = Trinity::Containers::MapGetValuePtr(AreaTasksByMap, player->GetMapId());
    if (!areaTasks)
        return;

    for (WorldQuest const* worldQuest : *areaTasks)
    {
        if (!worldQuest->IsInArea(player))
            continue;

        QuestStatus status = player->GetQuestStatus(worldQuest->QuestID);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
            continue;

        if (status == QUEST_STATUS_REWARDED)
        {
            // completed in an older cycle -> the quest was reset on retail
            int64 const* completedCycle = Trinity::Containers::MapGetValuePtr(state.CompletedCycles, worldQuest->QuestID);
            if (completedCycle && *completedCycle >= int64(worldQuest->GetCycleStart(now)))
                continue;

            player->RemoveRewardedQuest(worldQuest->QuestID);
        }

        if (!player->CanTakeQuest(worldQuest->Template, false))
            continue;

        player->AddQuestAndCheckCompletion(worldQuest->Template, nullptr);
        if (player->GetQuestStatus(worldQuest->QuestID) == QUEST_STATUS_INCOMPLETE)
            state.AddedByArea.insert(worldQuest->QuestID);
    }
}
}

void Initialize()
{
    WorldQuests.clear();
    AreaTasksByMap.clear();

    Enabled = sConfigMgr->GetBoolDefault("RetailSystems.WorldQuests.Enabled", true);
    if (!Enabled)
        return;

    uint32 oldMSTime = getMSTime();

    //                                                   0         1           2               3              4
    QueryResult result = WorldDatabase.Query("SELECT `QuestID`, `Duration`, `WorldStateID`, `WorldStateValue`, `StartTime` FROM `retail_world_quest`");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 RetailSystems world quests. Table `retail_world_quest` is empty.");
        return;
    }

    uint32 areaTaskCount = 0;
    do
    {
        Field* fields = result->Fetch();

        WorldQuest worldQuest;
        worldQuest.QuestID = fields[0].GetUInt32();
        worldQuest.Duration = fields[1].GetUInt32();
        worldQuest.WorldStateID = fields[2].GetInt32();
        worldQuest.WorldStateValue = fields[3].GetInt32();
        worldQuest.StartTime = fields[4].GetInt64();
        worldQuest.Template = sObjectMgr->GetQuestTemplate(worldQuest.QuestID);
        if (!worldQuest.Template)
        {
            TC_LOG_ERROR("sql.sql", "Table `retail_world_quest` references non-existing quest {}, skipped.", worldQuest.QuestID);
            continue;
        }

        if (worldQuest.Template->GetQuestType() == QUEST_TYPE_TASK)
            LoadAreas(worldQuest);

        WorldStateMgr::SetValue(worldQuest.WorldStateID, worldQuest.WorldStateValue, false, nullptr);
        WorldQuests.emplace(worldQuest.QuestID, std::move(worldQuest));
    } while (result->NextRow());

    for (auto const& [questId, worldQuest] : WorldQuests)
    {
        if (worldQuest.Areas.empty())
            continue;

        AreaTasksByMap[worldQuest.MapID].push_back(&worldQuest);
        ++areaTaskCount;
    }

    TC_LOG_INFO("server.loading", ">> Loaded {} RetailSystems world quests ({} area tasks) in {} ms", WorldQuests.size(), areaTaskCount, GetMSTimeDiffToNow(oldMSTime));
}

void Update(uint32 diff)
{
    if (!Enabled || AreaTasksByMap.empty())
        return;

    if (AreaCheckTimer > diff)
    {
        AreaCheckTimer -= diff;
        return;
    }

    AreaCheckTimer = AREA_CHECK_INTERVAL;
    time_t now = GameTime::GetGameTime();

    for (auto const& [_, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld() || player->IsBeingTeleported() || !player->IsAlive())
            continue;

        auto stateItr = CharacterStates.find(player->GetGUID().GetCounter());
        if (stateItr == CharacterStates.end())
            continue;

        UpdateAreaTasks(player, stateItr->second, now);
    }
}

void FillUpdateResponse(WorldPackets::Quest::WorldQuestUpdateResponse& response)
{
    if (!Enabled)
        return;

    time_t now = GameTime::GetGameTime();
    response.WorldQuestUpdates.reserve(WorldQuests.size());
    for (auto const& [questId, worldQuest] : WorldQuests)
        response.WorldQuestUpdates.emplace_back(worldQuest.GetCycleStart(now), questId, worldQuest.Duration, worldQuest.WorldStateID, worldQuest.WorldStateValue);
}

void OnLogin(Player* player)
{
    if (!Enabled)
        return;

    CharacterState& state = CharacterStates[player->GetGUID().GetCounter()];
    state = {};

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `QuestID`, `CycleStart` FROM `retail_character_world_quest` WHERE `guid` = {}", player->GetGUID().GetCounter()))
    {
        do
        {
            Field* fields = result->Fetch();
            state.CompletedCycles[fields[0].GetUInt32()] = fields[1].GetInt64();
        } while (result->NextRow());
    }

    // area tasks left in the quest log (e.g. logout inside the area) are tracked again
    for (auto const& [questId, worldQuest] : WorldQuests)
        if (!worldQuest.Areas.empty() && player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
            state.AddedByArea.insert(questId);
}

void OnLogout(Player* player)
{
    CharacterStates.erase(player->GetGUID().GetCounter());
}

void OnQuestStatusChange(Player* player, uint32 questId)
{
    if (!Enabled)
        return;

    WorldQuest const* worldQuest = Trinity::Containers::MapGetValuePtr(WorldQuests, questId);
    if (!worldQuest || player->GetQuestStatus(questId) != QUEST_STATUS_REWARDED)
        return;

    int64 cycleStart = int64(worldQuest->GetCycleStart(GameTime::GetGameTime()));
    auto stateItr = CharacterStates.find(player->GetGUID().GetCounter());
    if (stateItr != CharacterStates.end())
    {
        stateItr->second.CompletedCycles[questId] = cycleStart;
        stateItr->second.AddedByArea.erase(questId);
    }

    CharacterDatabase.PExecute("REPLACE INTO `retail_character_world_quest` (`guid`, `QuestID`, `CycleStart`) VALUES ({}, {}, {})",
        player->GetGUID().GetCounter(), questId, cycleStart);
}

void RemoveCharacterState(uint64 characterGuid)
{
    CharacterStates.erase(characterGuid);
    CharacterDatabase.PExecute("DELETE FROM `retail_character_world_quest` WHERE `guid` = {}", characterGuid);
}
}
