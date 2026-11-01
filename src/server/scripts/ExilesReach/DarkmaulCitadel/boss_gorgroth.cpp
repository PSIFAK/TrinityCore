/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "Conversation.h"
#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "TemporarySummon.h"
#include "darkmaul_citadel.h"

enum GorgrothSpells
{
    SPELL_SUMMON_GHOULS_RITUAL   = 306097,
    SPELL_GHOUL_SPAWN_VISUAL     = 1238235,
    SPELL_RITUAL_ACTIVATION      = 326862,
    SPELL_SHADOW_BOLT            = 305913,
    SPELL_DARK_COMMAND           = 308527,
    SPELL_DARK_COMMAND_MISSILE   = 308499,
    SPELL_DARK_COMMAND_DAMAGE    = 308502,
    SPELL_FINAL_SACRIFICE        = 306800,
    SPELL_FINAL_SACRIFICE_CHANNEL = 318234,
    SPELL_RAVNYR_TRANSFORMATION  = 318318
};

enum GorgrothEvents
{
    EVENT_SHADOW_BOLT = 1,
    EVENT_DARK_COMMAND
};

enum GorgrothMisc
{
    CONVERSATION_GORGROTH_COMBAT   = 12957,
    CONVERSATION_GORGROTH_DEFEATED = 13421,
    ACTOR_GORGROTH                 = 71723,
    EMOTE_STATE_GORGROTH_READY     = 425
};

// where the ghouls of the ritual rise (retail sniff 12.0.1.66709)
static constexpr Position GhoulPositions[] =
{
    { 834.599f, -1774.52f, 248.677f, 0.417195f },
    { 831.056f, -1780.48f, 248.684f, 4.28758f },
    { 832.458f, -1785.97f, 248.68f, 1.12279f },
    { 841.474f, -1772.5f, 248.671f, 5.32793f },
    { 836.812f, -1768.35f, 248.671f, 2.46004f }
};

// 156814 - Gor'groth
// Retail: when the group nears his hall he raises five ghouls; once they are dead he can be fought. At half health he
// sacrifices himself to awaken Ravnyr (the corrupted Kalecgos), who is the rest of the encounter.
struct boss_gorgroth : public BossAI
{
    boss_gorgroth(Creature* creature) : BossAI(creature, DATA_GORGROTH), _sacrificing(false), _ghoulsAlive(0) { }

