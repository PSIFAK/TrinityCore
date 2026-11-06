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

#include "ScriptMgr.h"
#include "Conversation.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Player.h"
#include "TaskScheduler.h"
#include "darkmaul_citadel.h"

// conversations of the companions and bosses, timed as in the retail sniff (12.0.1.66709)
enum DarkmaulCitadelConversations
{
    CONVERSATION_ENTRANCE          = 12934,
    CONVERSATION_SHADOWY_ORB       = 12938,
    CONVERSATION_TUNK_APPROACH     = 12939,
    CONVERSATION_TUNK_ENGAGE       = 12945,
    CONVERSATION_TUNK_DEFEATED     = 12946,
    CONVERSATION_GORGROTH_APPROACH = 12953,
    CONVERSATION_GORGROTH_ENGAGE   = 12956,
    ACTOR_GORGROTH_ENGAGE          = 73447
};

enum DarkmaulCitadelMisc
{
    NPC_SHADOWY_ORB                = 156828,
    NPC_DARKMAUL_SHADOWCALLER      = 156821,
    SPELL_SHADOWY_ORB_RELEASED     = 306537,
    SPELL_NECROTIC_WEATHER         = 314675,   // fog and screen effect until Ravnyr is defeated
    SPELL_WRATHION_PERIODIC_ATTACK = 1259806,
    SPELL_WRATHION_TRANSFORM       = 373400
};

// retail starts Gor'groth's ritual when the group reaches the stairs before his hall
static constexpr Position GorgrothApproachPosition = { 884.3f, -1858.2f, 249.0f };
static constexpr float GorgrothApproachDistance = 20.0f;
static constexpr Position WrathionLandPosition = { 820.7f, -1757.0f, 248.7f };
static constexpr Position WrathionWalkPosition = { 831.4f, -1776.8f, 248.7f };

// the shadowcallers channelling the orb in the first room
static constexpr float ShadowcallerSearchDistance = 40.0f;

static constexpr float ConversationApproachDistance = 40.0f;

static constexpr ObjectData creatureData[] =
{
    { BOSS_TUNK,     DATA_TUNK     },
    { BOSS_GORGROTH, DATA_GORGROTH },
    { BOSS_RAVNYR,   DATA_RAVNYR   },
};

static constexpr DoorData doorData[] =
{
    { GO_TEMP_DOOR, DATA_TUNK, EncounterDoorBehavior::OpenWhenDone },
};

static constexpr int32 WORLD_STATE_ENCOUNTER_IN_PROGRESS = 18063;

static constexpr DungeonEncounterData encounters[] =
{
    { DATA_TUNK,   {{ 2325 }} },
    { DATA_RAVNYR, {{ 2326 }} }
};

class instance_darkmaul_citadel : public InstanceMapScript
{
public:
    instance_darkmaul_citadel() : InstanceMapScript(DCScriptName, 2236) { }

    struct instance_darkmaul_citadel_InstanceMapScript: public InstanceScript
    {
        instance_darkmaul_citadel_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
            LoadObjectData(creatureData, {});
            LoadDoorData(doorData);
            LoadDungeonEncounterData(encounters);
        }

