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
 * - There is no creature_text for Hymdall in the world database, so no Talk() calls are made.
 * - Storm Breath (188404 / 192959), Ball Lightning (193240 / 188405) and Dancing Blade (193214) create
 *   areatriggers (create properties 4960, 4961, 4492, 4987, 4985) which are not in the world database yet;
 *   the spells are still cast so the encounter works once that data is added.
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

enum HymdallSpells
{
    SPELL_HORN_OF_VALOR             = 191284,
    SPELL_DANCING_BLADE             = 193235,
    SPELL_DANCING_BLADE_AT          = 193214,
    SPELL_BLOODLETTING_SWEEP        = 193092,

    // Heroic+
    SPELL_BALL_LIGHTNING_AT_FRONT   = 193240,
    SPELL_BALL_LIGHTNING_AT_BACK    = 188405,

    // Storm Drake
    SPELL_STORM_BREATH_VISUAL       = 188404,
    SPELL_STORM_BREATH_AT           = 192959,

    // Valarjar Thundercaller (trash)
    SPELL_THUNDERSTRIKE_DAMAGE      = 198605
};

enum HymdallEvents
{
    EVENT_HORN_OF_VALOR = 1,
    EVENT_SUMMON_STORM_DRAKE,
    EVENT_DANCING_BLADE,
    EVENT_BLOODLETTING_SWEEP
};

enum HymdallMisc
{
    POINT_STORM_DRAKE_END           = 1,
    MAX_STORM_DRAKES                = 3,
    MAX_STORM_DRAKE_POSITIONS       = 6
};

Position const StormDrakePositions[MAX_STORM_DRAKE_POSITIONS] =
{
    { 3566.12f, 675.00f, 626.44f, 4.67f },
    { 3566.12f, 383.00f, 626.44f, 1.60f },
    { 3534.45f, 675.02f, 626.44f, 4.67f },
    { 3534.45f, 383.00f, 626.44f, 1.60f },
    { 3502.45f, 675.04f, 626.44f, 4.67f },
    { 3502.45f, 383.00f, 626.44f, 1.60f }
};

// 94960 - Hymdall
struct boss_hymdall : public BossAI
{
    boss_hymdall(Creature* creature) : BossAI(creature, DATA_HYMDALL), _defeated(false), _drakeCount(0), _drakePosIndex(0) { }

    void JustAppeared() override
    {
        // Hymdall is not killed, he yields; do not let him come back after a reload
        if (instance->GetBossState(DATA_HYMDALL) == DONE)
            me->DespawnOrUnsummon();
    }

    void Reset() override
    {
        if (_defeated)
            return;

        _Reset();
        _drakeCount = 0;
        _drakePosIndex = 0;
    }

    void JustEngagedWith(Unit* who) override
    {
        if (_defeated)
            return;

        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        events.ScheduleEvent(EVENT_HORN_OF_VALOR, 6s);
        events.ScheduleEvent(EVENT_DANCING_BLADE, 15s);
        events.ScheduleEvent(EVENT_BLOODLETTING_SWEEP, 20s);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        if (_defeated)
            return;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        BossAI::EnterEvadeMode(why);
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);

        if (summon->GetEntry() == NPC_DANCING_BLADE)
        {
            summon->SetReactState(REACT_PASSIVE);
            summon->CastSpell(summon, SPELL_DANCING_BLADE_AT, true);
        }
    }

    void JustReachedHome() override
    {
        _JustReachedHome();

        if (_defeated)
        {
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->DespawnOrUnsummon(5s);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_defeated)
        {
            damage = 0;
            return;
        }

        if (!me->HealthBelowPctDamaged(10, damage))
            return;

        damage = 0;
        _defeated = true;

        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();

        instance->instance->DoOnPlayers([this](Player* player)
        {
            player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
        });

        me->SetFaction(FACTION_FRIENDLY);
        me->SetImmuneToAll(true);
        me->RemoveAllAuras();
        me->AttackStop();
        me->CombatStop(true);
        me->GetMotionMaster()->MoveTargetedHome();
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
                case EVENT_HORN_OF_VALOR:
                {
                    DoCastSelf(SPELL_HORN_OF_VALOR);
                    if (_drakeCount < MAX_STORM_DRAKES)
                        ++_drakeCount;
                    _drakePosIndex = uint8(urand(0, MAX_STORM_DRAKE_POSITIONS - 1));
                    for (uint8 i = 0; i < _drakeCount; ++i)
                        events.ScheduleEvent(EVENT_SUMMON_STORM_DRAKE, 5s + std::chrono::seconds(i));
                    events.Repeat(30s);
                    break;
                }
                case EVENT_SUMMON_STORM_DRAKE:
                    me->SummonCreature(NPC_STORM_DRAKE, StormDrakePositions[_drakePosIndex], TEMPSUMMON_TIMED_DESPAWN, 30s);
                    _drakePosIndex = uint8((_drakePosIndex + 2) % MAX_STORM_DRAKE_POSITIONS);
                    break;
                case EVENT_DANCING_BLADE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 80.0f, true))
                        DoCast(target, SPELL_DANCING_BLADE);
                    events.Repeat(11s);
                    break;
                case EVENT_BLOODLETTING_SWEEP:
                    DoCastVictim(SPELL_BLOODLETTING_SWEEP);
                    events.Repeat(26s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    bool _defeated;
    uint8 _drakeCount;
    uint8 _drakePosIndex;
};

// 97788 - Storm Drake
struct npc_hymdall_storm_drake : public ScriptedAI
{
    npc_hymdall_storm_drake(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->SetSpeedRate(MOVE_FLIGHT, 3.5f);

        _scheduler.Schedule(3s, [this](TaskContext /*task*/)
        {
            DoCastSelf(SPELL_STORM_BREATH_VISUAL);

            float x, y, z;
            me->GetClosePoint(x, y, z, me->GetCombatReach(), 220.0f);
            me->GetMotionMaster()->MovePoint(POINT_STORM_DRAKE_END, x, y, z, false);

            _scheduler.Schedule(1s, [this](TaskContext task)
            {
                ChargeStaticField();
                task.Repeat(2s);
            });
        });
    }

    void ChargeStaticField()
    {
        Creature* staticField = me->FindNearestCreature(NPC_STATIC_FIELD, 15.0f);
        if (!staticField || staticField->HasAura(SPELL_STORM_BREATH_AT))
            return;

        staticField->CastSpell(staticField, SPELL_STORM_BREATH_AT, true);

        if (me->GetMap()->IsHeroicOrHigher())
        {
            staticField->CastSpell(staticField, SPELL_BALL_LIGHTNING_AT_FRONT, true);
            staticField->CastSpell(staticField, SPELL_BALL_LIGHTNING_AT_BACK, true);
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_STORM_DRAKE_END)
            return;

        _scheduler.CancelAll();
        me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 198599 - Thunderstrike (Valarjar Thundercaller)
class spell_hov_thunderstrike : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_THUNDERSTRIKE_DAMAGE });
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        Unit* target = GetTarget();
        Unit* caster = GetCaster();
        if (!caster)
            caster = target->FindNearestCreature(NPC_VALARJAR_THUNDERCALLER, 40.0f);

        if (caster)
            caster->CastSpell(target, SPELL_THUNDERSTRIKE_DAMAGE, true);
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_hov_thunderstrike::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_hymdall()
{
    RegisterHallsOfValorCreatureAI(boss_hymdall);
    RegisterHallsOfValorCreatureAI(npc_hymdall_storm_drake);

    RegisterSpellScript(spell_hov_thunderstrike);
}
