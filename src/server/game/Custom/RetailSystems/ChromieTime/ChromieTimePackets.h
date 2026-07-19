/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_CHROMIE_TIME_PACKETS_H
#define TRINITYCORE_CHROMIE_TIME_PACKETS_H

#include "ObjectGuid.h"
#include "Packet.h"

namespace WorldPackets::ChromieTime
{
class SelectExpansion final : public ClientPacket
{
public:
    explicit SelectExpansion(WorldPacket&& packet) : ClientPacket(CMSG_CHROMIE_TIME_SELECT_EXPANSION, std::move(packet)) { }

    void Read() override;

    ObjectGuid NpcGUID;
    uint32 ExpansionID = 0;
};

class SelectExpansionSuccess final : public ServerPacket
{
public:
    SelectExpansionSuccess() : ServerPacket(SMSG_CHROMIE_TIME_SELECT_EXPANSION_SUCCESS, 0) { }

    WorldPacket const* Write() override { return &_worldPacket; }
};
}

#endif // TRINITYCORE_CHROMIE_TIME_PACKETS_H
