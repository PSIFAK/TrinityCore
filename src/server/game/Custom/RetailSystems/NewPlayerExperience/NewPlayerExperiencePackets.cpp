/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "NewPlayerExperiencePackets.h"

namespace WorldPackets::NewPlayerExperience
{
WorldPacket const* RegionwideCharacterRestrictionsData::Write()
{
    _worldPacket << uint32(Characters.size());
    for (CharacterRestriction const& character : Characters)
    {
        _worldPacket << uint8(0);
        _worldPacket << character.Guid;
        _worldPacket << uint32(character.RestrictionFlags);
        _worldPacket << uint32(character.Unknown);
    }

    return &_worldPacket;
}
}
