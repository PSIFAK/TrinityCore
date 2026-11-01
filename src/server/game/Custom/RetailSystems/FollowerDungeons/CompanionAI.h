/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_COMPANION_AI_H
#define TRINITYCORE_RETAIL_COMPANION_AI_H

#include "ScriptedCreature.h"
#include "Duration.h"
#include <unordered_map>
#include <vector>

namespace RetailSystems::FollowerDungeons
{
enum class CompanionRole : uint8
{
    Tank,
    Healer,
    Damage
};

enum class CompanionTarget : uint8
{
    Victim,     // current enemy
    Self,
    Heal,       // the most hurt ally below HealthPct
    Shield,     // the tank or the leader without the spell's aura (tank first)
    Taunt,      // an enemy attacking someone else (tank only)
    Charge      // current enemy farther than melee range
};

struct CompanionSpell
{
    uint32 SpellId = 0;
    CompanionTarget Target = CompanionTarget::Victim;
    Milliseconds Cooldown = 0ms;
    uint8 HealthPct = 100;     // Heal: cast on allies below this health
    uint8 MinEnemies = 0;      // area spells: enemies within 8 yards
};

// SetGUID id of the player the companion follows and protects
constexpr int32 DATA_COMPANION_LEADER = 1;

// Party ally driven by a role: follows its leader, never pulls on its own, fights what attacks the
// leader or the other companions, and casts its spells by priority. It does not die (stops at 1 health
// point), as there is no way yet to bring it back. Used by the Darkmaul Citadel allies and by the follower
// dungeon NPCs.
class TC_GAME_API CompanionAI : public ScriptedAI
{
public:
    CompanionAI(Creature* creature, CompanionRole role, std::vector<CompanionSpell> spells, float followDistance, float followAngle, bool ranged);

    void SetGUID(ObjectGuid const& guid, int32 id) override;
    ObjectGuid GetGUID(int32 id) const override;

    CompanionRole GetRole() const { return _role; }
    Player* GetLeader() const;

    void Reset() override;
    void MoveInLineOfSight(Unit* /*who*/) override { }
    void JustEngagedWith(Unit* who) override;
    void EnterEvadeMode(EvadeReason why) override;
    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType) override;
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType damageType, SpellInfo const* spellInfo) override;
    void UpdateAI(uint32 diff) override;

private:
    void Think();
    void ReturnToLeader();
    std::vector<Creature*> GetCompanions() const;
    std::vector<Unit*> GetAllies(Player* leader) const;
    std::vector<Unit*> GetThreats(Player* leader) const;
    Unit* SelectEnemy(Player* leader, std::vector<Unit*> const& threats) const;
    Unit* SelectSpellTarget(CompanionSpell const& spell, Player* leader, std::vector<Unit*> const& threats) const;
    bool CastSpells(Player* leader, std::vector<Unit*> const& threats);

    CompanionRole _role;
    std::vector<CompanionSpell> _spells;
    float _followDistance;
    float _followAngle;
    bool _ranged;
    ObjectGuid _leaderGuid;
    std::unordered_map<uint32, TimePoint> _spellReady;
    Milliseconds _thinkTimer;
};
}

#endif // TRINITYCORE_RETAIL_COMPANION_AI_H
