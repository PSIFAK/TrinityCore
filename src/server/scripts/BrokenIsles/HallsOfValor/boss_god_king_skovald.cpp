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
 * - There is no creature_text for Skovald, Odyn or the Aegis in the world database, so no Talk() calls are made.
 * - God-King Skovald has no world database spawn; the instance script summons him once the four kings are defeated.
 * - The intro conversation (193975 -> conversation 550) is not in the world database and is not cast.
 * - Claim the Aegis (194112) and its handoff aura (193988) use nearby-entry targets without conditions, so the
 *   pickup is handled directly by the script when Skovald reaches the Aegis.
 * - Infernal Flames (193704) creates an areatrigger (create properties 5031) which is not in the world database yet.
 * - The Aegis of Aggramar (98364) has neither gossip flag nor spellclick data in the world database; the OnGossipHello
 *   pickup only works once that data exists.
 */

#include "Containers.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "halls_of_valor.h"

enum SkovaldSpells
{
    SPELL_ZERO_ENERGY_REGEN         = 78725,

    SPELL_FELBLAZE_RUSH             = 193658,
    SPELL_FELBLAZE_RUSH_CHARGE      = 193659, // EffectBasePoints of 193658
    SPELL_RAGNAROK                  = 193826,
    SPELL_RAGNAROK_DAMAGE           = 193827,
    SPELL_RAGNAROK_MOD              = 202494,
    SPELL_SAVAGE_BLADE              = 193668,
    SPELL_SAVAGE_BLADE_BLEED        = 193686,
    SPELL_INFERNAL_FLAMES_AURA      = 193983,
    SPELL_INFERNAL_FLAMES_SEARCH_N  = 194222,
    SPELL_INFERNAL_FLAMES_SEARCH_H  = 207510,
    SPELL_INFERNAL_FLAMES_MISSILE   = 193705,
    SPELL_INFERNAL_FLAMES_AT        = 193704,
    SPELL_FLAME_OF_WOE_MISSILE      = 207509,

    // Aegis of Aggramar
    SPELL_AEGIS_VISUAL_SHIELD       = 193769,
    SPELL_AEGIS_OF_AGGRAMAR_PLAYER  = 193783,
    SPELL_DROP_AEGIS                = 193991,

    // Honored Ancestor
    SPELL_ANCESTOR_VISUAL_1         = 199745,
    SPELL_ANCESTOR_VISUAL_2         = 199738,
    SPELL_ANCESTRAL_BLESSING        = 199747,

    // Flame of Woe
    SPELL_CONSUMING_FLAME           = 221093,
    SPELL_FLAME_OF_WOE_PERIODIC     = 221094,
    SPELL_FLAME_OF_WOE_FIXATE       = 193708
};

enum SkovaldEvents
{
    EVENT_ENERGIZE = 1,
    EVENT_FELBLAZE_RUSH,
    EVENT_SAVAGE_BLADE,
    EVENT_PICK_AEGIS
};

enum SkovaldMisc
{
    POINT_AEGIS             = 1,
    POINT_ANCESTOR_OWNER    = 2,
    SKOVALD_START_ENERGY    = 80
};

// 95675 - God-King Skovald
struct boss_god_king_skovald : public BossAI
{
    boss_god_king_skovald(Creature* creature) : BossAI(creature, DATA_GOD_KING_SKOVALD), _active(false), _aegisEvent(false), _energyTick(0) { }

    void JustAppeared() override
    {
        me->SetHomePosition(me->GetPosition());

        if (instance->GetData(DATA_SKOVALD_INTRO) == DONE)
        {
            Activate();
            return;
        }

        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);

