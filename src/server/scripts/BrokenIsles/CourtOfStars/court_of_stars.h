/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEF_COURT_OF_STARS_H_
#define DEF_COURT_OF_STARS_H_

#include "CreatureAIImpl.h"

#define DataHeader "CourtOfStars"
#define COSScriptName "instance_court_of_stars"

constexpr uint32 EncounterCount = 3;

enum COSDataTypes : uint32
{
    // Encounters
    DATA_PATROL_CAPTAIN_GERDO   = 0,
    DATA_TALIXAE_FLAMEWREATH    = 1,
    DATA_ADVISOR_MELANDRUS      = 2,

    // Creatures
    DATA_JAZSHARIU,
    DATA_BAALGAR_THE_WATCHFUL,
    DATA_IMACUTYA,

    // Misc
    DATA_BEACON_ACTIVATE,           // SetData: every arcane beacon that is not disabled calls for help
    DATA_TALIXAE_GUARDS_KILLED,     // GetData: number of Fel Covenant guards killed
    DATA_SPY_EVENT,                 // Get/SetData: state of the "find the spy" event
    DATA_SPY_GUID                   // GetGuidData: the suspicious noble that is the spy
};

enum COSCreatureIds
{
    // Bosses
    BOSS_PATROL_CAPTAIN_GERDO   = 104215,
    BOSS_TALIXAE_FLAMEWREATH    = 104217,
    BOSS_ADVISOR_MELANDRUS      = 104218,

    // Patrol Captain Gerdo
    NPC_ARCANE_BEACON           = 104245,
    NPC_VIGILANT_DUSKWATCH      = 104918,
    NPC_STREETSWEEPER           = 110403,

    // Talixae Flamewreath
    NPC_JAZSHARIU               = 104273,
    NPC_BAALGAR_THE_WATCHFUL    = 104274,
    NPC_IMACUTYA                = 104275,
    NPC_FELBOUND_ENFORCER       = 104278,

    // Advisor Melandrus
    NPC_IMAGE_OF_MELANDRUS      = 105754,
    NPC_ENVELOPING_WINDS        = 112687,

    // Misc
    NPC_SUSPICIOUS_NOBLE        = 107435,
    NPC_NIGHTBORNE_BOAT         = 105264
};

enum COSGameObjectIds
{
    GO_WAYPOINT_DOOR            = 251849,
    GO_MANOR_DOOR               = 251847,
    GO_SKYWARD_TERRACE_DOOR     = 251848
};

enum COSSharedSpells
{
    SPELL_BEACON_DISABLED       = 210257,
    SPELL_SUMMON_VIGILANT_NIGHTWATCH = 210435
};

template <class AI, class T>
inline AI* GetCourtOfStarsAI(T* obj)
{
    return GetInstanceAI<AI>(obj, COSScriptName);
}

#define RegisterCourtOfStarsCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetCourtOfStarsAI)

#endif // DEF_COURT_OF_STARS_H_
