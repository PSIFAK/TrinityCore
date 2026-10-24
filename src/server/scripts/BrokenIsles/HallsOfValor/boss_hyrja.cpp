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

/*
 * Notes:
 * - There is no creature_text for Hyrja, Olmyr or Solsten in the world database, so no Talk() calls are made.
 * - The intro conversation (198226 -> conversation 827) is not in the world database and is not cast.
 * - Arcing Bolt (191978) is not handled: its dummy effect needs a damage spell link that is not known.
 * - Eye of the Storm (203955), Sanctify (192163) and the defender auras (192150 / 192152) create areatriggers
 *   (create properties 5667, 4885, 4866, 4865) which are not in the world database yet.
 * - The Heroic Expel Light bounce of the original script relied on stack bookkeeping that is not portable; only the
 *   explosion on removal is kept.
 */

#include "Containers.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "halls_of_valor.h"

enum HyrjaSpells
{
    SPELL_ASCENDANCE                = 191475,
    SPELL_ASCENDANCE_JUMP           = 191478,
    SPELL_EMPOWERMENT_TRACKER       = 192130,
    SPELL_EXPEL_LIGHT               = 192044,
    SPELL_EXPEL_LIGHT_AURA          = 192048,
    SPELL_EXPEL_LIGHT_DAMAGE        = 192067,
    SPELL_SHIELD_OF_LIGHT           = 192018,
    SPELL_MYSTIC_EMPOWERMENT_THUNDER = 192132,
    SPELL_MYSTIC_EMPOWERMENT_HOLY   = 192133,
    SPELL_EMPOWERMENT_THUNDER_AURA  = 192008,
    SPELL_EMPOWERMENT_HOLY_AURA     = 191924,

    // Special abilities
    SPELL_EYE_OF_THE_STORM          = 192304,
    SPELL_EYE_OF_THE_STORM_PERIODIC = 200901,
    SPELL_EYE_OF_THE_STORM_AT       = 203955,
    SPELL_SANCTIFY                  = 192158,
    SPELL_SANCTIFY_AT               = 192163,

    // Defenders
    SPELL_OLMYR_SPIRIT_VISUAL       = 191899,
    SPELL_SOLSTEN_SPIRIT_VISUAL     = 192147,
    SPELL_OLMYR_LIGHT               = 192288,
    SPELL_SOLSTEN_AREA              = 192150,
    SPELL_OLMYR_AREA                = 192152
};

enum HyrjaEvents
{
    EVENT_EXPEL_LIGHT = 1,
    EVENT_SHIELD_OF_LIGHT,
    EVENT_SPECIAL_ABILITY,

    EVENT_DEFENDER_PRIMARY,
    EVENT_DEFENDER_SPECIAL
};

// 95833 - Hyrja
struct boss_hyrja : public BossAI
{
    boss_hyrja(Creature* creature) : BossAI(creature, DATA_HYRJA), _activated(false), _defendersDefeated(0) { }

    void JustAppeared() override
    {
        if (!_activated)
            me->SetReactState(REACT_PASSIVE);
    }

    void Reset() override
    {
        _Reset();
        me->RemoveAurasDueToSpell(SPELL_EMPOWERMENT_TRACKER);

        if (_activated)
            DoCastSelf(SPELL_ASCENDANCE, true);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        DoCastSelf(SPELL_EMPOWERMENT_TRACKER, true);

        if (Creature* olmyr = instance->GetCreature(DATA_OLMYR_THE_ENLIGHTENED))
            olmyr->CastSpell(olmyr, SPELL_OLMYR_AREA, true);

        if (Creature* solsten = instance->GetCreature(DATA_SOLSTEN))
            solsten->CastSpell(solsten, SPELL_SOLSTEN_AREA, true);

        events.ScheduleEvent(EVENT_EXPEL_LIGHT, 3s);
        events.ScheduleEvent(EVENT_SHIELD_OF_LIGHT, 10s);
        events.ScheduleEvent(EVENT_SPECIAL_ABILITY, 20s);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        BossAI::EnterEvadeMode(why);
    }