        _introScheduler.Schedule(24s, [this](TaskContext /*task*/)
        {
            instance->SetData(DATA_SKOVALD_INTRO, DONE);
            Activate();
        });
    }

    void Activate()
    {
        _active = true;
        me->SetImmuneToAll(false);
        me->SetReactState(REACT_DEFENSIVE);
    }

    void Reset() override
    {
        _Reset();

        DoCastSelf(SPELL_ZERO_ENERGY_REGEN, true);
        me->SetPowerType(POWER_ENERGY);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPower(POWER_ENERGY, SKOVALD_START_ENERGY);
        me->RemoveAurasDueToSpell(SPELL_RAGNAROK_MOD);

        _aegisEvent = false;
        _energyTick = 0;

        if (_active)
            me->SetReactState(REACT_DEFENSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        DoCastSelf(SPELL_REMOVE_ODYNS_BLESSING, true);
        me->SetReactState(REACT_AGGRESSIVE);

        events.ScheduleEvent(EVENT_ENERGIZE, 1s);
        events.ScheduleEvent(EVENT_FELBLAZE_RUSH, 7s);
        events.ScheduleEvent(EVENT_SAVAGE_BLADE, 24s);
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
        if (action == ACTION_AEGIS_DROPPED && me->IsEngaged())
            events.RescheduleEvent(EVENT_PICK_AEGIS, 1s);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_AEGIS)
            return;

        if (Creature* aegis = instance->GetCreature(DATA_AEGIS_OF_AGGRAMAR))
        {
            aegis->DespawnOrUnsummon();
            ClaimAegis();
        }
        else
            ResumeAttack();
    }

    void ClaimAegis()
    {
        if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat))
            me->SetFacingToObject(target);

        DoCastSelf(SPELL_INFERNAL_FLAMES_AURA, true);
        ResumeAttack();
    }

    void ResumeAttack()
    {
        _aegisEvent = false;
        me->SetReactState(REACT_AGGRESSIVE);
        if (Unit* victim = SelectTarget(SelectTargetMethod::MaxThreat))
            AttackStart(victim);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        Unit* unitTarget = target->ToUnit();
        if (!unitTarget)
            return;

        switch (spellInfo->Id)
        {
            case SPELL_SAVAGE_BLADE:
                if (unitTarget != me)
                    DoCast(unitTarget, SPELL_SAVAGE_BLADE_BLEED, true);
                break;
            case SPELL_INFERNAL_FLAMES_SEARCH_N:
                if (!me->isInBack(unitTarget, float(M_PI) * 0.8f))
                    DoCast(unitTarget, SPELL_INFERNAL_FLAMES_MISSILE, true);
                break;
            case SPELL_INFERNAL_FLAMES_SEARCH_H:
                DoCast(unitTarget, SPELL_FLAME_OF_WOE_MISSILE, true);
                break;
            default:
                break;
        }
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
                case EVENT_ENERGIZE:
                {
                    int32 power = me->GetPower(POWER_ENERGY);
                    if (power >= 100)
                    {
                        if (!_aegisEvent)
                        {
                            DoCastSelf(SPELL_RAGNAROK);
                            // Ragnarok has no power cost in the client data, reset the energy manually
                            me->SetPower(POWER_ENERGY, 0);
                        }
                    }
                    else
                    {
                        // +2, +2, +1 energy per second
                        me->SetPower(POWER_ENERGY, power + (_energyTick < 2 ? 2 : 1));
                        _energyTick = uint8((_energyTick + 1) % 3);
                    }
                    events.Repeat(1s);
                    break;
                }
                case EVENT_FELBLAZE_RUSH:
                    if (!_aegisEvent)
                        DoCastAOE(SPELL_FELBLAZE_RUSH);
                    events.Repeat(26s);
                    break;
                case EVENT_SAVAGE_BLADE:
                    if (!_aegisEvent)
                        DoCastVictim(SPELL_SAVAGE_BLADE);
                    events.Repeat(24s);
                    break;
                case EVENT_PICK_AEGIS:
                    if (Creature* aegis = instance->GetCreature(DATA_AEGIS_OF_AGGRAMAR))
                    {
                        _aegisEvent = true;
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        me->GetMotionMaster()->MovePoint(POINT_AEGIS, aegis->GetPosition());
                    }
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    TaskScheduler _introScheduler;
    bool _active;
    bool _aegisEvent;
    uint8 _energyTick;
};

// 98364 - Aegis of Aggramar
struct npc_skovald_aegis_of_aggramar : public ScriptedAI
{
    npc_skovald_aegis_of_aggramar(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);
        DoCastSelf(SPELL_AEGIS_VISUAL_SHIELD, true);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        InstanceScript* instance = me->GetInstanceScript();
        if (!summoner->IsPlayer() || !instance)
            return;

