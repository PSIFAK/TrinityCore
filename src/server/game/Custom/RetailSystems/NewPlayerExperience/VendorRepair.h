/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_VENDOR_REPAIR_H
#define TRINITYCORE_RETAIL_VENDOR_REPAIR_H

#include "Define.h"

namespace RetailSystems::NewPlayerExperience
{
// CreatureAI::SetGUID id sent to a vendor's AI with the player's guid after CMSG_REPAIR_ITEM
// (Exile's Reach "Repair Yourself" is credited by repairing at the quartermaster)
constexpr int32 DATA_PLAYER_REPAIRED_ITEMS = 0x52505231;
}

#endif // TRINITYCORE_RETAIL_VENDOR_REPAIR_H
