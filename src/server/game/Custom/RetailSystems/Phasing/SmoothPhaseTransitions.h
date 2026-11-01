/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_SMOOTH_PHASE_TRANSITIONS_H
#define TRINITYCORE_RETAIL_SMOOTH_PHASE_TRANSITIONS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>

class Player;

namespace RetailSystems::Phasing
{
// When a phase change swaps a creature for its copy in the new phase (same entry), retail creates the copy with
// smooth phasing (ReplaceObject = the old one) and the client cross-fades them instead of popping. Lives around the
// player's visibility update of a phase change: pairs are found before it, the info is cleared after it.
class SmoothPhaseTransition
{
public:
    explicit SmoothPhaseTransition(Player* player);
    ~SmoothPhaseTransition();

    SmoothPhaseTransition(SmoothPhaseTransition const&) = delete;
    SmoothPhaseTransition& operator=(SmoothPhaseTransition const&) = delete;

private:
    Player* _player;
    std::vector<ObjectGuid> _replacing;
};
}

#endif // TRINITYCORE_RETAIL_SMOOTH_PHASE_TRANSITIONS_H