    void JustDied(Unit* /*killer*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
    }

    void DoAction(int32 action) override
    {
        if (action != ACTION_DEFENDER_DEFEATED || _activated)
            return;

        if (++_defendersDefeated < 2)
            return;

        DoCastSelf(SPELL_ASCENDANCE);

        _introScheduler.Schedule(2s, [this](TaskContext /*task*/)
        {
            // Jump destination depends on spell target data that is not in the world database, the spell falls back to the caster position
            DoCastSelf(SPELL_ASCENDANCE_JUMP, true);
        });

        _introScheduler.Schedule(4s, [this](TaskContext /*task*/)
        {
            _activated = true;
            me->SetHomePosition(me->GetPosition());
            me->SetImmuneToPC(false);
            me->SetReactState(REACT_AGGRESSIVE);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _introScheduler.Update(diff);

        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_EXPEL_LIGHT:
                    DoCastAOE(SPELL_EXPEL_LIGHT);
                    events.Repeat(30s);
                    break;
                case EVENT_SHIELD_OF_LIGHT:
                    DoCastVictim(SPELL_SHIELD_OF_LIGHT);
                    events.Repeat(20s);
                    break;
                case EVENT_SPECIAL_ABILITY:
                {
                    Creature* olmyr = instance->GetCreature(DATA_OLMYR_THE_ENLIGHTENED);
                    Creature* solsten = instance->GetCreature(DATA_SOLSTEN);
                    if (olmyr && solsten)
                    {
                        // Hyrja uses the ability of the defender she is closest to
                        if (me->GetDistance(solsten) < me->GetDistance(olmyr))
                        {
                            DoCastSelf(SPELL_EYE_OF_THE_STORM, true);
                            DoCastSelf(SPELL_EYE_OF_THE_STORM_AT, true);
                        }
                        else
                            DoCastSelf(SPELL_SANCTIFY);
                    }
                    events.Repeat(30s);
                    break;
                }
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    TaskScheduler _introScheduler;
    bool _activated;
    uint8 _defendersDefeated;
};

// 97202 - Olmyr the Enlightened
// 97219 - Solsten
struct npc_hyrja_defender : public ScriptedAI
{
    npc_hyrja_defender(Creature* creature) : ScriptedAI(creature), _spiritForm(false), _evadePending(false) { }

    void Reset() override
    {
        _events.Reset();

        if (_spiritForm)
            ApplySpiritForm();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_DEFENDER_PRIMARY, 5s);
        _events.ScheduleEvent(EVENT_DEFENDER_SPECIAL, 30s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage < me->GetHealth())
            return;

        damage = 0;

        if (_spiritForm)
            return;

        _spiritForm = true;

        if (InstanceScript* instance = me->GetInstanceScript())
            if (Creature* hyrja = instance->GetCreature(DATA_HYRJA))
                hyrja->AI()->DoAction(ACTION_DEFENDER_DEFEATED);

        _evadePending = true;
    }

    void ApplySpiritForm()
    {
        me->AttackStop();
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);
        DoCastSelf(me->GetEntry() == NPC_OLMYR_THE_ENLIGHTENED ? SPELL_OLMYR_SPIRIT_VISUAL : SPELL_SOLSTEN_SPIRIT_VISUAL, true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_evadePending)
        {
            _evadePending = false;
            EnterEvadeMode(EvadeReason::Other);
            return;
        }

        if (!UpdateVictim())
            return;

        if (me->GetDistance(me->GetHomePosition()) >= 30.0f)
        {
            EnterEvadeMode(EvadeReason::Boundary);
            return;
        }

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DEFENDER_PRIMARY:
                    if (me->GetEntry() == NPC_OLMYR_THE_ENLIGHTENED)
                        DoCastVictim(SPELL_OLMYR_LIGHT);
                    else
                        DoCastSelf(SPELL_MYSTIC_EMPOWERMENT_THUNDER, true);
                    _events.Repeat(7s);
                    break;
                case EVENT_DEFENDER_SPECIAL:
                    if (me->GetEntry() == NPC_OLMYR_THE_ENLIGHTENED)
                        DoCastSelf(SPELL_SANCTIFY);
                    else
                    {
                        DoCastSelf(SPELL_EYE_OF_THE_STORM, true);
                        DoCastSelf(SPELL_EYE_OF_THE_STORM_AT, true);
                        DoCastSelf(SPELL_EYE_OF_THE_STORM_PERIODIC);
                    }
                    _events.Repeat(30s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
    bool _spiritForm;
    bool _evadePending;
};

// 192130 - Empowerment Tracker
class spell_hyrja_empowerment_tracker : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MYSTIC_EMPOWERMENT_THUNDER, SPELL_MYSTIC_EMPOWERMENT_HOLY });
    }

    static void ReduceOpposingEmpowerment(Unit* target, uint32 spellId)
    {
        Aura* aura = target->GetAura(spellId);
        if (!aura)
            return;

        if (aura->GetStackAmount() > 2 && target->GetMap()->IsHeroicOrHigher())
            aura->ModStackAmount(-2);
        else
            aura->Remove();
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/) const
    {
        Unit* target = GetTarget();
        bool const thunder = target->HasAura(SPELL_EMPOWERMENT_THUNDER_AURA);
        bool const holy = target->HasAura(SPELL_EMPOWERMENT_HOLY_AURA);

        if (thunder)
        {
            if (!holy)
                ReduceOpposingEmpowerment(target, SPELL_MYSTIC_EMPOWERMENT_HOLY);
            target->CastSpell(target, SPELL_MYSTIC_EMPOWERMENT_THUNDER, true);
        }

        if (holy)
        {
            if (!thunder)
                ReduceOpposingEmpowerment(target, SPELL_MYSTIC_EMPOWERMENT_THUNDER);
            target->CastSpell(target, SPELL_MYSTIC_EMPOWERMENT_HOLY, true);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hyrja_empowerment_tracker::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 192044 - Expel Light
class spell_hyrja_expel_light : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EXPEL_LIGHT_AURA });
    }

    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        targets.remove_if([](WorldObject const* target)
        {
            return !target->IsPlayer();
        });

        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/) const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_EXPEL_LIGHT_AURA, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hyrja_expel_light::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_hyrja_expel_light::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 192048 - Expel Light
