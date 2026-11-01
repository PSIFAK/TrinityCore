/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CompanionAI.h"
#include "GameTime.h"
#include "Group.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellDefines.h"
#include <algorithm>

namespace RetailSystems::FollowerDungeons
{
namespace
{
constexpr Milliseconds THINK_INTERVAL = 500ms;
constexpr float COMPANION_SEARCH_RANGE = 80.0f;
constexpr float THREAT_RANGE = 50.0f;           // enemies farther than this from the leader are ignored
constexpr float CATCH_UP_DISTANCE = 80.0f;      // out of combat, a companion this far behind teleports to the leader
constexpr float COMPANION_MELEE_RANGE = 8.0f;
constexpr float CHARGE_MAX_RANGE = 25.0f;
constexpr float RANGED_DISTANCE = 25.0f;

bool IsEnemy(Unit const* me, Unit const* unit)
{
    return unit && unit->IsAlive() && unit->IsInWorld() && me->IsValidAttackTarget(unit);
}
}

CompanionAI::CompanionAI(Creature* creature, CompanionRole role, std::vector<CompanionSpell> spells, float followDistance, float followAngle, bool ranged)
    : ScriptedAI(creature), _role(role), _spells(std::move(spells)), _followDistance(followDistance), _followAngle(followAngle), _ranged(ranged),
    _thinkTimer(THINK_INTERVAL)
{
}

void CompanionAI::SetGUID(ObjectGuid const& guid, int32 id)
{
    if (id != DATA_COMPANION_LEADER)
        return;

    _leaderGuid = guid;
    if (!me->IsInCombat())
        ReturnToLeader();
}

ObjectGuid CompanionAI::GetGUID(int32 id) const
{
    return id == DATA_COMPANION_LEADER ? _leaderGuid : ObjectGuid::Empty;
}

Player* CompanionAI::GetLeader() const
{
    Player* leader = ObjectAccessor::GetPlayer(*me, _leaderGuid);
    if (leader && leader->IsInWorld() && leader->GetMap() == me->GetMap())
        return leader;

    return nullptr;
}

void CompanionAI::Reset()
{
    _spellReady.clear();
    _thinkTimer = THINK_INTERVAL;
}

void CompanionAI::JustEngagedWith(Unit* /*who*/)
{
    // the next think picks the target by role
    _thinkTimer = 0ms;
}

void CompanionAI::EnterEvadeMode(EvadeReason /*why*/)
{
    // never run back to the spawn point: go back to the leader
    me->CombatStop(true);
    me->SetLastDamagedTime(0);
    ReturnToLeader();
}

void CompanionAI::DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/)
{
    // kills by companions count for the leader: loot, quest credit and full experience
    Creature* creature = victim ? victim->ToCreature() : nullptr;
    if (!creature)
        return;

    if (Player* leader = GetLeader())
    {
        if (!creature->hasLootRecipient())
            creature->SetTappedBy(leader);
        creature->LowerPlayerDamageReq(damage);
    }
}

void CompanionAI::DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/)
{
    if (damage >= me->GetHealth())
        damage = me->GetHealth() > 1 ? me->GetHealth() - 1 : 0;
}

void CompanionAI::UpdateAI(uint32 diff)
{
    _thinkTimer -= Milliseconds(diff);
    if (_thinkTimer <= 0ms)
    {
        _thinkTimer = THINK_INTERVAL;
        Think();
    }

    if (!_ranged && me->GetVictim() && !me->HasUnitState(UNIT_STATE_CASTING))
        me->DoMeleeAttackIfReady();
}

void CompanionAI::ReturnToLeader()
{
    if (Player* leader = GetLeader())
        me->GetMotionMaster()->MoveFollow(leader, _followDistance, _followAngle);
}

std::vector<Creature*> CompanionAI::GetCompanions() const
{
    std::vector<Creature*> companions;
    std::list<Creature*> creatures;
    me->GetCreatureListWithOptionsInGrid(creatures, COMPANION_SEARCH_RANGE, { .IsAlive = FindCreatureAliveState::Alive });
    for (Creature* creature : creatures)
    {
        if (creature == me)
            continue;

        if (CompanionAI const* ai = dynamic_cast<CompanionAI const*>(creature->AI()))
            if (ai->_leaderGuid == _leaderGuid)
                companions.push_back(creature);
    }

    return companions;
}

std::vector<Unit*> CompanionAI::GetAllies(Player* leader) const
{
    std::vector<Unit*> allies;
    allies.push_back(me);
    if (leader->IsAlive())
        allies.push_back(leader);

    if (Group* group = leader->GetGroup())
        for (GroupReference const& itr : group->GetMembers())
            if (Player* member = itr.GetSource())
                if (member != leader && member->IsAlive() && member->GetMap() == me->GetMap())
                    allies.push_back(member);

    for (Creature* companion : GetCompanions())
        allies.push_back(companion);

    return allies;
}

std::vector<Unit*> CompanionAI::GetThreats(Player* leader) const
{
    std::vector<Unit*> threats;
    auto add = [&](Unit* unit)
    {
        if (IsEnemy(me, unit) && unit->GetExactDist(leader) <= THREAT_RANGE && std::find(threats.begin(), threats.end(), unit) == threats.end())
            threats.push_back(unit);
    };

    for (Unit* ally : GetAllies(leader))
        for (Unit* attacker : ally->getAttackers())
            add(attacker);

    // the leader pulls: its target is fair game even before it fights back
    if (leader->IsInCombat())
        add(leader->GetVictim());

    return threats;
}

