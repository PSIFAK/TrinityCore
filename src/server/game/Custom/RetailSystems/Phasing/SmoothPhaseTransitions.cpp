/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SmoothPhaseTransitions.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SmoothPhasing.h"
#include <list>

namespace RetailSystems::Phasing
{
static constexpr float SameSpotDistance = 3.0f;

// Retail (12.1.5 sniff): 93 creates during the Exile's Reach quests carry ReplaceObject, always the same entry, most
// of them on the same spot (the copy of the next phase) and some up to ~120 yd away (an NPC that moved on).
SmoothPhaseTransition::SmoothPhaseTransition(Player* player) : _player(player)
{
    if (!player->IsInWorld())
        return;

    std::list<Creature*> creatures;
    player->GetCreatureListWithOptionsInGrid(creatures, player->GetVisibilityRange(), { .IgnorePhases = true });

    std::vector<Creature*> leaving;
    std::vector<Creature*> arriving;
    for (Creature* creature : creatures)
    {
        // personal copies and what they replace keep their own smooth phasing
        if (creature->IsPrivateObject())
            continue;

        if (SmoothPhasing const* smoothPhasing = creature->GetSmoothPhasing())
            if (smoothPhasing->GetInfoForSeer(player->GetGUID()))
                continue;

        bool atClient = player->HaveAtClient(creature);
        bool visible = player->CanSeeOrDetect(creature, { .DistanceCheck = true });
        if (atClient && !visible)
            leaving.push_back(creature);
        else if (!atClient && visible)
            arriving.push_back(creature);
    }

    // the copy of the same creature first (wherever it went), then another creature standing on the same spot (the
    // wounded Alaria that becomes the standing one, a camp NPC swapped for its next version)
    auto findReplaced = [&leaving](Creature const* creature)
    {
        auto nearest = leaving.end();
        for (auto itr = leaving.begin(); itr != leaving.end(); ++itr)
            if ((*itr)->GetEntry() == creature->GetEntry() && (nearest == leaving.end() || creature->GetExactDistSq(*itr) < creature->GetExactDistSq(*nearest)))
                nearest = itr;

        if (nearest != leaving.end())
            return nearest;

        for (auto itr = leaving.begin(); itr != leaving.end(); ++itr)
            if (creature->GetExactDistSq(*itr) <= SameSpotDistance * SameSpotDistance && (nearest == leaving.end() || creature->GetExactDistSq(*itr) < creature->GetExactDistSq(*nearest)))
                nearest = itr;

        return nearest;
    };

    for (Creature* creature : arriving)
    {
        auto nearest = findReplaced(creature);
        if (nearest == leaving.end())
            continue;

        // Disabled: the info is only written into the create packet; the creature itself stays visible
        SmoothPhasingInfo info{ (*nearest)->GetGUID(), true, true };
        info.Disabled = true;
        creature->GetOrCreateSmoothPhasing()->SetViewerDependentInfo(player->GetGUID(), info);
        _replacing.push_back(creature->GetGUID());
        leaving.erase(nearest);
    }
}

SmoothPhaseTransition::~SmoothPhaseTransition()
{
    for (ObjectGuid const& guid : _replacing)
        if (Creature* creature = ObjectAccessor::GetCreature(*_player, guid))
            if (SmoothPhasing* smoothPhasing = creature->GetSmoothPhasing())
                smoothPhasing->ClearViewerDependentInfo(_player->GetGUID());
}
}
