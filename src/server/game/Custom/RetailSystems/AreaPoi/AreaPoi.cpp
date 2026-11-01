/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "AreaPoi.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "WorldSession.h"

namespace WorldPackets::AreaPoi
{
WorldPacket const* AreaPoiUpdateResponse::Write()
{
    _worldPacket << uint32(Pois.size());
    for (AreaPoiData const& poi : Pois)
    {
        _worldPacket << int64(poi.StartTime);
        _worldPacket << int32(poi.AreaPoiID);
        _worldPacket << int32(poi.DurationSec);
        _worldPacket << uint32(poi.WorldStateVariableID);
        _worldPacket << uint32(poi.WorldStateValue);
    }

    return &_worldPacket;
}
}

namespace RetailSystems::AreaPoi
{
namespace
{
struct ScheduledPoi
{
    int32 AreaPoiID = 0;
    time_t FirstStart = 0;
    int64 PeriodSec = 0;
    int32 DurationSec = 0;
    uint32 WorldStateVariableID = 0;
    uint32 WorldStateValue = 0;
};

std::vector<ScheduledPoi> Schedule;
}

void Load()
{
    Schedule.clear();
    QueryResult result = WorldDatabase.Query("SELECT `area_poi_id`, `first_start`, `period_sec`, `duration_sec`, `world_state_variable_id`, "
        "`world_state_value` FROM `retail_area_poi`");
    if (!result)
    {
        TC_LOG_INFO("server.loading", "RetailSystems: loaded 0 timed area POIs");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        ScheduledPoi poi;
        poi.AreaPoiID = fields[0].GetInt32();
        poi.FirstStart = time_t(fields[1].GetUInt64());
        poi.PeriodSec = fields[2].GetInt64();
        poi.DurationSec = fields[3].GetInt32();
        poi.WorldStateVariableID = fields[4].GetUInt32();
        poi.WorldStateValue = fields[5].GetUInt32();
        if (poi.PeriodSec <= 0 || poi.DurationSec <= 0 || poi.DurationSec > poi.PeriodSec)
        {
            TC_LOG_ERROR("sql.sql", "Table `retail_area_poi`: area POI {} has period {} and duration {}, skipped", poi.AreaPoiID, poi.PeriodSec, poi.DurationSec);
            continue;
        }

        Schedule.push_back(poi);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", "RetailSystems: loaded {} timed area POIs", Schedule.size());
}

std::vector<WorldPackets::AreaPoi::AreaPoiData> GetActive(time_t now)
{
    std::vector<WorldPackets::AreaPoi::AreaPoiData> active;
    for (ScheduledPoi const& poi : Schedule)
    {
        if (now < poi.FirstStart)
            continue;

        // the current cycle: active while its first DurationSec seconds run
        int64 intoCycle = int64(now - poi.FirstStart) % poi.PeriodSec;
        if (intoCycle >= poi.DurationSec)
            continue;

        WorldPackets::AreaPoi::AreaPoiData& data = active.emplace_back();
        data.StartTime = now - time_t(intoCycle);
        data.AreaPoiID = poi.AreaPoiID;
        data.DurationSec = poi.DurationSec;
        data.WorldStateVariableID = poi.WorldStateVariableID;
        data.WorldStateValue = poi.WorldStateValue;
    }

    return active;
}
}

void WorldSession::HandleRequestAreaPoiUpdate(WorldPackets::AreaPoi::RequestAreaPoiUpdate& /*packet*/)
{
    WorldPackets::AreaPoi::AreaPoiUpdateResponse response;
    response.Pois = RetailSystems::AreaPoi::GetActive(GameTime::GetGameTime());
    SendPacket(response.Write());
}
