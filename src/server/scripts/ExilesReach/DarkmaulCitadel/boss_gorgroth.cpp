/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "darkmaul_citadel.h"

enum GorgrothSpells
{
    SPELL_SUMMON_GHOULS_RITUAL   = 306097,
    SPELL_RITUAL_ACTIVATION      = 326862,
    SPELL_SHADOW_BOLT            = 305913,
    SPELL_DARK_COMMAND           = 308527,
    SPELL_DARK_COMMAND_MISSILE   = 308499,
    SPELL_DARK_COMMAND_DAMAGE    = 308502,
    SPELL_DARK_WAVE              = 306800,
    SPELL_RITUAL_COLLAPSE        = 318234
};

enum GorgrothEvents
{
    EVENT_SHADOW_BOLT = 1,
    EVENT_DARK_COMMAND,
    EVENT_DARK_WAVE
};

// 156814 - Gor'groth
struct boss_gorgroth : public BossAI
{
    boss_gorgroth(Creature* creature) : BossAI(creature, DATA_GORGROTH) { }

    void Reset() override
    {
        _Reset();
        DoCastSelf(SPELL_SUMMON_GHOULS_RITUAL, true);

        if (instance && instance->GetBossState(DATA_TUNK) == DONE)
            me->RemoveUnitFlag(UnitFlags(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE));
        else
            me->SetUnitFlag(UnitFlags(UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC));
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
        me->RemoveAurasDueToSpell(SPELL_SUMMON_GHOULS_RITUAL);
        DoCastSelf(SPELL_RITUAL_ACTIVATION, true);

        events.ScheduleEvent(EVENT_SHADOW_BOLT, 4s);
        events.ScheduleEvent(EVENT_DARK_COMMAND, 10s);
        events.ScheduleEvent(EVENT_DARK_WAVE, 18s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        DoCastSelf(SPELL_RITUAL_COLLAPSE, true);
        _JustDied();

        if (Creature* ravnyr = instance->GetCreature(DATA_RAVNYR))
        {
            ravnyr->RemoveUnitFlag(UnitFlags(UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NON_ATTACKABLE));
            ravnyr->SetReactState(REACT_AGGRESSIVE);
        }
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
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
                    DoCastVictim(SPELL_DARK_COMMAND_DAMAGE, true);
                    events.Repeat(16s);
                    break;
                case EVENT_DARK_WAVE:
                    DoCastSelf(SPELL_DARK_WAVE);
                    events.Repeat(20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        me->DoMeleeAttackIfReady();
    }
};

void AddSC_boss_gorgroth()
{
    RegisterDarkmaulCitadelCreatureAI(boss_gorgroth);
}