        void OnPlayerEnter(Player* player) override
        {
            if (GetBossState(DATA_RAVNYR) != DONE)
                player->CastSpell(player, SPELL_NECROTIC_WEATHER, true);

            if (_entranceStarted)
                return;

            _entranceStarted = true;
            _scheduler.Schedule(6s, [this](TaskContext const& /*context*/)
            {
                if (Player* player = GetAnyPlayer())
                    StartConversation(CONVERSATION_ENTRANCE, player);
            });
        }

        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);
            if (creature->GetEntry() == NPC_SHADOWY_ORB)
                _shadowyOrbGuid = creature->GetGUID();
            else if (creature->GetEntry() == NPC_WRATHION)
                _wrathionGuid = creature->GetGUID();
        }

        uint32 GetData(uint32 type) const override
        {
            return type == DATA_GORGROTH_RITUAL ? uint32(_gorgrothRitualDone) : 0;
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_GORGROTH_RITUAL)
                _gorgrothRitualDone = data != 0;
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            InstanceScript::OnGameObjectCreate(go);
            if (go->GetEntry() == GO_GORGROTH_DOOR)
                _gorgrothDoorGuid = go->GetGUID();

            if (go->GetEntry() != GO_SHADOWY_DOOR)
                return;

            _shadowyDoorGuid = go->GetGUID();
            if (_shadowyOrbDone || GetBossState(DATA_TUNK) != NOT_STARTED)
                go->SetGoState(GO_STATE_ACTIVE);
        }

        // retail: once the last shadowcaller of the first room dies, the orb is released, the door to the
        // citadel opens and the companions comment on the orb a moment later
        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetEntry() != NPC_DARKMAUL_SHADOWCALLER || _shadowyOrbDone)
                return;

            Creature* orb = instance->GetCreature(_shadowyOrbGuid);
            if (!orb || !orb->IsWithinDist(unit, ShadowcallerSearchDistance))
                return;

            std::list<Creature*> shadowcallers;
            orb->GetCreatureListWithEntryInGrid(shadowcallers, NPC_DARKMAUL_SHADOWCALLER, ShadowcallerSearchDistance);
            for (Creature const* shadowcaller : shadowcallers)
                if (shadowcaller->IsAlive())
                    return;

            _shadowyOrbDone = true;
            orb->CastSpell(orb, SPELL_SHADOWY_ORB_RELEASED, true);
            if (GameObject* door = instance->GetGameObject(_shadowyDoorGuid))
                door->SetGoState(GO_STATE_ACTIVE);

            _scheduler.Schedule(3s, [this](TaskContext const& /*context*/)
            {
                if (Creature* releasedOrb = instance->GetCreature(_shadowyOrbGuid))
                    StartConversation(CONVERSATION_SHADOWY_ORB, releasedOrb);
            });
        }

        bool SetBossState(uint32 id, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            // retail (12.1.5 sniff): world state 18063 is 1 from the pull to the end of an encounter; Gor'groth and Ravnyr are one
            if (state == IN_PROGRESS || state == FAIL || (state == DONE && id != DATA_GORGROTH))
                DoUpdateWorldState(WORLD_STATE_ENCOUNTER_IN_PROGRESS, state == IN_PROGRESS ? 1 : 0);

            Creature* boss = GetCreature(id);
            if (!boss)
                return true;

            switch (id)
            {
                case DATA_TUNK:
                    if (state == IN_PROGRESS)
                        StartConversation(CONVERSATION_TUNK_ENGAGE, boss);
                    else if (state == DONE)
                        StartConversation(CONVERSATION_TUNK_DEFEATED, boss);
                    break;
                // Gor'groth and Ravnyr are one encounter in retail: the hall door closes 4.5 seconds after the pull and
                // opens again on a wipe or once Ravnyr is defeated; a wipe brings both back
                case DATA_GORGROTH:
                    if (state == IN_PROGRESS)
                    {
                        // encounter 2326 is Ravnyr's, but retail starts it with Gor'groth
                        SendDungeonEncounterStart(GetBossDungeonEncounter(DATA_RAVNYR));
                        StartConversation(CONVERSATION_GORGROTH_ENGAGE, boss);
                        _scheduler.Schedule(4500ms, [this](TaskContext const& /*context*/)
                        {
                            if (GetBossState(DATA_GORGROTH) == IN_PROGRESS)
                                SetGorgrothDoor(false);
                        });
                    }
                    else if (state == FAIL)
                    {
                        SendDungeonEncounterEnd(GetBossDungeonEncounter(DATA_RAVNYR), false);
                        SetGorgrothDoor(true);
                    }
                    break;
                case DATA_RAVNYR:
                    if (state == FAIL)
                    {
                        SetGorgrothDoor(true);
                        if (GetBossState(DATA_GORGROTH) == DONE)
                        {
                            InstanceScript::SetBossState(DATA_GORGROTH, NOT_STARTED);
                            if (Creature* gorgroth = GetCreature(DATA_GORGROTH))
                                gorgroth->AI()->DoAction(ACTION_GORGROTH_RETURN);
                        }
                    }
                    else if (state == DONE)
                    {
                        SetGorgrothDoor(true);
                        for (MapReference const& ref : instance->GetPlayers())
                            if (Player* player = ref.GetSource())
                                player->RemoveAurasDueToSpell(SPELL_NECROTIC_WEATHER);
                        StartWrathionEnding();
                    }
                    break;
                default:
                    break;
            }

            return true;
        }

        void Update(uint32 diff) override
        {
            _scheduler.Update(diff);

            _approachCheckTimer += diff;
            if (_approachCheckTimer < 1000)
                return;
            _approachCheckTimer = 0;

            // the companions comment once the group comes near the next boss
            if (!_tunkApproached && GetBossState(DATA_TUNK) == NOT_STARTED)
                if (Creature* tunk = GetCreature(DATA_TUNK))
                    if (IsPlayerNear(tunk))
                    {
                        _tunkApproached = true;
                        StartConversation(CONVERSATION_TUNK_APPROACH, tunk);
                    }

            // the companions comment on the hall, Gor'groth raises his ghouls 5 seconds later
            if (!_gorgrothApproached && GetBossState(DATA_TUNK) == DONE && GetBossState(DATA_GORGROTH) == NOT_STARTED)
                if (Creature* gorgroth = GetCreature(DATA_GORGROTH))
                    if (IsPlayerNear(GorgrothApproachPosition, GorgrothApproachDistance))
                    {
                        _gorgrothApproached = true;
                        StartConversation(CONVERSATION_GORGROTH_APPROACH, gorgroth);
                        _scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
                        {
                            if (Creature* raiser = GetCreature(DATA_GORGROTH))
                                raiser->AI()->DoAction(ACTION_RAISE_GHOULS);
                        });
                    }

            // released spirits get the fog back when they return
            if (GetBossState(DATA_RAVNYR) != DONE)
                for (MapReference const& ref : instance->GetPlayers())
                    if (Player* player = ref.GetSource())
                        if (player->IsAlive() && !player->HasAura(SPELL_NECROTIC_WEATHER))
                            player->CastSpell(player, SPELL_NECROTIC_WEATHER, true);
        }

    private:
        Player* GetAnyPlayer() const
        {
            for (MapReference const& ref : instance->GetPlayers())
                if (Player* player = ref.GetSource())
                    return player;
            return nullptr;
        }

        bool IsPlayerNear(Position const& position, float distance) const
        {
            for (MapReference const& ref : instance->GetPlayers())
                if (Player* player = ref.GetSource())
                    if (player->IsAlive() && player->IsWithinDist3d(&position, distance))
                        return true;
            return false;
        }

        void SetGorgrothDoor(bool open)
        {
            if (GameObject* door = instance->GetGameObject(_gorgrothDoorGuid))
                door->SetGoState(open ? GO_STATE_ACTIVE : GO_STATE_READY);
        }

        // retail: Wrathion stops circling, lands next to Kalecgos, takes his visage and walks over
        void StartWrathionEnding()
        {
            Creature* wrathion = instance->GetCreature(_wrathionGuid);
            if (!wrathion)
                return;

            wrathion->RemoveAurasDueToSpell(SPELL_WRATHION_PERIODIC_ATTACK);
            wrathion->GetMotionMaster()->Clear();
            wrathion->GetMotionMaster()->MovePoint(0, WrathionLandPosition);
            _scheduler.Schedule(5800ms, [this](TaskContext const& /*context*/)
            {
                Creature* landed = instance->GetCreature(_wrathionGuid);
                if (!landed)
                    return;

                landed->SetDisableGravity(false);
                landed->CastSpell(landed, SPELL_WRATHION_TRANSFORM, true);
                landed->GetMotionMaster()->MovePoint(0, WrathionWalkPosition);
            });
        }

        bool IsPlayerNear(Creature const* creature) const
        {
            for (MapReference const& ref : instance->GetPlayers())
                if (Player* player = ref.GetSource())
                    if (player->IsAlive() && player->IsWithinDist(creature, ConversationApproachDistance))
                        return true;
            return false;
        }

        // actors listed in conversation_actors are filled in by the core
        static void StartConversation(uint32 conversationId, Unit* creator)
        {
            Conversation* conversation = Conversation::CreateConversation(conversationId, creator, *creator, ObjectGuid::Empty, nullptr, false);
            if (!conversation)
                return;

            // retail (12.1.5 sniff) sends the Gor'groth pull line with an actor that has no object
            if (conversationId == CONVERSATION_GORGROTH_ENGAGE)
                conversation->AddActor(ACTOR_GORGROTH_ENGAGE, 0, ObjectGuid::Empty);

            if (!conversation->Start())
                delete conversation;
        }

        TaskScheduler _scheduler;
        ObjectGuid _shadowyOrbGuid;
        ObjectGuid _shadowyDoorGuid;
        ObjectGuid _gorgrothDoorGuid;
        ObjectGuid _wrathionGuid;
        bool _gorgrothRitualDone = false;
        uint32 _approachCheckTimer = 0;
        bool _entranceStarted = false;
        bool _shadowyOrbDone = false;
        bool _tunkApproached = false;
        bool _gorgrothApproached = false;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_darkmaul_citadel_InstanceMapScript(map);
    }
};

void AddSC_instance_darkmaul_citadel()
{
    new instance_darkmaul_citadel();
}
