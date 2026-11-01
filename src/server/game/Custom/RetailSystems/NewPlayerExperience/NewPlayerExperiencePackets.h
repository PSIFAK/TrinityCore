/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_NEW_PLAYER_EXPERIENCE_PACKETS_H
#define TRINITYCORE_NEW_PLAYER_EXPERIENCE_PACKETS_H

#include "ObjectGuid.h"
#include "Packet.h"
#include <vector>

namespace WorldPackets::NewPlayerExperience
{
// per character restrictions, sent right after the character list. Layout from the retail sniff (12.1.5.70688): one byte,
// the character guid, the restriction mask, one more uint32 (1 in every sample). Bit 23 of the mask is what the client's
// C_PlayerInfo.IsPlayerNPERestricted / IsCharacterNPERestricted test: without it the new player tutorial never loads.
class RegionwideCharacterRestrictionsData final : public ServerPacket
{
public:
    struct CharacterRestriction
    {
        ObjectGuid Guid;
        uint32 RestrictionFlags = 0;
        uint32 Unknown = 1;
    };

    RegionwideCharacterRestrictionsData() : ServerPacket(SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA, 4) { }

    WorldPacket const* Write() override;

    std::vector<CharacterRestriction> Characters;
};
}

#endif // TRINITYCORE_NEW_PLAYER_EXPERIENCE_PACKETS_H
