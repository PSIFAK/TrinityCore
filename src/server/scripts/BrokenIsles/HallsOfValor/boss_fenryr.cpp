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
 * - There is no creature_text for Fenryr in the world database, so no Talk() calls are made.
 * - The trash version of Fenryr (95674) is spawned in the world database; at 60% health he flees to his lair, where
 *   the instance script summons the boss version (99868, no world database spawn) at 60% health.
 * - Ravenous Leap: the original filter/force-cast chain (197549 / 197558 / 197560) is replaced by a scripted chain of
 *   jumps (196495) to every player in range, the jump itself triggers the impact (196496) which applies the bleed (196497).
 * - The "Field of the Eternal Hunt" / "Halls of Valor" conversation triggers (101697, 101712) are not ported: their
 *   conversations (1179, 1186) are not in the world database and the creatures have neither gossip nor spellclick data.
 */

#include "Containers.h"
#include "Creature.h"
#include "InstanceScript.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "halls_of_valor.h"

enum FenryrSpells
{
    SPELL_UNNERVING_HOWL        = 196543,
    SPELL_RAVENOUS_LEAP         = 197558,
    SPELL_RAVENOUS_LEAP_JUMP    = 196495,
    SPELL_RAVENOUS_LEAP_IMPACT  = 196496,
    SPELL_RAVENOUS_LEAP_DOT     = 196497,
    SPELL_CLAW_FRENZY           = 196512,
    SPELL_SCENT_OF_BLOOD        = 198800,
    SPELL_SCENT_OF_BLOOD_FIXATE = 196838,
    SPELL_SCENT_OF_BLOOD_BUFF   = 196801,
    SPELL_SCENT_OF_BLOOD_DAMAGE = 196828,

    // Heroic+
    SPELL_SUMMON_PACKMATE       = 207708,
    SPELL_PACKMATE_STEALTH      = 207707,
    SPELL_STRENGTH_OF_THE_PACK  = 199186,
    SPELL_STRENGTH_PACK_SEARCH  = 200922,
    SPELL_STRENGTH_PACK_BUFF    = 199184,

    // Cosmetic
    SPELL_STEALTH               = 196567,
    SPELL_LICKING_WOUNDS        = 200561,
    SPELL_BLOOD_SPLAT           = 200553
};

enum FenryrEvents
{
    EVENT_UNNERVING_HOWL = 1,
    EVENT_RAVENOUS_LEAP,
    EVENT_CLAW_FRENZY,
    EVENT_SCENT_OF_BLOOD,
    EVENT_SUMMON_PACKMATES
};

enum FenryrMisc
{
    POINT_FENRYR_LAIR   = 1
};

class RavenousLeapChain
{
public:
    void Start(Creature* me)
    {
        _targets.clear();

        std::list<Player*> players;
        me->GetPlayerListInGrid(players, 100.0f);
        for (Player* player : players)
            if (player->IsAlive() && !player->IsGameMaster())
                _targets.push_back(player->GetGUID());

        Trinity::Containers::RandomShuffle(_targets);
        Next(me);
    }

    void Next(Creature* me)
    {
        while (!_targets.empty())
        {
            ObjectGuid guid = _targets.back();
            _targets.pop_back();

            if (Player* target = ObjectAccessor::GetPlayer(*me, guid))
            {
                if (target->IsAlive() && me->IsWithinDistInMap(target, 100.0f))
                {
                    me->CastSpell(target, SPELL_RAVENOUS_LEAP_JUMP, true);
                    return;
                }
            }
        }
    }

    void Clear() { _targets.clear(); }

private:
    std::vector<ObjectGuid> _targets;
};

// 99868 - Fenryr
struct boss_fenryr : public BossAI
{
    boss_fenryr(Creature* creature) : BossAI(creature, DATA_FENRYR) { }

    void Reset() override
    {
        _Reset();
        _leapChain.Clear();
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RAVENOUS_LEAP_DOT);

        // Fenryr starts the encounter wounded from the first fight and does not regenerate
        me->SetRegenerateHealth(false);
        me->SetHealth(me->CountPctFromMaxHealth(60));
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        events.ScheduleEvent(EVENT_UNNERVING_HOWL, 3s);
        events.ScheduleEvent(EVENT_SCENT_OF_BLOOD, 9s);
        events.ScheduleEvent(EVENT_RAVENOUS_LEAP, 28s);
        events.ScheduleEvent(EVENT_CLAW_FRENZY, 40s);

