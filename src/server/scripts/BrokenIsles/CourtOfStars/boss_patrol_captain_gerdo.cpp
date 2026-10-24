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
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "court_of_stars.h"

// Note: there is no creature_text for any Court of Stars creature in TDB, so the Talk() calls of the original script were dropped.

enum GerdoSpells
{
    SPELL_RESONANT_SLASH            = 207261,
    SPELL_RESONANT_SLASH_FRONT      = 206574,
    SPELL_RESONANT_SLASH_BACK       = 206580,
    SPELL_ARCANE_LOCKDOWN           = 207278,
    SPELL_SIGNAL_BEACON             = 207806,
    SPELL_FLASK_OF_THE_SOLEMN_NIGHT = 207815,
    SPELL_STREETSWEEPER             = 219488,

    // Vigilant Duskwatch
    SPELL_HINDER                    = 215204,

    // Streetsweeper
    SPELL_STREETSWEEPER_VISUAL      = 219475,
    SPELL_STREETSWEEPER_AREATRIGGER = 219477, // AreaTrigger 7870 has no create properties in TDB yet
    SPELL_STREETSWEEPER_BEAM        = 219483
};

enum GerdoEvents
{
    EVENT_RESONANT_SLASH = 1,
    EVENT_ARCANE_LOCKDOWN,
    EVENT_SIGNAL_BEACON,
    EVENT_STREETSWEEPER,
    EVENT_FLASK_OF_THE_SOLEMN_NIGHT,

    EVENT_HINDER
};

enum GerdoMisc
{
    POINT_FLASK                 = 1
};

constexpr Position FlaskPosition = { 1308.29f, 3537.42f, 20.76f };

// 104215 - Patrol Captain Gerdo
struct boss_patrol_captain_gerdo : public BossAI
{
    boss_patrol_captain_gerdo(Creature* creature) : BossAI(creature, DATA_PATROL_CAPTAIN_GERDO), _flaskTriggered(false) { }

    void Reset() override
    {
        _Reset();
        _flaskTriggered = false;
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        events.ScheduleEvent(EVENT_RESONANT_SLASH, 7s);
        events.ScheduleEvent(EVENT_ARCANE_LOCKDOWN, 15s);
        events.ScheduleEvent(EVENT_STREETSWEEPER, 18s);
        events.ScheduleEvent(EVENT_SIGNAL_BEACON, 23s);
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

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_SIGNAL_BEACON)
            instance->SetData(DATA_BEACON_ACTIVATE, 0);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_flaskTriggered || !me->HealthBelowPctDamaged(25, damage))
            return;

        _flaskTriggered = true;
        events.ScheduleEvent(EVENT_FLASK_OF_THE_SOLEMN_NIGHT, 5s);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_FLASK)
            return;

        DoCastSelf(SPELL_FLASK_OF_THE_SOLEMN_NIGHT);
        me->SetReactState(REACT_AGGRESSIVE);
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
                case EVENT_RESONANT_SLASH:
                    DoCastSelf(SPELL_RESONANT_SLASH);
                    events.Repeat(12s);
                    break;
                case EVENT_ARCANE_LOCKDOWN:
                    DoCastSelf(SPELL_ARCANE_LOCKDOWN);
                    events.Repeat(28s);
                    break;
                case EVENT_SIGNAL_BEACON:
                    DoCastSelf(SPELL_SIGNAL_BEACON);
                    break;
                case EVENT_STREETSWEEPER:
                    DoCastVictim(SPELL_STREETSWEEPER);
                    events.Repeat(18s);
                    break;
                case EVENT_FLASK_OF_THE_SOLEMN_NIGHT:
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->GetMotionMaster()->MovePoint(POINT_FLASK, FlaskPosition);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    bool _flaskTriggered;
};

// 104918 - Vigilant Duskwatch
struct npc_cos_vigilant_duskwatch : public ScriptedAI
{
    npc_cos_vigilant_duskwatch(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // Summoned by an Arcane Beacon (faction 190); the TDB template only has the placeholder faction 35
        me->SetFaction(FACTION_MONSTER_2);
        DoZoneInCombat();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_HINDER, 5s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (_events.ExecuteEvent() == EVENT_HINDER)
        {
            DoCastVictim(SPELL_HINDER);
            _events.Repeat(10s);
        }
    }

private:
    EventMap _events;
};