        // Dropped by a player: only Skovald can reclaim it
        me->SetUninteractible(true);

        Creature* skovald = instance->GetCreature(DATA_GOD_KING_SKOVALD);
        if (!skovald || instance->GetBossState(DATA_GOD_KING_SKOVALD) != IN_PROGRESS || !skovald->IsEngaged())
        {
            me->DespawnOrUnsummon();
            return;
        }

        skovald->AI()->DoAction(ACTION_AEGIS_DROPPED);
    }

    bool OnGossipHello(Player* player) override
    {
        CloseGossipMenuFor(player);
        player->CastSpell(player, SPELL_AEGIS_OF_AGGRAMAR_PLAYER, true);
        me->DespawnOrUnsummon();
        return true;
    }

    void UpdateAI(uint32 /*diff*/) override { }
};

// 101326 - Honored Ancestor
struct npc_skovald_honored_ancestor : public ScriptedAI
{
    npc_skovald_honored_ancestor(Creature* creature) : ScriptedAI(creature), _done(false) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _ownerGUID = summoner->GetGUID();
        // Template faction is friendly and the summon properties carry no faction, inherit the summoner's
        me->SetFaction(summoner->GetFaction());

        me->SetReactState(REACT_PASSIVE);
        me->SetWalk(true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISTRACT, true);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);

        DoCastSelf(SPELL_ANCESTOR_VISUAL_1, true);
        DoCastSelf(SPELL_ANCESTOR_VISUAL_2, true);

        _scheduler.Schedule(1s, [this](TaskContext task)
        {
            if (_done)
                return;

            Unit* owner = ObjectAccessor::GetUnit(*me, _ownerGUID);
            if (!owner || !owner->IsAlive())
            {
                me->DespawnOrUnsummon();
                return;
            }

            if (me->GetDistance(owner) < 3.0f)
            {
                _done = true;
                DoCast(owner, SPELL_ANCESTRAL_BLESSING, true);
                me->SetVisible(false);
                me->DespawnOrUnsummon(3s);
                return;
            }

            if (!me->HasUnitState(UNIT_STATE_STUNNED))
                me->GetMotionMaster()->MovePoint(POINT_ANCESTOR_OWNER, owner->GetPosition());

            task.Repeat(1s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    ObjectGuid _ownerGUID;
    bool _done;
};

// 104822 - Flame of Woe
struct npc_skovald_flame_of_woe : public ScriptedAI
{
    npc_skovald_flame_of_woe(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // Template faction is friendly and the summon properties carry no faction, inherit the summoner's
        me->SetFaction(summoner->GetFaction());
        me->SetSpeedRate(MOVE_RUN, 0.7f);
        DoZoneInCombat();

        DoCastSelf(SPELL_CONSUMING_FLAME, true);
        DoCastSelf(SPELL_FLAME_OF_WOE_PERIODIC, true);

        _scheduler.Schedule(1s, [this](TaskContext task)
        {
            Unit* victim = me->GetVictim();
            if (!victim || !victim->HasAura(SPELL_FLAME_OF_WOE_FIXATE, me->GetGUID()))
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 80.0f, true))
                {
                    ResetThreatList();
                    AddThreat(target, 100000.0f);
                    AttackStart(target);
                    DoCast(target, SPELL_FLAME_OF_WOE_FIXATE, true);
                }
            }
            task.Repeat(2s);
        });

        _scheduler.Schedule(2s, [this](TaskContext task)
        {
            DoCastSelf(SPELL_INFERNAL_FLAMES_AT, true);
            task.Repeat(4s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 193658 - Felblaze Rush
class spell_skovald_felblaze_rush : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_FELBLAZE_RUSH_CHARGE });
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
        GetCaster()->CastSpell(GetHitUnit(), SPELL_FELBLAZE_RUSH_CHARGE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_skovald_felblaze_rush::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_skovald_felblaze_rush::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 193826 - Ragnarok
class spell_skovald_ragnarok : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_RAGNAROK_DAMAGE, SPELL_RAGNAROK_MOD });
    }

    void HandlePeriodic(AuraEffect const* aurEff) const
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->CastSpell(caster, SPELL_RAGNAROK_DAMAGE, aurEff);
        caster->CastSpell(caster, SPELL_RAGNAROK_MOD, aurEff);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_skovald_ragnarok::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 193991 - Drop Aegis
