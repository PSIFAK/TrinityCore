/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CompanionAI.h"
#include "FollowerDungeonService.h"
#include "ScriptMgr.h"

using namespace RetailSystems::FollowerDungeons;

// NPC members of a follower dungeon party (FollowerDungeonService). The retail follower spells are not in the
// 12.0.7 client data, so they cast the class spells of their specialization.
namespace
{
enum FollowerSpells
{
    // Captain Garrick, Protection paladin
    SPELL_HAND_OF_RECKONING    = 62124,
    SPELL_AVENGERS_SHIELD      = 31935,
    SPELL_SHIELD_OF_RIGHTEOUS  = 53600,
    SPELL_CONSECRATION         = 26573,

    // Crenna Earth-Daughter, Restoration druid
    SPELL_LIFEBLOOM            = 33763,
    SPELL_SWIFTMEND            = 419458,
    SPELL_REGROWTH             = 419287,
    SPELL_REJUVENATION         = 419204,
    SPELL_WRATH                = 5176,

    // Meredy Huntswell, Fire mage
    SPELL_FIRE_BLAST           = 108853,
    SPELL_FLAMESTRIKE          = 2120,
    SPELL_FIREBALL             = 133,

    // Austin Huxworth, Marksmanship hunter
    SPELL_AIMED_SHOT           = 19434,
    SPELL_MULTI_SHOT           = 2643,
    SPELL_STEADY_SHOT          = 56641,

    // Shuja Grimaxe, Elemental shaman
    SPELL_FLAME_SHOCK          = 429619,
    SPELL_CHAIN_LIGHTNING      = 429652,
    SPELL_LIGHTNING_BOLT       = 429663
};

constexpr Milliseconds PARTY_CHECK_INTERVAL = 2s;

struct FollowerDungeonNpcAI : public CompanionAI
{
    using CompanionAI::CompanionAI;

    void UpdateAI(uint32 diff) override
    {
        // the party is gone (left the dungeon, disbanded): so is the NPC
        _partyCheckTimer -= Milliseconds(diff);
        if (_partyCheckTimer <= 0ms)
        {
            _partyCheckTimer = PARTY_CHECK_INTERVAL;
            if (!IsPartyMember(me))
            {
                me->DespawnOrUnsummon();
                return;
            }
        }

        CompanionAI::UpdateAI(diff);
    }

private:
    Milliseconds _partyCheckTimer = PARTY_CHECK_INTERVAL;
};

// 209057 - Captain Garrick
struct npc_follower_captain_garrick : public FollowerDungeonNpcAI
{
    npc_follower_captain_garrick(Creature* creature) : FollowerDungeonNpcAI(creature, CompanionRole::Tank,
    {
        { SPELL_HAND_OF_RECKONING,   CompanionTarget::Taunt,  8s },
        { SPELL_AVENGERS_SHIELD,     CompanionTarget::Charge, 15s },
        { SPELL_SHIELD_OF_RIGHTEOUS, CompanionTarget::Victim, 8s },
        { SPELL_CONSECRATION,        CompanionTarget::Self,   12s, 100, 1 }
    }, 3.0f, 0.0f, false) { }
};

// 209072 - Crenna Earth-Daughter
struct npc_follower_crenna_earth_daughter : public FollowerDungeonNpcAI
{
    npc_follower_crenna_earth_daughter(Creature* creature) : FollowerDungeonNpcAI(creature, CompanionRole::Healer,
    {
        { SPELL_LIFEBLOOM,    CompanionTarget::Shield, 15s },
        { SPELL_SWIFTMEND,    CompanionTarget::Heal,   15s, 50 },
        { SPELL_REGROWTH,     CompanionTarget::Heal,   0s,  75 },
        { SPELL_REJUVENATION, CompanionTarget::Heal,   3s,  95 },
        { SPELL_WRATH,        CompanionTarget::Victim, 0s }
    }, 6.0f, float(M_PI), true) { }
};

// 209059 - Meredy Huntswell
struct npc_follower_meredy_huntswell : public FollowerDungeonNpcAI
{
    npc_follower_meredy_huntswell(Creature* creature) : FollowerDungeonNpcAI(creature, CompanionRole::Damage,
    {
        { SPELL_FIRE_BLAST,  CompanionTarget::Victim, 12s },
        { SPELL_FLAMESTRIKE, CompanionTarget::Victim, 15s, 100, 3 },
        { SPELL_FIREBALL,    CompanionTarget::Victim, 0s }
    }, 4.0f, float(M_PI) * 0.75f, true) { }
};

// 209065 - Austin Huxworth
struct npc_follower_austin_huxworth : public FollowerDungeonNpcAI
{
    npc_follower_austin_huxworth(Creature* creature) : FollowerDungeonNpcAI(creature, CompanionRole::Damage,
    {
        { SPELL_AIMED_SHOT,  CompanionTarget::Victim, 12s },
        { SPELL_MULTI_SHOT,  CompanionTarget::Victim, 8s, 100, 2 },
        { SPELL_STEADY_SHOT, CompanionTarget::Victim, 0s }
    }, 4.0f, float(M_PI) * 1.25f, true) { }
};

// 214390 - Shuja Grimaxe
struct npc_follower_shuja_grimaxe : public FollowerDungeonNpcAI
{
    npc_follower_shuja_grimaxe(Creature* creature) : FollowerDungeonNpcAI(creature, CompanionRole::Damage,
    {
        { SPELL_FLAME_SHOCK,     CompanionTarget::Victim, 12s },
        { SPELL_CHAIN_LIGHTNING, CompanionTarget::Victim, 6s, 100, 2 },
        { SPELL_LIGHTNING_BOLT,  CompanionTarget::Victim, 0s }
    }, 4.0f, float(M_PI) / 2.0f, true) { }
};
}

void AddSC_follower_dungeon_npcs()
{
    RegisterCreatureAI(npc_follower_captain_garrick);
    RegisterCreatureAI(npc_follower_crenna_earth_daughter);
    RegisterCreatureAI(npc_follower_meredy_huntswell);
    RegisterCreatureAI(npc_follower_austin_huxworth);
    RegisterCreatureAI(npc_follower_shuja_grimaxe);
}