    void Reset() override
    {
        _Reset();
        _sacrificing = false;

        if (instance->GetData(DATA_GORGROTH_RITUAL))
            SetAttackable();
        else
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC));
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_GORGROTH_RETURN)
        {
            // the group wiped on Ravnyr: retail brings Gor'groth back 30 seconds later
            scheduler.Schedule(30s, [this](TaskContext const& /*context*/)
            {
                _sacrificing = false;
                me->SetVisible(true);
                me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                me->CombatStop(true);
                Reset();
            });
            return;
        }

        if (action != ACTION_RAISE_GHOULS || _ghoulsAlive || instance->GetData(DATA_GORGROTH_RITUAL))
            return;

        DoCastSelf(SPELL_SUMMON_GHOULS_RITUAL);
        for (Position const& position : GhoulPositions)
            if (me->SummonCreature(NPC_DECAYING_CORPSE, position, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10s))
                ++_ghoulsAlive;
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);
        if (summon->GetEntry() != NPC_DECAYING_CORPSE)
            return;

        // they climb out of the ground and attack about 4 seconds later
        summon->CastSpell(summon, SPELL_GHOUL_SPAWN_VISUAL, true);
        summon->SetReactState(REACT_PASSIVE);
        ObjectGuid ghoulGuid = summon->GetGUID();
        scheduler.Schedule(4s, [this, ghoulGuid](TaskContext const& /*context*/)
        {
            Creature* ghoul = ObjectAccessor::GetCreature(*me, ghoulGuid);
            if (!ghoul || !ghoul->IsAlive())
                return;

            ghoul->SetReactState(REACT_AGGRESSIVE);
            if (Player* player = ghoul->SelectNearestPlayer(60.0f))
                ghoul->AI()->AttackStart(player);
        });
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() != NPC_DECAYING_CORPSE || !_ghoulsAlive)
            return;

        if (--_ghoulsAlive)
            return;

        instance->SetData(DATA_GORGROTH_RITUAL, 1);
        DoCastSelf(SPELL_RITUAL_ACTIVATION, true);
        SetAttackable();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        me->RemoveAurasDueToSpell(SPELL_RITUAL_ACTIVATION);
        me->SetEmoteState(EMOTE_ONESHOT_NONE);

        events.ScheduleEvent(EVENT_SHADOW_BOLT, 6s);
        events.ScheduleEvent(EVENT_DARK_COMMAND, 12s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
    {
        if (_sacrificing)
        {
            damage = 0;
            return;
        }

        if (!me->HealthBelowPctDamaged(50, damage))
            return;

        damage = 0;
        StartFinalSacrifice();
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (_sacrificing)
            return;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        if (instance->GetBossState(DATA_GORGROTH) == IN_PROGRESS)
            instance->SetBossState(DATA_GORGROTH, FAIL);
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
        if (_sacrificing || !UpdateVictim())
            return;

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SHADOW_BOLT:
                    DoCastVictim(SPELL_SHADOW_BOLT);
                    events.Repeat(8s);
                    break;
                case EVENT_DARK_COMMAND:
                    DoCastSelf(SPELL_DARK_COMMAND);
                    DoCastVictim(SPELL_DARK_COMMAND_MISSILE, true);
                    scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
                    {
                        if (!_sacrificing)
                            DoCastVictim(SPELL_DARK_COMMAND_DAMAGE, true);
                    });
                    events.Repeat(16s);
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
    void SetAttackable()
    {
        me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE));
        me->SetEmoteState(Emote(EMOTE_STATE_GORGROTH_READY));
        me->SetReactState(REACT_AGGRESSIVE);
    }

    // retail timings from the sniff, counted from the moment he reaches half health
    void StartFinalSacrifice()
    {
        _sacrificing = true;
        events.Reset();
        me->AttackStop();
        me->SetReactState(REACT_PASSIVE);
        me->GetMotionMaster()->Clear();
        me->StopMoving();

        Creature* ravnyr = instance->GetCreature(DATA_RAVNYR);
        if (ravnyr)
            DoCast(ravnyr, SPELL_FINAL_SACRIFICE);

        if (Conversation* conversation = Conversation::CreateConversation(CONVERSATION_GORGROTH_COMBAT, me, *me, ObjectGuid::Empty, nullptr, false))
        {
            conversation->AddActor(ACTOR_GORGROTH, 0, me->GetGUID());
            if (!conversation->Start())
                delete conversation;
        }

        scheduler.Schedule(2700ms, [this](TaskContext context)
        {
            me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            if (Creature* target = instance->GetCreature(DATA_RAVNYR))
                me->CastSpell(target, SPELL_FINAL_SACRIFICE_CHANNEL, true);

            if (context.GetRepeatCounter() < 10)
                context.Repeat(200ms);
        });

        scheduler.Schedule(10s, [this](TaskContext const& /*context*/)
        {
            if (Creature* target = instance->GetCreature(DATA_RAVNYR))
            {
                target->CastSpell(target, SPELL_RAVNYR_TRANSFORMATION, true);
                if (Conversation* conversation = Conversation::CreateConversation(CONVERSATION_GORGROTH_DEFEATED, target, *target, ObjectGuid::Empty, nullptr, false))
                    if (!conversation->Start())
                        delete conversation;
            }
        });

        scheduler.Schedule(15800ms, [this](TaskContext const& /*context*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            instance->SetBossState(DATA_GORGROTH, DONE);
            if (Creature* target = instance->GetCreature(DATA_RAVNYR))
                target->AI()->DoAction(ACTION_RAVNYR_AWAKEN);

            // retail despawns him; he is only hidden so that a wipe on Ravnyr can bring him back (ACTION_GORGROTH_RETURN)
            me->CombatStop(true);
            me->SetVisible(false);
        });
    }

    bool _sacrificing;
    uint8 _ghoulsAlive;
};

void AddSC_boss_gorgroth()
{
    RegisterDarkmaulCitadelCreatureAI(boss_gorgroth);
}