class spell_skovald_drop_aegis : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_AEGIS_OF_AGGRAMAR_SUMMON });
    }

    void HandleHit(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        Unit* caster = GetCaster();
        WorldLocation const* dest = GetHitDest();
        InstanceScript* instance = caster->GetInstanceScript();
        if (!dest || !instance || instance->GetBossState(DATA_GOD_KING_SKOVALD) != IN_PROGRESS)
            return;

        caster->CastSpell(dest->GetPosition(), SPELL_AEGIS_OF_AGGRAMAR_SUMMON, true);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_skovald_drop_aegis::HandleHit, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
    }
};

// 193783 - Aegis of Aggramar (player)
class spell_skovald_aegis_of_aggramar_player : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DROP_AEGIS });
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_DROP_AEGIS, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_skovald_aegis_of_aggramar_player::HandleRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 193983 - Aegis of Aggramar (Skovald)
class spell_skovald_aegis_of_aggramar : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DROP_AEGIS });
    }

    void CalculateAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/) const
    {
        amount = -1;
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount) const
    {
        Unit* target = GetTarget();
        Unit* attacker = dmgInfo.GetAttacker();

        // Only attacks from close behind get through the Aegis
        if (attacker && attacker->GetDistance(target) < 5.0f && target->isInBack(attacker, float(M_PI) * 0.8f))
            absorbAmount = 0;
        else
            absorbAmount = dmgInfo.GetDamage();
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        GetTarget()->CastSpell(GetTarget(), SPELL_DROP_AEGIS, true);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_skovald_aegis_of_aggramar::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_skovald_aegis_of_aggramar::Absorb, EFFECT_1);
        AfterEffectRemove += AuraEffectRemoveFn(spell_skovald_aegis_of_aggramar::HandleRemove, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
    }
};

// 193743 - Aegis of Aggramar (player shield)
class spell_skovald_aegis_of_aggramar_shield : public AuraScript
{
    void CalculateAmount(AuraEffect const* /*aurEff*/, SpellEffectValue& amount, bool& /*canBeRecalculated*/) const
    {
        amount = -1;
    }

    void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount) const
    {
        Unit* attacker = dmgInfo.GetAttacker();

        // Damage coming from the front of the shield bearer is absorbed
        if (attacker && GetTarget()->HasInArc(float(M_PI) * 1.4f, attacker))
            absorbAmount = dmgInfo.GetDamage();
        else
            absorbAmount = 0;
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_skovald_aegis_of_aggramar_shield::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_skovald_aegis_of_aggramar_shield::Absorb, EFFECT_0);
    }
};

// 202711 - Aegis of Aggramar
class spell_skovald_aegis_check_cast : public SpellScript
{
    SpellCastResult CheckRequirement() const
    {
        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
            if (instance->GetBossState(DATA_GOD_KING_SKOVALD) != IN_PROGRESS)
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_skovald_aegis_check_cast::CheckRequirement);
    }
};

void AddSC_boss_god_king_skovald()
{
    RegisterHallsOfValorCreatureAI(boss_god_king_skovald);
    RegisterHallsOfValorCreatureAI(npc_skovald_aegis_of_aggramar);
    RegisterHallsOfValorCreatureAI(npc_skovald_honored_ancestor);
    RegisterHallsOfValorCreatureAI(npc_skovald_flame_of_woe);

    RegisterSpellScript(spell_skovald_felblaze_rush);
    RegisterSpellScript(spell_skovald_ragnarok);
    RegisterSpellScript(spell_skovald_drop_aegis);
    RegisterSpellScript(spell_skovald_aegis_of_aggramar_player);
    RegisterSpellScript(spell_skovald_aegis_of_aggramar);
    RegisterSpellScript(spell_skovald_aegis_of_aggramar_shield);
    RegisterSpellScript(spell_skovald_aegis_check_cast);
}
