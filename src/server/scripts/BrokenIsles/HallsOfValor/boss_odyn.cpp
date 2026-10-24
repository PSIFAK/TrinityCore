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
 * - There is no creature_text for Odyn in the world database, so no Talk() calls are made.
 * - Spear of Light (198072) and Shatter Spears (198077) are not ported: the searcher/missile chain that places the
 *   Spear of Light creatures (100575) is not known.
 * - The Runic Brand areatriggers (197968, 197971, 197972, 197975, 197977 -> create properties 5398, 5400, 5401, 5403,
 *   5404) are not in the world database yet; the spells are still cast.
 * - Odyn is not killed: at 80% health he yields and the encounter is completed.
 */

#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "halls_of_valor.h"

enum OdynSpells
{
    SPELL_RADIANT_TEMPEST           = 201006,
    SPELL_RADIANT_TEMPEST_PULL      = 201008, // EffectBasePoints of 201006 EFFECT_0
    SPELL_RUNIC_BRAND               = 197961,

    SPELL_RUNIC_BRAND_PURE          = 197963,
    SPELL_RUNIC_BRAND_RED           = 197964,
    SPELL_RUNIC_BRAND_YELLOW        = 197965,
    SPELL_RUNIC_BRAND_BLUE          = 197966,
    SPELL_RUNIC_BRAND_GREEN         = 197967,

    SPELL_RUNIC_BRAND_PURE_AT       = 197968,
    SPELL_RUNIC_BRAND_RED_AT        = 197971,
    SPELL_RUNIC_BRAND_YELLOW_AT     = 197972,
    SPELL_RUNIC_BRAND_BLUE_AT       = 197975,
    SPELL_RUNIC_BRAND_GREEN_AT      = 197977,

    SPELL_UNWORTHY_DOT              = 200988,

    // Heroic+
    SPELL_SUMMON_STORMFORGED        = 201209,
    SPELL_SURGE                     = 198750,
    SPELL_CONDUIT                   = 199787,

    // Achievement helper spell (criteria: be spell target)
    SPELL_ODYN_ACHIEVEMENT_CREDIT   = 208534
};

enum OdynEvents
{
    EVENT_RADIANT_TEMPEST = 1,
    EVENT_RUNIC_BRAND,
    EVENT_SUMMON_STORMFORGED,
    EVENT_CHECK_ACHIEVEMENT
};

enum OdynMisc
{
    POINT_ODYN_JUMP     = 1
};

static constexpr uint32 RunicBrandSpells[MaxRunicBrands] =
{
    SPELL_RUNIC_BRAND_PURE,
    SPELL_RUNIC_BRAND_RED,
    SPELL_RUNIC_BRAND_YELLOW,
    SPELL_RUNIC_BRAND_BLUE,
    SPELL_RUNIC_BRAND_GREEN
};

static constexpr uint32 RunicBrandAreaTriggerSpells[MaxRunicBrands] =
{
    SPELL_RUNIC_BRAND_PURE_AT,
    SPELL_RUNIC_BRAND_RED_AT,
    SPELL_RUNIC_BRAND_YELLOW_AT,
    SPELL_RUNIC_BRAND_BLUE_AT,
    SPELL_RUNIC_BRAND_GREEN_AT
};

Position const OdynJumpPos = { 2399.30f, 528.85f, 749.0f, 0.0f };

// 95676 - Odyn
struct boss_odyn : public BossAI
{
    boss_odyn(Creature* creature) : BossAI(creature, DATA_ODYN), _introDone(false), _defeated(false), _achievement(false), _runicBrandCount(0) { }

    void JustAppeared() override
    {
        if (instance->GetBossState(DATA_ODYN) == DONE)
        {
            _defeated = true;
            me->SetImmuneToAll(true);
            return;
        }

        if (instance->GetBossState(DATA_GOD_KING_SKOVALD) == DONE)
            DoAction(ACTION_ODYN_INTRO);
    }