class spell_hyrja_expel_light_aura : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_EXPEL_LIGHT_DAMAGE });
    }

    void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/) const
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(GetTarget()->GetPosition(), SPELL_EXPEL_LIGHT_DAMAGE, aurEff);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_hyrja_expel_light_aura::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 192067 - Expel Light
class spell_hyrja_expel_light_damage : public SpellScript
{
    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        // Area entry target without conditions, only players are valid
        targets.remove_if([](WorldObject const* target)
        {
            return !target->IsPlayer();
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hyrja_expel_light_damage::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
    }
};

// 192158 - Sanctify
class spell_hyrja_sanctify : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SANCTIFY_AT });
    }

    void HandlePeriodic(AuraEffect const* aurEff) const
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        for (uint8 i = 0; i < 6; ++i)
            caster->CastSpell(caster->GetPosition(), SPELL_SANCTIFY_AT, aurEff);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_hyrja_sanctify::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

void AddSC_boss_hyrja()
{
    RegisterHallsOfValorCreatureAI(boss_hyrja);
    RegisterHallsOfValorCreatureAI(npc_hyrja_defender);

    RegisterSpellScript(spell_hyrja_empowerment_tracker);
    RegisterSpellScript(spell_hyrja_expel_light);
    RegisterSpellScript(spell_hyrja_expel_light_aura);
    RegisterSpellScript(spell_hyrja_expel_light_damage);
    RegisterSpellScript(spell_hyrja_sanctify);
}
