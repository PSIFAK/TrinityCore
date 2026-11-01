/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Conversation.h"
#include "Group.h"
#include "InstanceScript.h"
#include "LFGMgr.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "darkmaul_citadel.h"

enum RavnyrSpells
{
    SPELL_RAVNYR_TRANSFORMATION   = 318318,
    SPELL_RAVNYR_ENGAGE           = 321403,
    SPELL_RAVNYR_ENGAGE_VISUAL    = 1238244,
    SPELL_DARK_BREATH             = 305515,
    SPELL_TAIL_SWIPE              = 305567,
    SPELL_RAVNYR_DEFEATED         = 1238253,
    SPELL_KALECGOS_RESTORED       = 1238361,
    SPELL_DETECT_PLAYER           = 274651,
    SPELL_NECROTIC_STORM          = 321904
};

enum RavnyrMisc
{
    SAY_WARNING_DARK_BREATH       = 0,
    CONVERSATION_RAVNYR_AWAKENS   = 12958,
    ACTOR_RAVNYR                  = 71232,
    ANIM_KIT_RAVNYR_AWAKEN        = 11062
};

enum RavnyrEvents
{
    EVENT_DARK_BREATH = 1,
    EVENT_TAIL_SWIPE
};

constexpr uint32 QUEST_DUNGEON_DARKMAUL_CITADEL = 55992;
constexpr uint32 NPC_DARKMAUL_EXIT_CREDIT = 161350;
constexpr uint32 GOSSIP_MENU_KALECGOS_DARKMAUL = 39497;
constexpr uint32 SPELL_DARKMAUL_EXIT_TELEPORT = 319030;
constexpr uint32 LFG_DUNGEON_DARKMAUL_CITADEL = 2043;

// 156501 - Ravnyr / Kalecgos
struct boss_ravnyr : public BossAI
{
    boss_ravnyr(Creature* creature) : BossAI(creature, DATA_RAVNYR), _defeated(false) { }

    void Reset() override
    {
        _Reset();
        me->RestoreFaction();
        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);

        _defeated = instance && instance->GetBossState(DATA_RAVNYR) == DONE;
        if (_defeated)
        {
            me->RemoveAllAuras();
            me->SetFaction(FACTION_FRIENDLY);
            me->SetReactState(REACT_PASSIVE);
            me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE));
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            DoCastSelf(SPELL_KALECGOS_RESTORED, true);
            return;
        }

        // asleep until Gor'groth's final sacrifice awakens him (ACTION_RAVNYR_AWAKEN)
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC));
    }

    void DoAction(int32 action) override
    {
        if (action != ACTION_RAVNYR_AWAKEN || _defeated)
            return;

        me->SetAIAnimKitId(0);
        me->PlayOneShotAnimKitId(ANIM_KIT_RAVNYR_AWAKEN);
        DoCastSelf(SPELL_DETECT_PLAYER, true);
        DoCastSelf(SPELL_RAVNYR_ENGAGE, true);
        DoCastSelf(SPELL_RAVNYR_ENGAGE_VISUAL, true);
        me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE));

        if (Conversation* conversation = Conversation::CreateConversation(CONVERSATION_RAVNYR_AWAKENS, me, *me, ObjectGuid::Empty, nullptr, false))
        {
            conversation->AddActor(ACTOR_RAVNYR, 0, me->GetGUID());
            if (!conversation->Start())
                delete conversation;
        }

        // retail: he attacks on his own about 5 seconds later
        scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
        {
            if (Player* player = me->SelectNearestPlayer(100.0f))
                AttackStart(player);
        });
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 2);
        if (Creature* bunny = me->FindNearestCreature(NPC_STORM_BUNNY, 100.0f))
            bunny->CastSpell(bunny, SPELL_NECROTIC_STORM, true);

        events.ScheduleEvent(EVENT_DARK_BREATH, 2300ms);
        events.ScheduleEvent(EVENT_TAIL_SWIPE, 15500ms);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_DARK_BREATH && target->IsPlayer())
            Talk(SAY_WARNING_DARK_BREATH, target);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        // retail: defeated at half health
        if (_defeated || !me->HealthBelowPctDamaged(50, damage))
            return;

        damage = 0;
        _defeated = true;
        events.Reset();
        me->AttackStop();
        me->CombatStop(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetHealth(me->GetMaxHealth());
        me->RemoveAllAuras();
        me->SetFaction(FACTION_FRIENDLY);
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC));
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
        DoCastSelf(SPELL_RAVNYR_DEFEATED, true);

        for (Map::PlayerList::const_iterator itr = me->GetMap()->GetPlayers().begin(); itr != me->GetMap()->GetPlayers().end(); ++itr)
            if (Player* player = itr->GetSource())
            {
                player->UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_CRITERIA_TREE, 89016, 1);
                player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());

                // LFGDungeons.db2 names no final encounter for the citadel: the dungeon finder learns it is done from here
                if (Group* group = player->GetGroup())
                    if (group->isLFGGroup())
                        sLFGMgr->FinishDungeon(group->GetGUID(), LFG_DUNGEON_DARKMAUL_CITADEL, me->GetMap());
            }

        scheduler.Schedule(7s, [this](TaskContext const& /*context*/)
        {
            me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE));
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            DoCastSelf(SPELL_KALECGOS_RESTORED, true);
        });
    }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (!_defeated || menuId != GOSSIP_MENU_KALECGOS_DARKMAUL || gossipListId != 0)
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        if (player->GetQuestStatus(QUEST_DUNGEON_DARKMAUL_CITADEL) == QUEST_STATUS_INCOMPLETE)
        {
            player->KilledMonsterCredit(NPC_DARKMAUL_EXIT_CREDIT);
            PhasingHandler::OnConditionChange(player);
        }

        // retail: the player casts the teleport to the island; leaving the map ends the one-player dungeon finder party
        player->CastSpell(player, SPELL_DARKMAUL_EXIT_TELEPORT, true);
        return true;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (_defeated)
            return;

        // a wipe resets the whole encounter: Gor'groth comes back too
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        instance->SetBossState(DATA_RAVNYR, FAIL);
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
        if (_defeated || !UpdateVictim())
            return;

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DARK_BREATH:
                    DoCastVictim(SPELL_DARK_BREATH);
                    events.Repeat(23s);
                    break;
                case EVENT_TAIL_SWIPE:
                    DoCastSelf(SPELL_TAIL_SWIPE);
                    events.Repeat(22s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        me->DoMeleeAttackIfReady();
    }

private:
    bool _defeated;
};

void AddSC_boss_ravnyr()
{
    RegisterDarkmaulCitadelCreatureAI(boss_ravnyr);
}