        if (IsHeroicOrHigher())
            events.ScheduleEvent(EVENT_SUMMON_PACKMATES, 4s);
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
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RAVENOUS_LEAP_DOT);
    }

    void OnSpellCast(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_RAVENOUS_LEAP)
            _leapChain.Start(me);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP)
            _leapChain.Next(me);
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_RAVENOUS_LEAP_IMPACT)
            return;

        if (Unit* unitTarget = target->ToUnit())
            DoCast(unitTarget, SPELL_RAVENOUS_LEAP_DOT, true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (me->GetDistance(me->GetHomePosition()) >= 60.0f)
        {
            EnterEvadeMode(EvadeReason::Boundary);
            return;
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_UNNERVING_HOWL:
                    DoCastAOE(SPELL_UNNERVING_HOWL);
                    events.Repeat(36s);
                    break;
                case EVENT_RAVENOUS_LEAP:
                    DoCastSelf(SPELL_RAVENOUS_LEAP);
                    events.Repeat(36s);
                    break;
                case EVENT_CLAW_FRENZY:
                    DoCastAOE(SPELL_CLAW_FRENZY);
                    events.Repeat(20s);
                    break;
                case EVENT_SCENT_OF_BLOOD:
                    DoCastAOE(SPELL_SCENT_OF_BLOOD, true);
                    events.Repeat(38s);
                    break;
                case EVENT_SUMMON_PACKMATES:
                    for (uint8 i = 0; i < 3; ++i)
                        me->CastSpell(me->GetNearPosition(40.0f, float(i)), SPELL_SUMMON_PACKMATE, true);
                    events.Repeat(36s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    RavenousLeapChain _leapChain;
};

// 95674 - Fenryr (first encounter, flees to his lair)
struct npc_hov_fenryr : public ScriptedAI
{
    npc_hov_fenryr(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()), _fled(false) { }

    void JustAppeared() override
    {
        if (_instance->GetData(DATA_FENRYR_EVENT) == DONE)
            me->DespawnOrUnsummon();
    }

    void Reset() override
    {
        _events.Reset();
        _leapChain.Clear();
        _instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RAVENOUS_LEAP_DOT);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        DoZoneInCombat();
        _events.ScheduleEvent(EVENT_UNNERVING_HOWL, 7s);
        _events.ScheduleEvent(EVENT_RAVENOUS_LEAP, 12s);
        _events.ScheduleEvent(EVENT_CLAW_FRENZY, 20s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_fled)
        {
            damage = 0;
            return;
        }

        if (!me->HealthBelowPctDamaged(60, damage))
            return;

        damage = 0;
        _fled = true;
        _events.Reset();
        _leapChain.Clear();
        _instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RAVENOUS_LEAP_DOT);

        me->AttackStop();
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToAll(true);
        me->CombatStop(true);
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveIdle();

        _scheduler.Schedule(5s, [this](TaskContext /*task*/)
        {
            DoCastSelf(SPELL_LICKING_WOUNDS, true);
            DoCastSelf(SPELL_BLOOD_SPLAT, true);
            DoCastSelf(SPELL_STEALTH, true);

            _instance->SetData(DATA_FENRYR_EVENT, DONE);
            me->GetMotionMaster()->MovePoint(POINT_FENRYR_LAIR, FenryrLairPos);
        });
    }

    void OnSpellCast(SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_RAVENOUS_LEAP)
            _leapChain.Start(me);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP)
            _leapChain.Next(me);
        else if (type == POINT_MOTION_TYPE && id == POINT_FENRYR_LAIR)
            me->DespawnOrUnsummon();
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_RAVENOUS_LEAP_IMPACT)
            return;

        if (Unit* unitTarget = target->ToUnit())
            DoCast(unitTarget, SPELL_RAVENOUS_LEAP_DOT, true);
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        if (_fled || !UpdateVictim())
            return;

        if (me->GetDistance(me->GetHomePosition()) >= 50.0f)
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
                case EVENT_UNNERVING_HOWL:
                    DoCastAOE(SPELL_UNNERVING_HOWL);
                    _events.Repeat(32s);
                    break;
                case EVENT_RAVENOUS_LEAP:
                    DoCastSelf(SPELL_RAVENOUS_LEAP);
                    _events.Repeat(34s);
                    break;
                case EVENT_CLAW_FRENZY:
                    DoCastAOE(SPELL_CLAW_FRENZY);
                    _events.Repeat(10s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    InstanceScript* _instance;
    EventMap _events;
    TaskScheduler _scheduler;
    RavenousLeapChain _leapChain;
    bool _fled;
};

