/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_CRAFTING_SERVICE_H
#define TRINITYCORE_RETAIL_CRAFTING_SERVICE_H

#include "Define.h"

class Player;
class Spell;
class SpellCastTargets;
class SpellEffectInfo;
class SpellInfo;
enum SpellCastResult : int32;

// Quality crafting for SPELL_EFFECT_CRAFT_ITEM (288).
// EffectMiscValue[0] is the CraftingData ID; modified crafting reagents come from the cast request.
namespace RetailSystems::Crafting
{
// Validates the modified crafting reagents of a cast (called from Spell::CheckItems)
TC_GAME_API SpellCastResult CheckCraft(Player const* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets);

// Resolves quality, creates the crafted item and consumes the modified crafting reagents
TC_GAME_API SpellCastResult Craft(Player* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets);

// SPELL_EFFECT_CREATE_ITEM with optional modified crafting reagents: reagent bonus trees are applied to the created item
// and ModifiedCraftingReagentItem::ItemContextOffset raises the item context given by EffectMiscValue[0]
TC_GAME_API SpellCastResult CheckCreateItemReagents(Player const* player, SpellInfo const* spellInfo, SpellCastTargets const& targets);
TC_GAME_API SpellCastResult CreateItemWithReagents(Spell* spell, Player* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets);
}

#endif // TRINITYCORE_RETAIL_CRAFTING_SERVICE_H
