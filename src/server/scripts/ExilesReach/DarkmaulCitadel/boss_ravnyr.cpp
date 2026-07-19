/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InstanceScript.h"
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
    SPELL_KALECGOS_RESTORED       = 1238361
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

        me->SetUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC));
        DoCastSelf(SPELL_RAVNYR_TRANSFORMATION, true);

        if (instance && instance->GetBossState(DATA_GORGROTH) == DONE)
            me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE));
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        DoCastSelf(SPELL_RAVNYR_ENGAGE, true);
        DoCastSelf(SPELL_RAVNYR_ENGAGE_VISUAL, true);

        events.ScheduleEvent(EVENT_DARK_BREATH, 7s);
        events.ScheduleEvent(EVENT_TAIL_SWIPE, 17s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_defeated || damage < me->GetHealth())
            return;

        damage = 0;
        _defeated = true;
        events.Reset();
        me->AttackStop();
        me->CombatStop(true);
        me->SetReactState(REACT_PASSIVE);
        me->SetHealth(me->GetMaxHealth());
        me->RemoveAllAuras();
        me->SetUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC));
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
        DoCastSelf(SPELL_RAVNYR_DEFEATED, true);

        for (Map::PlayerList::const_iterator itr = me->GetMap()->GetPlayers().begin(); itr != me->GetMap()->GetPlayers().end(); ++itr)
            if (Player* player = itr->GetSource())
            {
                player->UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_CRITERIA_TREE, 89016, 1);
                player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
            }

        scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
        {
            me->SetFaction(FACTION_FRIENDLY);
            me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE));
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            DoCastSelf(SPELL_KALECGOS_RESTORED, true);
        });
    }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (!_defeated || menuId != GOSSIP_MENU_KALECGOS_DARKMAUL || gossipListId != 0 ||
            player->GetQuestStatus(QUEST_DUNGEON_DARKMAUL_CITADEL) != QUEST_STATUS_INCOMPLETE)
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        player->KilledMonsterCredit(NPC_DARKMAUL_EXIT_CREDIT);
        PhasingHandler::OnConditionChange(player);
        player->TeleportTo(2175, 721.7f, -1838.4f, 186.5f, 4.326671f, TELE_TO_NONE, {}, SPELL_DARKMAUL_EXIT_TELEPORT);
        return true;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (_defeated)
            return;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
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
                    events.Repeat(18s);
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