Unit* CompanionAI::SelectEnemy(Player* leader, std::vector<Unit*> const& threats) const
{
    if (threats.empty())
        return nullptr;

    if (_role == CompanionRole::Tank)
    {
        // pick up whatever hits someone else first
        for (Unit* threat : threats)
            if (Unit* victim = threat->GetVictim())
                if (victim != me)
                    return threat;
    }
    else
    {
        // assist the tank, then the leader
        for (Creature* companion : GetCompanions())
            if (CompanionAI const* ai = dynamic_cast<CompanionAI const*>(companion->AI()))
                if (ai->_role == CompanionRole::Tank && IsEnemy(me, companion->GetVictim()))
                    return companion->GetVictim();

        if (Unit* victim = leader->GetVictim())
            if (std::find(threats.begin(), threats.end(), victim) != threats.end())
                return victim;
    }

    if (Unit* victim = me->GetVictim())
        if (std::find(threats.begin(), threats.end(), victim) != threats.end())
            return victim;

    return *std::min_element(threats.begin(), threats.end(), [this](Unit const* a, Unit const* b)
    {
        return me->GetExactDistSq(a) < me->GetExactDistSq(b);
    });
}

Unit* CompanionAI::SelectSpellTarget(CompanionSpell const& spell, Player* leader, std::vector<Unit*> const& threats) const
{
    switch (spell.Target)
    {
        case CompanionTarget::Victim:
            return me->GetVictim();
        case CompanionTarget::Self:
            return me;
        case CompanionTarget::Heal:
        {
            Unit* target = nullptr;
            for (Unit* ally : GetAllies(leader))
                if (ally->GetHealthPct() < spell.HealthPct && (!target || ally->GetHealthPct() < target->GetHealthPct()))
                    target = ally;
            return target;
        }
        case CompanionTarget::Shield:
        {
            if (threats.empty())
                return nullptr;

            std::vector<Unit*> candidates;
            for (Creature* companion : GetCompanions())
                if (CompanionAI const* ai = dynamic_cast<CompanionAI const*>(companion->AI()))
                    if (ai->_role == CompanionRole::Tank)
                        candidates.push_back(companion);
            if (leader->IsAlive())
                candidates.push_back(leader);

            for (Unit* candidate : candidates)
                if (!candidate->HasAura(spell.SpellId))
                    return candidate;
            return nullptr;
        }
        case CompanionTarget::Taunt:
            for (Unit* threat : threats)
                if (Unit* victim = threat->GetVictim())
                    if (victim != me)
                        return threat;
            return nullptr;
        case CompanionTarget::Charge:
            if (Unit* victim = me->GetVictim())
                if (me->GetExactDist(victim) > COMPANION_MELEE_RANGE && me->GetExactDist(victim) < CHARGE_MAX_RANGE)
                    return victim;
            return nullptr;
        default:
            return nullptr;
    }
}

bool CompanionAI::CastSpells(Player* leader, std::vector<Unit*> const& threats)
{
    TimePoint now = GameTime::Now();
    for (CompanionSpell const& spell : _spells)
    {
        auto itr = _spellReady.find(spell.SpellId);
        if (itr != _spellReady.end() && itr->second > now)
            continue;

        if (spell.MinEnemies)
        {
            uint8 nearby = uint8(std::count_if(threats.begin(), threats.end(), [this](Unit const* threat) { return me->GetExactDist(threat) <= COMPANION_MELEE_RANGE; }));
            if (nearby < spell.MinEnemies)
                continue;
        }

        Unit* target = SelectSpellTarget(spell, leader, threats);
        if (!target)
            continue;

        // companions cast class spells of players: no mana, holy power or reagents to track
        if (me->CastSpell(target, spell.SpellId, CastSpellExtraArgs(TriggerCastFlags(TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_SHAPESHIFT))) == SPELL_CAST_OK)
        {
            _spellReady[spell.SpellId] = now + spell.Cooldown;
            return true;
        }
    }

    return false;
}

void CompanionAI::Think()
{
    if (!me->IsAlive() || me->HasUnitState(UNIT_STATE_CASTING))
        return;

    Player* leader = GetLeader();
    if (!leader)
    {
        // nobody set the leader yet (or it left): protect the first player in the instance
        for (MapReference const& ref : me->GetMap()->GetPlayers())
            if (Player* player = ref.GetSource())
                if (player->IsAlive() && !player->IsGameMaster())
                {
                    _leaderGuid = player->GetGUID();
                    leader = player;
                    ReturnToLeader();
                    break;
                }

        if (!leader)
            return;
    }

    if (!me->IsInCombat() && !leader->IsInCombat() && leader->IsAlive() && me->GetExactDist(leader) > CATCH_UP_DISTANCE)
    {
        me->NearTeleportTo(leader->GetNearPosition(_followDistance, _followAngle));
        ReturnToLeader();
        return;
    }

    std::vector<Unit*> threats = GetThreats(leader);
    if (Unit* enemy = SelectEnemy(leader, threats))
    {
        if (me->GetVictim() != enemy)
        {
            if (_ranged)
                AttackStartCaster(enemy, RANGED_DISTANCE);
            else
                AttackStart(enemy);
        }
    }
    else if (me->GetVictim() || me->IsInCombat())
        EnterEvadeMode(EvadeReason::NoHostiles);
    else if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
        ReturnToLeader();

    CastSpells(leader, threats);
}
}
