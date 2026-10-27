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
    CONVERSATION_GORGROTH_COMBAT   = 12957,
    CONVERSATION_GORGROTH_DEFEATED = 13421,
    CONVERSATION_RAVNYR_AWAKENS    = 12958
};

enum DarkmaulCitadelConversationActors
{
    ACTOR_GORGROTH                 = 71723,
    ACTOR_RAVNYR                   = 71232
};

enum DarkmaulCitadelMisc
{
    NPC_SHADOWY_ORB                = 156828
};

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

        void OnPlayerEnter(Player* /*player*/) override
        {
            if (_entranceStarted)
                return;

            _entranceStarted = true;
            _scheduler.Schedule(6s, [this](TaskContext const& /*context*/)
            {
                if (Player* player = GetAnyPlayer())
                    StartConversation(CONVERSATION_ENTRANCE, player);
            });
        }

        void OnUnitDeath(Unit* unit) override
        {
            if (unit->GetEntry() == NPC_SHADOWY_ORB && !_shadowyOrbDone)
            {
                _shadowyOrbDone = true;
                StartConversation(CONVERSATION_SHADOWY_ORB, unit);
            }
        }

        bool SetBossState(uint32 id, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

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
                case DATA_GORGROTH:
                    if (state == IN_PROGRESS)
                    {
                        StartConversation(CONVERSATION_GORGROTH_ENGAGE, boss);
                        _scheduler.Schedule(19s, [this](TaskContext const& /*context*/)
                        {
                            if (GetBossState(DATA_GORGROTH) == IN_PROGRESS)
                                if (Creature* gorgroth = GetCreature(DATA_GORGROTH))
                                    StartConversation(CONVERSATION_GORGROTH_COMBAT, gorgroth, { { ACTOR_GORGROTH, 0, gorgroth } });
                        });
                    }
                    else if (state == DONE)
                        StartConversation(CONVERSATION_GORGROTH_DEFEATED, boss);
                    break;
                case DATA_RAVNYR:
                    if (state == IN_PROGRESS)
                        StartConversation(CONVERSATION_RAVNYR_AWAKENS, boss, { { ACTOR_RAVNYR, 0, boss } });
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

            if (!_gorgrothApproached && GetBossState(DATA_TUNK) == DONE && GetBossState(DATA_GORGROTH) == NOT_STARTED)
                if (Creature* gorgroth = GetCreature(DATA_GORGROTH))
                    if (IsPlayerNear(gorgroth))
                    {
                        _gorgrothApproached = true;
                        StartConversation(CONVERSATION_GORGROTH_APPROACH, gorgroth);
                    }
        }

    private:
        struct ActorRef
        {
            int32 ActorId;
            uint32 Index;
            Creature* Actor;
        };

        Player* GetAnyPlayer() const
        {
            for (MapReference const& ref : instance->GetPlayers())
                if (Player* player = ref.GetSource())
                    return player;
            return nullptr;
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
        static void StartConversation(uint32 conversationId, Unit* creator, std::initializer_list<ActorRef> actors = {})
        {
            Conversation* conversation = Conversation::CreateConversation(conversationId, creator, *creator, ObjectGuid::Empty, nullptr, false);
            if (!conversation)
                return;

            for (ActorRef const& actor : actors)
                conversation->AddActor(actor.ActorId, actor.Index, actor.Actor->GetGUID());

            if (!conversation->Start())
                delete conversation;
        }

        TaskScheduler _scheduler;
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
