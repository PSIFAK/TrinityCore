/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_AREA_POI_H
#define TRINITYCORE_RETAIL_AREA_POI_H

#include "Define.h"
#include "Packet.h"
#include <ctime>
#include <vector>

// Timed area POIs (world events on the map: Dreamsurge, invasion bosses, surges...). The client asks for the active
// ones (CMSG_REQUEST_AREA_POI_UPDATE) when the map opens and every minute; the server answers with their start time,
// duration and world state. The set comes from `retail_area_poi` (world DB), each entry repeating with its period.
namespace WorldPackets::AreaPoi
{
class RequestAreaPoiUpdate final : public ClientPacket
{
public:
    explicit RequestAreaPoiUpdate(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_AREA_POI_UPDATE, std::move(packet)) { }

    void Read() override { }
};

struct AreaPoiData
{
    time_t StartTime = 0;
    int32 AreaPoiID = 0;
    int32 DurationSec = 0;
    uint32 WorldStateVariableID = 0;
    uint32 WorldStateValue = 0;
};

// layout from the retail sniff (12.1.5.70688): count, then 24 bytes per POI
class AreaPoiUpdateResponse final : public ServerPacket
{
public:
    AreaPoiUpdateResponse() : ServerPacket(SMSG_AREA_POI_UPDATE_RESPONSE, 4) { }

    WorldPacket const* Write() override;

    std::vector<AreaPoiData> Pois;
};
}

namespace RetailSystems::AreaPoi
{
TC_GAME_API void Load();
TC_GAME_API std::vector<WorldPackets::AreaPoi::AreaPoiData> GetActive(time_t now);
}

#endif // TRINITYCORE_RETAIL_AREA_POI_H
