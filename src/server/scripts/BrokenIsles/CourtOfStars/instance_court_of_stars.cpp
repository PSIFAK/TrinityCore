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

#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectGuid.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "court_of_stars.h"

static constexpr ObjectData creatureData[] =
{
    { BOSS_PATROL_CAPTAIN_GERDO, DATA_PATROL_CAPTAIN_GERDO },
    { BOSS_TALIXAE_FLAMEWREATH,  DATA_TALIXAE_FLAMEWREATH  },
    { BOSS_ADVISOR_MELANDRUS,    DATA_ADVISOR_MELANDRUS    },
    { NPC_JAZSHARIU,             DATA_JAZSHARIU            },
    { NPC_BAALGAR_THE_WATCHFUL,  DATA_BAALGAR_THE_WATCHFUL },
    { NPC_IMACUTYA,              DATA_IMACUTYA             },
};

// The original script also gated GO_SKYWARD_TERRACE_DOOR on the spy event. The spy event depends on
// the Masquerade disguise which nothing in our data hands out yet, so the door only follows Melandrus.
static constexpr DoorData doorData[] =
{
    { GO_WAYPOINT_DOOR,        DATA_PATROL_CAPTAIN_GERDO, EncounterDoorBehavior::OpenWhenDone          },
    { GO_MANOR_DOOR,           DATA_TALIXAE_FLAMEWREATH,  EncounterDoorBehavior::OpenWhenDone          },
    { GO_SKYWARD_TERRACE_DOOR, DATA_ADVISOR_MELANDRUS,    EncounterDoorBehavior::OpenWhenNotInProgress },
};

static constexpr DungeonEncounterData encounters[] =
{
    { DATA_PATROL_CAPTAIN_GERDO, {{ 1868 }} },
    { DATA_TALIXAE_FLAMEWREATH,  {{ 1869 }} },
    { DATA_ADVISOR_MELANDRUS,    {{ 1870 }} }
};

// Spots the Fel Covenant guards move to when a Felbound Enforcer dies nearby
static constexpr Position GuardCheckPositions[3] =
{
    { 1133.87f, 3278.71f, 20.10f },
    { 1156.15f, 3327.12f, 20.90f },
    { 1053.71f, 3357.99f, 19.84f }
};

// Order in which the guards answer a dead enforcer
static constexpr uint32 GuardDataOrder[3] =
{
    DATA_IMACUTYA,
    DATA_BAALGAR_THE_WATCHFUL,
    DATA_JAZSHARIU
};

static constexpr uint32 SPELL_BOND_OF_FLAME = 209723;
static constexpr uint32 TALIXAE_GUARD_COUNT = 3;
static constexpr uint32 POINT_GUARD_ENFORCER = 1;

class instance_court_of_stars : public InstanceMapScript
{
public:
    instance_court_of_stars() : InstanceMapScript(COSScriptName, 1571) { }

    struct instance_court_of_stars_InstanceMapScript : public InstanceScript
    {
        instance_court_of_stars_InstanceMapScript(InstanceMap* map) : InstanceScript(map),
            _talixaeGuardsKilled(*this, "TalixaeGuardsKilled", 0),
            _spyEvent(*this, "SpyEvent", NOT_STARTED),
            _guardsSent(0), _noblesSeen(0)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadObjectData(creatureData, {});
            LoadDoorData(doorData);
            LoadDungeonEncounterData(encounters);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);

            switch (creature->GetEntry())
            {
                case NPC_ARCANE_BEACON:
                    _beaconGUIDs.push_back(creature->GetGUID());
                    break;
                case NPC_SUSPICIOUS_NOBLE:
                    // Reservoir sampling: every noble loaded before the event starts has the same chance to be the spy
                    if (_spyEvent == NOT_STARTED)
                    {
                        ++_noblesSeen;
                        if (urand(1, _noblesSeen) == 1)
                            _spyGUID = creature->GetGUID();
                    }
                    break;
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            Creature* creature = unit->ToCreature();
            if (!creature)
                return;

            switch (creature->GetEntry())
            {
                case NPC_JAZSHARIU:
                case NPC_BAALGAR_THE_WATCHFUL:
                case NPC_IMACUTYA:
                    if (_talixaeGuardsKilled < TALIXAE_GUARD_COUNT)
                    {
                        _talixaeGuardsKilled = _talixaeGuardsKilled + 1;
                        if (_talixaeGuardsKilled >= TALIXAE_GUARD_COUNT)
                            if (Creature* talixae = GetCreature(DATA_TALIXAE_FLAMEWREATH))
                                talixae->RemoveAurasDueToSpell(SPELL_BOND_OF_FLAME);
                    }
                    break;
                case NPC_FELBOUND_ENFORCER:
                    SendGuardToEnforcer(creature);
                    break;
                default:
                    break;
            }
        }

        void SendGuardToEnforcer(Creature const* enforcer)
        {
            for (uint8 i = _guardsSent; i < TALIXAE_GUARD_COUNT; ++i)
            {
                Creature* guard = GetCreature(GuardDataOrder[i]);
                if (!guard || !guard->IsAlive() || guard->IsInCombat())
                    continue;

                Position dest = enforcer->GetPosition();
                for (Position const& checkPos : GuardCheckPositions)
                {
                    if (enforcer->GetExactDist2d(checkPos) < 40.0f)
                    {
                        dest = checkPos;
                        break;
                    }
                }

                _guardsSent = uint8(i + 1);
                guard->SetHomePosition(dest);
                guard->GetMotionMaster()->MovePoint(POINT_GUARD_ENFORCER, dest);
                return;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_BEACON_ACTIVATE:
                    for (ObjectGuid const& guid : _beaconGUIDs)
                        if (Creature* beacon = instance->GetCreature(guid))
                            if (beacon->IsAlive() && !beacon->HasAura(SPELL_BEACON_DISABLED))
                                beacon->CastSpell(beacon, SPELL_SUMMON_VIGILANT_NIGHTWATCH, true);
                    break;
                case DATA_SPY_EVENT:
                    _spyEvent = data;
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_TALIXAE_GUARDS_KILLED:
                    return _talixaeGuardsKilled;
                case DATA_SPY_EVENT:
                    return _spyEvent;
                default:
                    break;
            }
            return 0;
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            if (type == DATA_SPY_GUID)
                return _spyGUID;

            return InstanceScript::GetGuidData(type);
        }

    private:
        PersistentInstanceScriptValue<uint32> _talixaeGuardsKilled;
        PersistentInstanceScriptValue<uint32> _spyEvent;
        GuidVector _beaconGUIDs;
        ObjectGuid _spyGUID;
        uint8 _guardsSent;
        uint32 _noblesSeen;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_court_of_stars_InstanceMapScript(map);
    }
};

void AddSC_instance_court_of_stars()
{
    new instance_court_of_stars();
}
