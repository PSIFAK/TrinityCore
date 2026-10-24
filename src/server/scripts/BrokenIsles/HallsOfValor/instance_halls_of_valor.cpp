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
#include "Player.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "halls_of_valor.h"

static constexpr ObjectData creatureData[] =
{
    { BOSS_HYMDALL,              DATA_HYMDALL               },
    { BOSS_HYRJA,                DATA_HYRJA                 },
    { BOSS_FENRYR,               DATA_FENRYR                },
    { BOSS_GOD_KING_SKOVALD,     DATA_GOD_KING_SKOVALD      },
    { BOSS_ODYN,                 DATA_ODYN                  },
    { NPC_OLMYR_THE_ENLIGHTENED, DATA_OLMYR_THE_ENLIGHTENED },
    { NPC_SOLSTEN,               DATA_SOLSTEN               },
    { NPC_AEGIS_OF_AGGRAMAR,     DATA_AEGIS_OF_AGGRAMAR     }
};

static constexpr ObjectData gameObjectData[] =
{
    { GO_RUNIC_BRAND_PURE,   DATA_RUNIC_BRAND_PURE   },
    { GO_RUNIC_BRAND_RED,    DATA_RUNIC_BRAND_RED    },
    { GO_RUNIC_BRAND_YELLOW, DATA_RUNIC_BRAND_YELLOW },
    { GO_RUNIC_BRAND_BLUE,   DATA_RUNIC_BRAND_BLUE   },
    { GO_RUNIC_BRAND_GREEN,  DATA_RUNIC_BRAND_GREEN  }
};

static constexpr DoorData doorData[] =
{
    { GO_HYMDALL_ENTRANCE_DOOR, DATA_HYMDALL,          EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_HYMDALL_EXIT_DOOR,     DATA_HYMDALL,          EncounterDoorBehavior::OpenWhenDone          },
    { GO_HYRJA_DOOR,            DATA_HYRJA,            EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_GATEWAY_OF_SPLENDOR,   DATA_HYRJA,            EncounterDoorBehavior::OpenWhenDone          },
    { GO_GATEWAY_OF_SPLENDOR,   DATA_FENRYR,           EncounterDoorBehavior::OpenWhenDone          },
    { GO_ODYN_DOOR,             DATA_GOD_KING_SKOVALD, EncounterDoorBehavior::OpenWhenNotInProgress },
    { GO_ODYN_DOOR,             DATA_ODYN,             EncounterDoorBehavior::OpenWhenNotInProgress }
};

static constexpr DungeonEncounterData encounters[] =
{
    { DATA_HYMDALL,          {{ 1805 }} },
    { DATA_HYRJA,            {{ 1806 }} },
    { DATA_FENRYR,           {{ 1807 }} },
    { DATA_GOD_KING_SKOVALD, {{ 1808 }} },
    { DATA_ODYN,             {{ 1809 }} }
};

uint32 const KingsCount = 4;

class instance_halls_of_valor : public InstanceMapScript
{
public:
    instance_halls_of_valor() : InstanceMapScript(HOVScriptName, 1477) { }

    struct instance_halls_of_valor_InstanceMapScript : public InstanceScript
    {
        instance_halls_of_valor_InstanceMapScript(InstanceMap* map) : InstanceScript(map),
            _fenryrEvent(*this, "FenryrEvent", NOT_STARTED),
            _kingsDefeated(*this, "KingsDefeated", 0),
            _skovaldIntro(*this, "SkovaldIntro", NOT_STARTED)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadObjectData(creatureData, gameObjectData);
            LoadDoorData(doorData);
            LoadDungeonEncounterData(encounters);
        }

        void OnPlayerEnter(Player* /*player*/) override
        {
            // Bosses that are summoned by scripted events must be restored after an instance reload
            SummonFenryrIfNeeded();
            SummonSkovaldIfNeeded();
        }