// 110403 - Streetsweeper
struct npc_cos_streetsweeper : public ScriptedAI
{
    npc_cos_streetsweeper(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _summonerGUID = summoner->GetGUID();
        if (Unit* unitSummoner = summoner->ToUnit())
            me->SetFaction(unitSummoner->GetFaction());

        DoCastSelf(SPELL_STREETSWEEPER_VISUAL, true);

        _scheduler.Schedule(5s, [this](TaskContext task)
        {
            if (Creature* gerdo = ObjectAccessor::GetCreature(*me, _summonerGUID))
                if (gerdo->IsAIEnabled())
                    if (Unit* target = gerdo->AI()->SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                        me->SetFacingToObject(target);

            task.Schedule(5s, [this](TaskContext areaTriggerTask)
            {
                DoCastSelf(SPELL_STREETSWEEPER_AREATRIGGER, true);

                areaTriggerTask.Schedule(2s, [this](TaskContext /*beamTask*/)
                {
                    DoCastSelf(SPELL_STREETSWEEPER_BEAM, true);
                });
            });
        });
    }

    void Reset() override { }

    void JustEngagedWith(Unit* /*who*/) override { }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    ObjectGuid _summonerGUID;
    TaskScheduler _scheduler;
};

// 207261 - Resonant Slash
class spell_gerdo_resonant_slash : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_RESONANT_SLASH_FRONT, SPELL_RESONANT_SLASH_BACK });
    }

    void HandleAfterCast() const
    {
        Unit* caster = GetCaster();
        CastSpellExtraArgs args(TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR);
        caster->CastSpell(caster->GetNearPosition(5.0f, 0.0f), SPELL_RESONANT_SLASH_FRONT, args);
        caster->CastSpell(caster->GetNearPosition(5.0f, float(M_PI)), SPELL_RESONANT_SLASH_BACK, args);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_gerdo_resonant_slash::HandleAfterCast);
    }
};

// 206574 - Resonant Slash (front)
// 206580 - Resonant Slash (back)
class spell_gerdo_resonant_slash_damage : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        // TARGET_UNIT_CONE_CASTER_TO_DEST_ENTRY without conditions would hit everything in the cone
        Unit* caster = GetCaster();
        targets.remove_if([caster](WorldObject const* target)
        {
            Unit const* unit = target->ToUnit();
            return !unit || !caster->IsValidAttackTarget(unit);
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gerdo_resonant_slash_damage::FilterTargets, EFFECT_ALL,
            TARGET_UNIT_CONE_CASTER_TO_DEST_ENTRY);
    }
};

// 207278 - Arcane Lockdown
// Applied with all stacks; jumping removes one (spell_proc: PROC_FLAG_JUMP + PROC_ATTR_USE_STACKS_FOR_CHARGES)
class spell_gerdo_arcane_lockdown : public SpellScript
{
    void HandleAfterHit() const
    {
        if (Aura* aura = GetHitAura())
            aura->SetStackAmount(static_cast<uint8>(GetSpellInfo()->StackAmount));
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_gerdo_arcane_lockdown::HandleAfterHit);
    }
};

// 210435 - Summon Vigilant Nightwatch
class spell_gerdo_summon_vigilant_nightwatch : public SpellScript
{
    void SelectTarget(WorldObject*& target) const
    {
        // Summon next to the beacon instead of whatever TARGET_DEST_NEARBY_ENTRY found without conditions
        target = GetCaster();
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_gerdo_summon_vigilant_nightwatch::SelectTarget, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
    }
};

void AddSC_boss_patrol_captain_gerdo()
{
    RegisterCourtOfStarsCreatureAI(boss_patrol_captain_gerdo);
    RegisterCourtOfStarsCreatureAI(npc_cos_vigilant_duskwatch);
    RegisterCourtOfStarsCreatureAI(npc_cos_streetsweeper);

    RegisterSpellScript(spell_gerdo_resonant_slash);
    RegisterSpellScript(spell_gerdo_resonant_slash_damage);
    RegisterSpellScript(spell_gerdo_arcane_lockdown);
    RegisterSpellScript(spell_gerdo_summon_vigilant_nightwatch);
}
