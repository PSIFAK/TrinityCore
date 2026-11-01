/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CompanionAI.h"
#include "ScriptMgr.h"
#include "darkmaul_citadel.h"

using namespace RetailSystems::FollowerDungeons;

// The three allies at the entrance of Darkmaul Citadel are not party members: retail spawns them with the
// instance, they follow the player, tank, heal and deal damage. Spells and cooldowns from the retail sniff (12.0.1.66709).
namespace
{
enum DarkmaulAllySpells
{
    // Captain Garrick
    SPELL_GARRICK_TAUNT        = 274217,
    SPELL_GARRICK_CHARGE       = 283407,
    SPELL_GARRICK_SHIELD_SLAM  = 318125,
    SPELL_GARRICK_WHIRLWIND    = 318151,

    // Henry Garrick
    // retail casts the area selectors 344426/344425, whose script puts these on the chosen ally
    SPELL_HENRY_POWER_WORD_SHIELD = 318158,
    SPELL_HENRY_FLASH_HEAL        = 314655,
    SPELL_HENRY_SHADOW_WORD_PAIN  = 318154,
    SPELL_HENRY_HOLY_FIRE         = 318159,
    SPELL_HENRY_SMITE             = 314658,

    // Lady Jaina Proudmoore
    SPELL_JAINA_CONE_OF_COLD   = 279565,
    SPELL_JAINA_ICE_LANCE      = 279567,
    SPELL_JAINA_FROSTBOLT      = 328496
};

// 161350 - Captain Garrick
struct npc_darkmaul_captain_garrick : public CompanionAI
{
    npc_darkmaul_captain_garrick(Creature* creature) : CompanionAI(creature, CompanionRole::Tank,
    {
        { SPELL_GARRICK_TAUNT,       CompanionTarget::Taunt,  10s },
        { SPELL_GARRICK_CHARGE,      CompanionTarget::Charge, 40s },
        { SPELL_GARRICK_SHIELD_SLAM, CompanionTarget::Victim, 20s },
        { SPELL_GARRICK_WHIRLWIND,   CompanionTarget::Victim, 10s, 100, 2 }
    }, 3.0f, 0.0f, false) { }
};

// 161504 - Henry Garrick
struct npc_darkmaul_henry_garrick : public CompanionAI
{
    npc_darkmaul_henry_garrick(Creature* creature) : CompanionAI(creature, CompanionRole::Healer,
    {
        { SPELL_HENRY_POWER_WORD_SHIELD, CompanionTarget::Shield, 7s },
        { SPELL_HENRY_FLASH_HEAL,        CompanionTarget::Heal,   2s, 90 },
        { SPELL_HENRY_SHADOW_WORD_PAIN,  CompanionTarget::Victim, 20s },
        { SPELL_HENRY_HOLY_FIRE,         CompanionTarget::Victim, 10s },
        { SPELL_HENRY_SMITE,             CompanionTarget::Victim, 0s }
    }, 5.0f, float(M_PI), true) { }
};

// 245401 - Lady Jaina Proudmoore
struct npc_darkmaul_jaina : public CompanionAI
{
    npc_darkmaul_jaina(Creature* creature) : CompanionAI(creature, CompanionRole::Damage,
    {
        { SPELL_JAINA_CONE_OF_COLD, CompanionTarget::Victim, 15s },
        { SPELL_JAINA_ICE_LANCE,    CompanionTarget::Victim, 10s },
        { SPELL_JAINA_FROSTBOLT,    CompanionTarget::Victim, 0s }
    }, 4.0f, float(M_PI) * 0.75f, true) { }
};
}

void AddSC_npc_darkmaul_allies()
{
    RegisterDarkmaulCitadelCreatureAI(npc_darkmaul_captain_garrick);
    RegisterDarkmaulCitadelCreatureAI(npc_darkmaul_henry_garrick);
    RegisterDarkmaulCitadelCreatureAI(npc_darkmaul_jaina);
}
