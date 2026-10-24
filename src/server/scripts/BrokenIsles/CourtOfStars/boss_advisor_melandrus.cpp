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

#include "Containers.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "court_of_stars.h"

// Note: there is no creature_text for any Court of Stars creature in TDB, so the Talk() calls of the original script
// and the Elisande (110443) intro conversation, which consisted only of Talk() calls, were dropped.

enum MelandrusSpells
{
    SPELL_PIERCING_GALE             = 209628,
    SPELL_SLICING_MAELSTROM         = 209676,
    SPELL_BLADE_SURGE               = 209602, // the damage spell 209667 triggers 209613 (summon Image of Advisor Melandrus)
    SPELL_ENVELOPING_WINDS          = 224327,
    SPELL_ENVELOPING_WINDS_SUMMON   = 224329,

    // Image of Advisor Melandrus
    SPELL_WIND_IMAGE                = 209614,
    SPELL_SLICING_MAELSTROM_IMAGE   = 209741,

    // Enveloping Winds
    SPELL_ENVELOPING_WINDS_AT       = 224330, // AreaTrigger 8425 has no create properties in TDB yet
    SPELL_ENVELOPING_WINDS_STUN     = 224333
};

enum MelandrusEvents
{
    EVENT_PIERCING_GALE = 1,
    EVENT_SLICING_MAELSTROM,
    EVENT_BLADE_SURGE,
    EVENT_ENVELOPING_WINDS
};

enum MelandrusActions
{
    ACTION_IMAGE_PIERCING_GALE = 1,
    ACTION_IMAGE_SLICING_MAELSTROM
};

enum MelandrusMisc
{
    POINT_WINDS_TARGET = 1
};

// 104218 - Advisor Melandrus
struct boss_advisor_melandrus : public BossAI
{
    boss_advisor_melandrus(Creature* creature) : BossAI(creature, DATA_ADVISOR_MELANDRUS) { }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        events.ScheduleEvent(EVENT_PIERCING_GALE, 6s);
        events.ScheduleEvent(EVENT_ENVELOPING_WINDS, 10s);
        events.ScheduleEvent(EVENT_SLICING_MAELSTROM, 11s);
        events.ScheduleEvent(EVENT_BLADE_SURGE, 19s);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void JustDied(Unit* /*killer*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
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
                case EVENT_PIERCING_GALE:
                {
                    DoCastVictim(SPELL_PIERCING_GALE);
                    summons.DoAction(ACTION_IMAGE_PIERCING_GALE, EntryCheckPredicate(NPC_IMAGE_OF_MELANDRUS));
                    events.Repeat(20s);
                    break;
                }
                case EVENT_SLICING_MAELSTROM:
                {
                    DoCastSelf(SPELL_SLICING_MAELSTROM);
                    summons.DoAction(ACTION_IMAGE_SLICING_MAELSTROM, EntryCheckPredicate(NPC_IMAGE_OF_MELANDRUS));
                    events.Repeat(18s);
                    break;
                }
                case EVENT_BLADE_SURGE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        DoCast(target, SPELL_BLADE_SURGE);
                    events.Repeat(19s);
                    break;
                case EVENT_ENVELOPING_WINDS:
                    DoCastSelf(SPELL_ENVELOPING_WINDS, true);
                    events.Repeat(20s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }
};

// 105754 - Image of Advisor Melandrus
struct npc_melandrus_image_of_advisor_melandrus : public ScriptedAI
{
    npc_melandrus_image_of_advisor_melandrus(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _summonerGUID = summoner->GetGUID();
        // TDB template has the placeholder faction 35, the image must be hostile like Melandrus
        if (Unit* unitSummoner = summoner->ToUnit())
            me->SetFaction(unitSummoner->GetFaction());
    }

    void Reset() override
    {
        DoCastSelf(SPELL_WIND_IMAGE, true);
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_IMAGE_PIERCING_GALE:
                if (Creature* melandrus = ObjectAccessor::GetCreature(*me, _summonerGUID))
                    if (Unit* victim = melandrus->GetVictim())
                        DoCast(victim, SPELL_PIERCING_GALE);
                break;
            case ACTION_IMAGE_SLICING_MAELSTROM:
                DoCastSelf(SPELL_SLICING_MAELSTROM_IMAGE);
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 /*diff*/) override { }

private:
    ObjectGuid _summonerGUID;
};

// 112687 - Enveloping Winds
struct npc_melandrus_enveloping_winds : public ScriptedAI
{
    npc_melandrus_enveloping_winds(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetSpeedRate(MOVE_RUN, 0.15f);
        me->SetSpeedRate(MOVE_WALK, 0.15f);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _summonerGUID = summoner->GetGUID();
        if (Unit* unitSummoner = summoner->ToUnit())
            me->SetFaction(unitSummoner->GetFaction());

        _scheduler.Schedule(5s, [this](TaskContext /*task*/)
        {
            DoCastSelf(SPELL_ENVELOPING_WINDS_AT, true);
            MoveToRandomTarget();
        });
    }

    void Reset() override { }

    void MoveToRandomTarget()
    {
        Creature* melandrus = ObjectAccessor::GetCreature(*me, _summonerGUID);
        if (!melandrus || !melandrus->IsAIEnabled())
            return;

        if (Unit* target = melandrus->AI()->SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
            me->GetMotionMaster()->MovePoint(POINT_WINDS_TARGET, target->GetPosition());
    }

    void OnSpellCast(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_ENVELOPING_WINDS_STUN)
            me->DespawnOrUnsummon(1s);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_WINDS_TARGET)
            return;

        _scheduler.Schedule(1s, [this](TaskContext /*task*/)
        {
            MoveToRandomTarget();
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    ObjectGuid _summonerGUID;
    TaskScheduler _scheduler;
};

// 224327 - Enveloping Winds
class spell_melandrus_enveloping_winds : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_ENVELOPING_WINDS_SUMMON });
    }

    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/) const
    {
        CastSpellExtraArgs args(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        GetCaster()->CastSpell(GetHitUnit()->GetPosition(), SPELL_ENVELOPING_WINDS_SUMMON, args);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_melandrus_enveloping_winds::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_melandrus_enveloping_winds::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_boss_advisor_melandrus()
{
    RegisterCourtOfStarsCreatureAI(boss_advisor_melandrus);
    RegisterCourtOfStarsCreatureAI(npc_melandrus_image_of_advisor_melandrus);
    RegisterCourtOfStarsCreatureAI(npc_melandrus_enveloping_winds);

    RegisterSpellScript(spell_melandrus_enveloping_winds);
}