        void OnUnitDeath(Unit* unit) override
        {
            Creature* creature = unit->ToCreature();
            if (!creature)
                return;

            switch (creature->GetEntry())
            {
                case NPC_KING_HALDOR:
                case NPC_KING_BJORN:
                case NPC_KING_RANULF:
                case NPC_KING_TOR:
                    if (_kingsDefeated < KingsCount)
                    {
                        _kingsDefeated = _kingsDefeated + 1;
                        if (_kingsDefeated >= KingsCount)
                            SummonSkovaldIfNeeded();
                    }
                    break;
                default:
                    break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case DATA_HYRJA:
                case DATA_FENRYR:
                    if (state == DONE && GetBossState(DATA_HYRJA) == DONE && GetBossState(DATA_FENRYR) == DONE)
                        DoCastSpellOnPlayers(SPELL_ODYNS_BLESSING);
                    break;
                case DATA_GOD_KING_SKOVALD:
                    if (state == DONE)
                    {
                        if (Creature* aegis = GetCreature(DATA_AEGIS_OF_AGGRAMAR))
                            aegis->DespawnOrUnsummon();

                        if (Creature* odyn = GetCreature(DATA_ODYN))
                            odyn->AI()->DoAction(ACTION_ODYN_INTRO);
                    }
                    else if (state == NOT_STARTED || state == FAIL)
                        SpawnAegis();
                    break;
                case DATA_ODYN:
                    if (state != IN_PROGRESS)
                        for (uint8 i = 0; i < MaxRunicBrands; ++i)
                            if (GameObject* rune = GetGameObject(DATA_RUNIC_BRAND_PURE + i))
                                rune->SetGoState(GO_STATE_READY);
                    break;
                default:
                    break;
            }

            return true;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_FENRYR_EVENT:
                    if (_fenryrEvent == DONE)
                        break;
                    _fenryrEvent = data;
                    SummonFenryrIfNeeded();
                    break;
                case DATA_SKOVALD_INTRO:
                    _skovaldIntro = data;
                    break;
                case DATA_RUNE_ACTIVATED:
                    if (data < MaxRunicBrands)
                        if (GameObject* rune = GetGameObject(DATA_RUNIC_BRAND_PURE + data))
                            rune->SetGoState(GO_STATE_ACTIVE);
                    break;
                case DATA_RUNE_DEACTIVATED:
                    if (data < MaxRunicBrands)
                    {
                        if (GameObject* rune = GetGameObject(DATA_RUNIC_BRAND_PURE + data))
                            rune->SetGoState(GO_STATE_READY);

                        if (Creature* odyn = GetCreature(DATA_ODYN))
                            odyn->AI()->SetData(DATA_RUNE_DEACTIVATED, data);
                    }
                    break;
                default:
                    break;
            }
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_FENRYR_EVENT:
                    return _fenryrEvent;
                case DATA_KINGS_EVENT:
                    return _kingsDefeated >= KingsCount ? DONE : NOT_STARTED;
                case DATA_SKOVALD_INTRO:
                    return _skovaldIntro;
                default:
                    break;
            }
            return 0;
        }

    private:
        void SummonFenryrIfNeeded()
        {
            if (_fenryrEvent != DONE || GetBossState(DATA_FENRYR) == DONE || GetCreature(DATA_FENRYR))
                return;

            // Boss Fenryr has no spawn in the world database, he waits in his lair once the trash version fled
            instance->SummonCreature(BOSS_FENRYR, FenryrLairPos);
        }

        void SummonSkovaldIfNeeded()
        {
            if (_kingsDefeated < KingsCount || GetBossState(DATA_GOD_KING_SKOVALD) == DONE || GetCreature(DATA_GOD_KING_SKOVALD))
                return;

            // God-King Skovald has no spawn in the world database, he arrives once all kings are defeated
            if (instance->SummonCreature(BOSS_GOD_KING_SKOVALD, SkovaldArrivePos))
                SpawnAegis();
        }

        void SpawnAegis()
        {
            if (GetBossState(DATA_GOD_KING_SKOVALD) == DONE)
                return;

            if (Creature* aegis = GetCreature(DATA_AEGIS_OF_AGGRAMAR))
                aegis->DespawnOrUnsummon();

            // Odyn's original Aegis toss (193940) targets a nearby-entry destination without data, place the Aegis at the hall center instead
            instance->SummonCreature(NPC_AEGIS_OF_AGGRAMAR, HOVHallCenterPos);
        }

        PersistentInstanceScriptValue<uint32> _fenryrEvent;
        PersistentInstanceScriptValue<uint32> _kingsDefeated;
        PersistentInstanceScriptValue<uint32> _skovaldIntro;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_halls_of_valor_InstanceMapScript(map);
    }
};

void AddSC_instance_halls_of_valor()
{
    new instance_halls_of_valor();
}