// 99922 - Ebonclaw Packmate
struct npc_fenryr_ebonclaw_packmate : public ScriptedAI
{
    npc_fenryr_ebonclaw_packmate(Creature* creature) : ScriptedAI(creature), _stealthed(true) { }

    void Reset() override
    {
        DoCastSelf(SPELL_STRENGTH_OF_THE_PACK, true);
        DoCastSelf(SPELL_PACKMATE_STEALTH, true);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // Template faction is friendly and the summon properties carry no faction, inherit the summoner's
        me->SetFaction(summoner->GetFaction());
        _stealthed = true;
        me->SetSpeedRate(MOVE_RUN, 0.4f);
        DoZoneInCombat();
    }

    void SpellHitTarget(WorldObject* target, SpellInfo const* spellInfo) override
    {
        // Area entry target without conditions, only other packmates are valid
        if (spellInfo->Id != SPELL_STRENGTH_PACK_SEARCH || target->GetEntry() != NPC_EBONCLAW_PACKMATE)
            return;

        if (Unit* unitTarget = target->ToUnit())
            unitTarget->CastSpell(unitTarget, SPELL_STRENGTH_PACK_BUFF, true);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;

        if (_stealthed && !me->HasAura(SPELL_PACKMATE_STEALTH))
        {
            _stealthed = false;
            me->SetSpeedRate(MOVE_RUN, 1.0f);
        }
    }

private:
    bool _stealthed;
};

// 198800 - Scent of Blood
class spell_fenryr_scent_of_blood : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SCENT_OF_BLOOD_FIXATE });
    }

    void FilterTargets(std::list<WorldObject*>& targets) const
    {
        if (targets.size() > 1)
            if (Unit* victim = GetCaster()->GetVictim())
                targets.remove(victim);

        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleDummy(SpellEffIndex /*effIndex*/) const
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_SCENT_OF_BLOOD_FIXATE, CastSpellExtraArgsInit{
            .TriggerFlags = TRIGGERED_FULL_MASK,
            .TriggeringSpell = GetSpell()
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_fenryr_scent_of_blood::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_fenryr_scent_of_blood::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 196838 - Scent of Blood
class spell_fenryr_scent_of_blood_fixate : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SCENT_OF_BLOOD_BUFF, SPELL_SCENT_OF_BLOOD_DAMAGE });
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        Creature* fenryr = Object::ToCreature(GetCaster());
        if (!fenryr || !fenryr->IsAIEnabled())
            return;

        fenryr->CastSpell(fenryr, SPELL_SCENT_OF_BLOOD_BUFF, true);
        fenryr->AI()->AttackStart(GetTarget());
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        Unit* fenryr = GetCaster();
        if (!fenryr)
            return;

        fenryr->RemoveAurasDueToSpell(SPELL_SCENT_OF_BLOOD_BUFF);
        fenryr->RemoveAurasDueToSpell(SPELL_SCENT_OF_BLOOD_DAMAGE);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_fenryr_scent_of_blood_fixate::HandleApply, EFFECT_0, SPELL_AURA_MOD_FIXATE, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_fenryr_scent_of_blood_fixate::HandleRemove, EFFECT_0, SPELL_AURA_MOD_FIXATE, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_fenryr()
{
    RegisterHallsOfValorCreatureAI(boss_fenryr);
    RegisterHallsOfValorCreatureAI(npc_hov_fenryr);
    RegisterHallsOfValorCreatureAI(npc_fenryr_ebonclaw_packmate);

    RegisterSpellScript(spell_fenryr_scent_of_blood);
    RegisterSpellScript(spell_fenryr_scent_of_blood_fixate);
}