    void Reset() override
    {
        if (_defeated)
            return;

        _Reset();
        _achievement = false;
        _runicBrandCount = 0;

        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNWORTHY_DOT);
        for (uint32 spellId : RunicBrandSpells)
            instance->DoRemoveAurasDueToSpellOnPlayers(spellId);
    }

    void JustEngagedWith(Unit* who) override
    {
        if (_defeated)
            return;

        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        _achievement = true;
        DoCastSelf(SPELL_REMOVE_ODYNS_BLESSING, true);

        events.ScheduleEvent(EVENT_RADIANT_TEMPEST, 24s);
        events.ScheduleEvent(EVENT_RUNIC_BRAND, 44s);

        if (IsHeroicOrHigher())
            events.ScheduleEvent(EVENT_SUMMON_STORMFORGED, 18s);

        if (IsMythic() || IsMythicPlus())
            events.ScheduleEvent(EVENT_CHECK_ACHIEVEMENT, 3s);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        if (_defeated)
            return;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        BossAI::EnterEvadeMode(why);
    }

    void DoAction(int32 action) override
    {
        if (action != ACTION_ODYN_INTRO || _introDone || _defeated)
            return;

        _introDone = true;

        _introScheduler.Schedule(5s, [this](TaskContext /*task*/)
        {
            me->GetMotionMaster()->MoveJump(POINT_ODYN_JUMP, OdynJumpPos, 25.0f, 15.0f);
        });
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != EFFECT_MOTION_TYPE || id != POINT_ODYN_JUMP)
            return;

        me->SetHomePosition(me->GetPosition());

        _introScheduler.Schedule(5s, [this](TaskContext /*task*/)
        {
            me->SetFaction(FACTION_MONSTER_2);
            me->SetImmuneToPC(false);
        });
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_RUNIC_BRAND || _runicBrandCount >= MaxRunicBrands)
            return;

        Unit* unitTarget = target->ToUnit();
        if (!unitTarget || !unitTarget->IsPlayer())
            return;

        DoCast(unitTarget, RunicBrandSpells[_runicBrandCount], true);
        DoCastSelf(RunicBrandAreaTriggerSpells[_runicBrandCount], true);
        instance->SetData(DATA_RUNE_ACTIVATED, _runicBrandCount);
        ++_runicBrandCount;
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == DATA_RUNE_DEACTIVATED && data < MaxRunicBrands)
            me->RemoveAreaTrigger(RunicBrandAreaTriggerSpells[data]);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_defeated)
        {
            damage = 0;
            return;
        }

        if (!me->HealthBelowPctDamaged(80, damage))
            return;

        damage = 0;
        _defeated = true;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();

        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNWORTHY_DOT);
        for (uint32 spellId : RunicBrandSpells)
            instance->DoRemoveAurasDueToSpellOnPlayers(spellId);

        instance->instance->DoOnPlayers([this](Player* player)
        {
            player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
        });

        if (_achievement && (IsMythic() || IsMythicPlus()))
            instance->DoUpdateCriteria(CriteriaType::BeSpellTarget, SPELL_ODYN_ACHIEVEMENT_CREDIT);

        me->SetFaction(FACTION_FRIENDLY);
        me->SetImmuneToAll(true);
        me->RemoveAllAuras();
        me->AttackStop();
        me->CombatStop(true);
        me->GetMotionMaster()->MoveTargetedHome();
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
                case EVENT_RADIANT_TEMPEST:
                    DoCastAOE(SPELL_RADIANT_TEMPEST);
                    events.Repeat(48s);
                    break;
                case EVENT_RUNIC_BRAND:
                    _runicBrandCount = 0;
                    DoCastAOE(SPELL_RUNIC_BRAND);
                    events.Repeat(56s);
                    break;
                case EVENT_SUMMON_STORMFORGED:
                    me->CastSpell(HOVHallCenterPos, SPELL_SUMMON_STORMFORGED, true);
                    events.Repeat(52s);
                    break;
                case EVENT_CHECK_ACHIEVEMENT:
                {
                    std::list<Creature*> obliterators;
                    me->GetCreatureListWithEntryInGrid(obliterators, NPC_STORMFORGED_OBLITERATOR, 150.0f);
                    for (Creature* obliterator : obliterators)
                        if (obliterator->HasAura(SPELL_CONDUIT))
                            _achievement = false;
                    events.Repeat(3s);
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
    bool _introDone;
    bool _defeated;
    bool _achievement;
    uint8 _runicBrandCount;
};

// 102019 - Stormforged Obliterator
struct npc_odyn_stormforged_obliterator : public ScriptedAI
{
    npc_odyn_stormforged_obliterator(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // Template faction is friendly and the summon properties carry no faction, inherit the summoner's
        me->SetFaction(summoner->GetFaction());
        me->SetReactState(REACT_PASSIVE);

        _scheduler.Schedule(2s, [this](TaskContext /*task*/)
        {
            me->SetReactState(REACT_AGGRESSIVE);
            DoZoneInCombat();

            _scheduler.Schedule(1s, [this](TaskContext task)
            {
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                    DoCastAOE(SPELL_SURGE);
                task.Repeat(4s);
            });
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (!UpdateVictim())
            return;
    }

private:
    TaskScheduler _scheduler;
};

// 201006 - Radiant Tempest
class spell_odyn_radiant_tempest : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_RADIANT_TEMPEST_PULL });
    }

    void HandleScript(SpellEffIndex /*effIndex*/) const
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (target == caster)
            return;

        target->CastSpell(caster, SPELL_RADIANT_TEMPEST_PULL, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_odyn_radiant_tempest::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// 197963 - Runic Brand
// 197964 - Runic Brand
// 197965 - Runic Brand
// 197966 - Runic Brand
// 197967 - Runic Brand
class spell_odyn_runic_brand : public AuraScript
{
    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        InstanceScript* instance = GetTarget()->GetInstanceScript();
        if (!instance)
            return;

        for (uint8 i = 0; i < MaxRunicBrands; ++i)
        {
            if (GetId() == RunicBrandSpells[i])
            {
                instance->SetData(DATA_RUNE_DEACTIVATED, i);
                break;
            }
        }
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_odyn_runic_brand::HandleRemove, EFFECT_1, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_odyn()
{
    RegisterHallsOfValorCreatureAI(boss_odyn);
    RegisterHallsOfValorCreatureAI(npc_odyn_stormforged_obliterator);

    RegisterSpellScript(spell_odyn_radiant_tempest);
    RegisterSpellScript(spell_odyn_runic_brand);
}
