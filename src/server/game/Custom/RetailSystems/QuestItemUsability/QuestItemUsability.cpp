/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// The cursor over a creature turns into the "use item" cursor when one of the quest items in the bags can be used on it.
// The client asks the server for each creature it hovers (CMSG_QUERY_QUEST_ITEM_USABILITY); without an answer it
// never shows the cursor. Retail (12.1.5 sniff): the First Aid Kit of quest 54951 is usable on the three wounded
// survivors only, the Re-Sizer of quest 56034 on the Wandering Boars only.

#include "ConditionMgr.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestItemUsabilityPackets.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "WorldSession.h"
#include <algorithm>
#include <memory>

SpellCastResult Spell::CheckCastWithoutCasting(SpellCastTargets const& targets, bool& scriptChecksTarget)
{
    InitExplicitTargets(targets);
    LoadScripts();
    scriptChecksTarget = std::ranges::any_of(m_loadedScripts, [](SpellScript* script) { return script->OnCheckCast.size() != 0; });
    return CheckCast(true);
}

namespace
{
// the quest the item belongs to: given on accept, its starter, or dropped for it
bool QuestUsesItem(Quest const* quest, ItemTemplate const* proto)
{
    if (quest->GetSrcItemId() == proto->GetId() || proto->GetStartQuest() == quest->GetQuestId())
        return true;

    return std::ranges::find(quest->ItemDrop, proto->GetId()) != quest->ItemDrop.end();
}

// errors that only depend on where the player stands or what it does right now: the cursor still shows
bool IsTransientCastError(SpellCastResult result)
{
    switch (result)
    {
        case SPELL_CAST_OK:
        case SPELL_FAILED_OUT_OF_RANGE:
        case SPELL_FAILED_LINE_OF_SIGHT:
        case SPELL_FAILED_UNIT_NOT_INFRONT:
        case SPELL_FAILED_NOT_INFRONT:
        case SPELL_FAILED_NOT_BEHIND:
        case SPELL_FAILED_MOVING:
        case SPELL_FAILED_NOT_READY:
        case SPELL_FAILED_ITEM_NOT_READY:
        case SPELL_FAILED_SPELL_IN_PROGRESS:
        case SPELL_FAILED_NOT_STANDING:
        case SPELL_FAILED_AFFECTING_COMBAT:
        case SPELL_FAILED_TOO_CLOSE:
            return true;
        default:
            return false;
    }
}

// target conditions of the effects that hit the explicit target; false when one of them rejects the creature
bool MeetsTargetConditions(SpellInfo const* spellInfo, Player const* player, Creature const* creature, bool& restricted)
{
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (!effect.ImplicitTargetConditions || effect.ImplicitTargetConditions->empty())
            continue;

        if (effect.TargetA.GetReferenceType() != TARGET_REFERENCE_TYPE_TARGET && effect.TargetB.GetReferenceType() != TARGET_REFERENCE_TYPE_TARGET)
            continue;

        restricted = true;
        if (!sConditionMgr->IsObjectMeetToConditions(creature, player, *effect.ImplicitTargetConditions))
            return false;
    }

    return true;
}

// the creature is what an unfinished objective asks for
bool IsObjectiveTarget(Player const* player, std::vector<Quest const*> const& quests, Creature const* creature)
{
    CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
    auto matches = [creatureTemplate](int32 objectId)
    {
        if (uint32(objectId) == creatureTemplate->Entry)
            return true;

        return std::ranges::find(creatureTemplate->KillCredit, uint32(objectId)) != std::end(creatureTemplate->KillCredit);
    };

    for (Quest const* quest : quests)
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == QUEST_OBJECTIVE_MONSTER && matches(objective.ObjectID)
                && !player->IsQuestObjectiveComplete(quest->GetQuestId(), objective.ID))
                return true;

    return false;
}

bool IsUsableOn(Player* player, Item* item, Creature* creature)
{
    if (!item)
        return false;

    ItemTemplate const* proto = item->GetTemplate();
    std::vector<Quest const*> quests;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player->GetQuestSlotQuestId(slot);
        if (!questId || player->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;

        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            if (QuestUsesItem(quest, proto))
                quests.push_back(quest);
    }

    if (quests.empty())
        return false;

    for (ItemEffectEntry const* effect : item->GetEffects())
    {
        if (effect->TriggerType != ITEM_SPELLTRIGGER_ON_USE)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(effect->SpellID, player->GetMap()->GetDifficultyID());
        if (!spellInfo)
            continue;

        if (spellInfo->CheckTarget(player, creature, false) != SPELL_CAST_OK || spellInfo->CheckExplicitTarget(player, creature) != SPELL_CAST_OK)
            continue;

        bool restricted = false;
        if (!MeetsTargetConditions(spellInfo, player, creature, restricted))
            continue;

        std::unique_ptr<Spell> spell = std::make_unique<Spell>(player, spellInfo, TRIGGERED_NONE);
        spell->m_CastItem = item;
        SpellCastTargets targets;
        targets.SetUnitTarget(creature);
        bool scriptChecksTarget = false;
        if (!IsTransientCastError(spell->CheckCastWithoutCasting(targets, scriptChecksTarget)))
            continue;

        // a spell that checks its target itself (conditions or script) decides alone; otherwise the target must be
        // what the quest asks for, so a heal kit does not light up the cursor on every creature
        if (restricted || scriptChecksTarget || IsObjectiveTarget(player, quests, creature))
            return true;
    }

    return false;
}
}

void WorldSession::HandleQueryQuestItemUsability(WorldPackets::QuestItemUsability::QueryQuestItemUsability& packet)
{
    Player* player = GetPlayer();
    Creature* creature = ObjectAccessor::GetCreature(*player, packet.CreatureGUID);

    WorldPackets::QuestItemUsability::QuestItemUsabilityResponse response;
    response.CreatureGUID = packet.CreatureGUID;
    for (ObjectGuid const& itemGuid : packet.ItemGUIDs)
    {
        response.ItemGUIDs.push_back(itemGuid);
        response.IsUsable.push_back(creature && IsUsableOn(player, player->GetItemByGuid(itemGuid), creature));
    }

    SendPacket(response.Write());
}
