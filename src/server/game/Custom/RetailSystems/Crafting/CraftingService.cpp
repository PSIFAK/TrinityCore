/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CraftingService.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "Item.h"
#include "ItemBonusMgr.h"
#include "ItemEnchantmentMgr.h"
#include "ItemPackets.h"
#include "Log.h"
#include "MapUtils.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RetailDB2Stores.h"
#include "Spell.h"
#include "SpellDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

namespace RetailSystems::Crafting
{
namespace
{
using namespace RetailSystems::DB2;

enum ModifiedCraftingReagentType : int32
{
    MCR_REAGENT_TYPE_OPTIONAL   = 0,
    MCR_REAGENT_TYPE_REQUIRED   = 1,
    MCR_REAGENT_TYPE_FINISHING  = 2,
    MCR_REAGENT_TYPE_SOCKET     = 3
};

struct CraftingIndexes
{
    CraftingIndexes()
    {
        for (ModifiedCraftingSpellSlotEntry const* spellSlot : ModifiedCraftingSpellSlotStore)
            SpellSlotsBySpell[spellSlot->SpellID].push_back(spellSlot);

        for (MCRSlotXMCRCategoryEntry const* slotCategory : MCRSlotXMCRCategoryStore)
            CategoriesByReagentSlot[slotCategory->ModifiedCraftingReagentSlotID].push_back(slotCategory->ModifiedCraftingCategoryID);

        for (CraftingReagentQualityEntry const* reagentQuality : CraftingReagentQualityStore)
        {
            ReagentQualityByItem[reagentQuality->ItemID] = reagentQuality;
            int32& maxOrder = MaxReagentOrderByCategory[reagentQuality->ModifiedCraftingCategoryID];
            maxOrder = std::max(maxOrder, reagentQuality->OrderIndex);
        }

        for (CraftingDataItemQualityEntry const* dataItem : CraftingDataItemQualityStore)
            ItemsByCraftingData[dataItem->CraftingDataID].push_back(dataItem);

        for (auto& [_, items] : ItemsByCraftingData)
            std::ranges::sort(items, {}, &CraftingDataItemQualityEntry::ID);

        for (CraftingDifficultyQualityEntry const* difficultyQuality : CraftingDifficultyQualityStore)
            QualitiesByDifficulty[difficultyQuality->CraftingDifficultyID].push_back(difficultyQuality);

        for (auto& [_, qualities] : QualitiesByDifficulty)
            std::ranges::sort(qualities, {}, &CraftingDifficultyQualityEntry::Order);
    }

    std::unordered_map<int32, std::vector<ModifiedCraftingSpellSlotEntry const*>> SpellSlotsBySpell;
    std::unordered_map<int32, std::vector<int32>> CategoriesByReagentSlot;
    std::unordered_map<int32, CraftingReagentQualityEntry const*> ReagentQualityByItem;
    std::unordered_map<int32, int32> MaxReagentOrderByCategory;
    std::unordered_map<int32, std::vector<CraftingDataItemQualityEntry const*>> ItemsByCraftingData;
    std::unordered_map<int32, std::vector<CraftingDifficultyQualityEntry const*>> QualitiesByDifficulty;
};

CraftingIndexes const& GetIndexes()
{
    static CraftingIndexes const indexes;
    return indexes;
}

struct QualitySlot
{
    float Weight;
    int32 ReagentCount;
    float QualityQuantity; // sum of quantity * (quality order / max quality order)
};

// Validated modified crafting reagents of a cast
struct ReagentResult
{
    std::vector<uint32> BonusTreeIDs;
    std::unordered_map<uint32 /*itemId*/, uint32 /*count*/> ReagentsToConsume;
    int32 DifficultyAdjustment = 0;
    int32 ItemContextOffset = 0;
    std::vector<QualitySlot> QualitySlots;
    bool HasQualityWeights = false;
};

struct CraftResult
{
    CraftingDataEntry const* Data = nullptr;
    uint32 ItemID = 0;
    CraftingQualityEntry const* Quality = nullptr;
    float QualityProgress = 0.0f;
    int32 Skill = 0;
    int32 SkillFromReagents = 0;
    SkillLineAbilityEntry const* SkillLineAbility = nullptr;
    ReagentResult Reagents;
};

// Category of a reagent item: quality reagents are listed in CraftingReagentQuality,
// optional/finishing reagents are mapped through ModifiedCraftingItem -> ModifiedCraftingReagentItem
int32 GetReagentCategory(int32 itemId, ModifiedCraftingReagentItemEntry const** reagentItem)
{
    *reagentItem = nullptr;

    if (ModifiedCraftingItemEntry const* modifiedCraftingItem = sModifiedCraftingItemStore.LookupEntry(itemId))
    {
        if (ModifiedCraftingReagentItemEntry const* entry = ModifiedCraftingReagentItemStore.LookupEntry(modifiedCraftingItem->ModifiedCraftingReagentItemID))
        {
            *reagentItem = entry;
            return entry->ModifiedCraftingCategoryID;
        }
    }

    if (CraftingReagentQualityEntry const* reagentQuality = Trinity::Containers::MapGetValuePtr(GetIndexes().ReagentQualityByItem, itemId))
        return reagentQuality->ModifiedCraftingCategoryID;

    return 0;
}

SkillLineAbilityEntry const* GetSkillLineAbility(uint32 spellId)
{
    SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
    return bounds.first != bounds.second ? bounds.first->second : nullptr;
}

// Validates the modified crafting reagents sent with a cast against the spell's ModifiedCraftingSpellSlot entries
SpellCastResult ResolveReagents(Player const* player, SpellInfo const* spellInfo, SpellCastTargets const& targets, ReagentResult& result)
{
    CraftingIndexes const& indexes = GetIndexes();

    std::unordered_map<int32, std::vector<SpellCraftingReagentTarget const*>> reagentsBySlot;
    for (SpellCraftingReagentTarget const& reagent : targets.GetCraftingReagents())
    {
        if (reagent.Quantity <= 0)
            return SPELL_FAILED_REAGENTS;

        reagentsBySlot[reagent.Slot].push_back(&reagent);
    }

    std::vector<ModifiedCraftingSpellSlotEntry const*> const* spellSlots = Trinity::Containers::MapGetValuePtr(indexes.SpellSlotsBySpell, spellInfo->Id);

    // every provided reagent must belong to a slot of this spell
    for (auto const& [slot, _] : reagentsBySlot)
        if (!spellSlots || std::ranges::none_of(*spellSlots, [slot](ModifiedCraftingSpellSlotEntry const* spellSlot) { return spellSlot->Slot == slot; }))
            return SPELL_FAILED_REAGENTS;

    if (spellSlots)
    {
        for (ModifiedCraftingSpellSlotEntry const* spellSlot : *spellSlots)
        {
            if (spellSlot->ReagentCount <= 0)
                continue;

            ModifiedCraftingReagentSlotEntry const* reagentSlot = ModifiedCraftingReagentSlotStore.LookupEntry(spellSlot->ModifiedCraftingReagentSlotID);
            if (!reagentSlot)
                continue;

            auto providedItr = reagentsBySlot.find(spellSlot->Slot);
            if (providedItr == reagentsBySlot.end())
            {
                if (reagentSlot->ReagentType == MCR_REAGENT_TYPE_REQUIRED)
                    return SPELL_FAILED_REAGENTS;

                continue;
            }

            std::vector<int32> const* allowedCategories = Trinity::Containers::MapGetValuePtr(indexes.CategoriesByReagentSlot, reagentSlot->ID);
            if (!allowedCategories)
                return SPELL_FAILED_REAGENTS;

            int32 providedCount = 0;
            float qualityQuantity = 0.0f;
            float slotWeight = 0.0f;
            bool isQualitySlot = false;

            for (SpellCraftingReagentTarget const* reagent : providedItr->second)
            {
                ModifiedCraftingReagentItemEntry const* reagentItem = nullptr;
                int32 categoryId = GetReagentCategory(reagent->ItemID, &reagentItem);
                if (!categoryId || !advstd::ranges::contains(*allowedCategories, categoryId))
                    return SPELL_FAILED_REAGENTS;

                providedCount += reagent->Quantity;
                result.ReagentsToConsume[reagent->ItemID] += reagent->Quantity;

                if (reagentItem)
                {
                    if (reagentItem->ItemBonusTreeID)
                        result.BonusTreeIDs.push_back(reagentItem->ItemBonusTreeID);

                    result.ItemContextOffset = std::max(result.ItemContextOffset, reagentItem->ItemContextOffset);
                }

                if (CraftingReagentQualityEntry const* reagentQuality = Trinity::Containers::MapGetValuePtr(indexes.ReagentQualityByItem, reagent->ItemID))
                {
                    result.DifficultyAdjustment += int32(reagentQuality->MaxDifficultyAdjustment);

                    int32 const* maxOrder = Trinity::Containers::MapGetValuePtr(indexes.MaxReagentOrderByCategory, categoryId);
                    if (reagentSlot->ReagentType == MCR_REAGENT_TYPE_REQUIRED && maxOrder && *maxOrder > 0)
                    {
                        isQualitySlot = true;
                        qualityQuantity += float(reagent->Quantity) * float(reagentQuality->OrderIndex) / float(*maxOrder);
                        if (ModifiedCraftingCategoryEntry const* category = ModifiedCraftingCategoryStore.LookupEntry(categoryId))
                            slotWeight = std::max(slotWeight, category->MatQualityWeight);
                    }
                }
            }

            if (providedCount != spellSlot->ReagentCount)
                return SPELL_FAILED_REAGENTS;

            if (isQualitySlot)
            {
                result.QualitySlots.push_back({ slotWeight, spellSlot->ReagentCount, qualityQuantity });
                result.HasQualityWeights = result.HasQualityWeights || slotWeight > 0.0f;
            }
        }
    }

    for (auto const& [itemId, count] : result.ReagentsToConsume)
        if (!player->HasItemCount(itemId, count))
            return SPELL_FAILED_REAGENTS;

    return SPELL_CAST_OK;
}

SpellCastResult ResolveCraft(Player const* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets, CraftResult& result)
{
    CraftingIndexes const& indexes = GetIndexes();

    result.Data = CraftingDataStore.LookupEntry(effectInfo.MiscValue);
    if (!result.Data)
    {
        TC_LOG_ERROR("spells", "RetailSystems::Crafting: spell {} references missing CraftingData {}", spellInfo->Id, effectInfo.MiscValue);
        return SPELL_FAILED_ERROR;
    }

    SpellCastResult reagentResult = ResolveReagents(player, spellInfo, targets, result.Reagents);
    if (reagentResult != SPELL_CAST_OK)
        return reagentResult;

    if (result.Data->ItemBonusTreeID)
        result.Reagents.BonusTreeIDs.insert(result.Reagents.BonusTreeIDs.begin(), result.Data->ItemBonusTreeID);

    int32 difficulty = result.Data->CraftingDifficulty + result.Reagents.DifficultyAdjustment;
    float totalQualityWeight = 0.0f;
    float achievedQualityWeight = 0.0f;

    // categories without MatQualityWeight contribute equally
    for (QualitySlot const& qualitySlot : result.Reagents.QualitySlots)
    {
        float weight = result.Reagents.HasQualityWeights ? qualitySlot.Weight : 1.0f;
        totalQualityWeight += weight * float(qualitySlot.ReagentCount);
        achievedQualityWeight += weight * qualitySlot.QualityQuantity;
    }

    result.SkillLineAbility = GetSkillLineAbility(spellInfo->Id);
    if (result.SkillLineAbility)
        result.Skill = player->GetSkillValue(result.SkillLineAbility->SkillupSkillLineID ? result.SkillLineAbility->SkillupSkillLineID : result.SkillLineAbility->SkillLine);

    CraftingDifficultyEntry const* craftingDifficulty = CraftingDifficultyStore.LookupEntry(result.Data->CraftingDifficultyID);
    if (craftingDifficulty && totalQualityWeight > 0.0f)
    {
        float maxReagentBonus = float(result.Data->CraftingDifficulty) * craftingDifficulty->CraftSkillBonusPercent / 100.0f;
        result.SkillFromReagents = int32(std::round(maxReagentBonus * achievedQualityWeight / totalQualityWeight));
    }

    int32 totalSkill = result.Skill + result.SkillFromReagents;

    // quality tier: highest CraftingDifficultyQuality reached by skill / difficulty
    int32 qualityIndex = -1;
    if (std::vector<CraftingDifficultyQualityEntry const*> const* qualities = Trinity::Containers::MapGetValuePtr(indexes.QualitiesByDifficulty, result.Data->CraftingDifficultyID))
    {
        float percent = difficulty > 0 ? float(totalSkill) * 100.0f / float(difficulty) : 100.0f;
        for (std::size_t i = 0; i < qualities->size(); ++i)
        {
            if ((*qualities)[i]->QualityPercentage > percent)
                break;

            qualityIndex = int32(i);
        }

        if (qualityIndex >= 0)
        {
            result.Quality = sCraftingQualityStore.LookupEntry((*qualities)[qualityIndex]->CraftingQualityID);
            if (std::size_t(qualityIndex + 1) < qualities->size())
            {
                float low = (*qualities)[qualityIndex]->QualityPercentage;
                float high = (*qualities)[qualityIndex + 1]->QualityPercentage;
                result.QualityProgress = high > low ? std::clamp((percent - low) / (high - low), 0.0f, 1.0f) : 0.0f;
            }
            else
                result.QualityProgress = 1.0f;
        }
    }

    if (result.Data->CraftedItemID)
        result.ItemID = result.Data->CraftedItemID;
    else if (std::vector<CraftingDataItemQualityEntry const*> const* items = Trinity::Containers::MapGetValuePtr(indexes.ItemsByCraftingData, result.Data->ID))
        result.ItemID = (*items)[std::clamp<std::size_t>(std::max(qualityIndex, 0), 0, items->size() - 1)]->ItemID;

    if (!result.ItemID || !sObjectMgr->GetItemTemplate(result.ItemID))
    {
        TC_LOG_ERROR("spells", "RetailSystems::Crafting: spell {} CraftingData {} has no craftable item", spellInfo->Id, result.Data->ID);
        return SPELL_FAILED_ERROR;
    }

    return SPELL_CAST_OK;
}

void InitModifiedCraftingStats(Item* item)
{
    // TODO: crafter-selected stats (missives) are sent as finishing reagents, use random secondary stats until they are mapped
    std::array<uint32, 4> allowedStats = { ITEM_MOD_MASTERY_RATING, ITEM_MOD_HASTE_RATING, ITEM_MOD_VERSATILITY, ITEM_MOD_CRIT_RATING };
    Trinity::Containers::RandomShuffle(allowedStats);

    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        if (item->GetItemStatType(i) == ITEM_MOD_MODIFIED_CRAFTING_STAT_1)
            item->SetModifier(ITEM_MODIFIER_CHANGE_MODIFIED_CRAFTING_STAT_1, allowedStats[0]);
        else if (item->GetItemStatType(i) == ITEM_MOD_MODIFIED_CRAFTING_STAT_2)
            item->SetModifier(ITEM_MODIFIER_CHANGE_MODIFIED_CRAFTING_STAT_2, allowedStats[1]);
    }
}

void SendCraftedItem(Player* player, Item* item, uint32 quantity, CraftResult const& result)
{
    WorldPackets::Item::ItemPushResult packet;
    packet.PlayerGUID = player->GetGUID();
    packet.Slot = item->GetBagSlot();
    packet.SlotInBag = item->GetCount() == quantity ? item->GetSlot() : -1;
    packet.Item.Initialize(item);
    packet.Quantity = quantity;
    packet.QuantityInInventory = player->GetItemCount(item->GetEntry());
    packet.ItemGUID = item->GetGUID();
    packet.Pushed = true;
    packet.Created = true;
    packet.ChatNotifyType = WorldPackets::Item::ItemPushResult::DISPLAY_TYPE_NORMAL;

    WorldPackets::Crafting::CraftingData& craftingData = packet.CraftingData.emplace();
    craftingData.CraftingQualityID = result.Quality ? result.Quality->ID : 0;
    craftingData.QualityProgress = result.QualityProgress;
    craftingData.SkillLineAbilityID = result.SkillLineAbility ? result.SkillLineAbility->ID : 0;
    craftingData.CraftingDataID = result.Data->ID;
    craftingData.SkillFromReagents = result.SkillFromReagents;
    craftingData.Skill = result.Skill;
    craftingData.ItemGUID = item->GetGUID();
    craftingData.Quantity = quantity;
    craftingData.NewItem.Initialize(item);

    player->SendDirectMessage(packet.Write());
}
}

SpellCastResult CheckCraft(Player const* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets)
{
    CraftResult result;
    return ResolveCraft(player, spellInfo, effectInfo, targets, result);
}

SpellCastResult Craft(Player* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets)
{
    CraftResult result;
    SpellCastResult castResult = ResolveCraft(player, spellInfo, effectInfo, targets, result);
    if (castResult != SPELL_CAST_OK)
        return castResult;

    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(result.ItemID);
    uint32 quantity = std::clamp<uint32>(uint32(std::max(effectInfo.BasePoints, 1.0f)), 1, itemTemplate->GetMaxStackSize());

    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, result.ItemID, quantity);
    if (msg != EQUIP_ERR_OK)
    {
        player->SendEquipError(msg, nullptr, nullptr, result.ItemID);
        return SPELL_FAILED_DONT_REPORT;
    }

    ItemContext context = spellInfo->HasAttribute(SPELL_ATTR0_IS_TRADESKILL) ? ItemContext::Trade_Skill : ItemContext::NONE;

    ItemBonusMgr::ItemBonusGenerationParams bonusParams(context);
    bonusParams.ExtraBonusTreeIDs = result.Reagents.BonusTreeIDs;
    bonusParams.SequenceLevel = result.Quality ? result.Quality->QualityTier : 0;
    std::vector<int32> bonusListIDs = ItemBonusMgr::GetBonusListsForItem(result.ItemID, bonusParams);

    Item* item = player->StoreNewItem(dest, result.ItemID, true, GenerateItemRandomBonusListId(result.ItemID), GuidSet(), context, &bonusListIDs);
    if (!item)
        return SPELL_FAILED_ERROR;

    if (result.Quality && result.Data->CraftedItemID)
        item->SetModifier(ITEM_MODIFIER_CRAFTING_QUALITY_ID, result.Quality->ID);

    InitModifiedCraftingStats(item);

    if (itemTemplate->HasSignature())
        item->SetCreator(player->GetGUID());

    for (auto const& [itemId, count] : result.Reagents.ReagentsToConsume)
        player->DestroyItemCount(itemId, count, true);

    SendCraftedItem(player, item, quantity, result);
    player->UpdateCraftSkill(spellInfo);
    return SPELL_CAST_OK;
}

SpellCastResult CheckCreateItemReagents(Player const* player, SpellInfo const* spellInfo, SpellCastTargets const& targets)
{
    if (targets.GetCraftingReagents().empty())
        return SPELL_CAST_OK;

    ReagentResult result;
    return ResolveReagents(player, spellInfo, targets, result);
}

SpellCastResult CreateItemWithReagents(Spell* spell, Player* player, SpellInfo const* spellInfo, SpellEffectInfo const& effectInfo, SpellCastTargets const& targets)
{
    ReagentResult result;
    SpellCastResult castResult = ResolveReagents(player, spellInfo, targets, result);
    if (castResult != SPELL_CAST_OK)
        return castResult;

    int32 context = effectInfo.MiscValue;
    if (!context && spellInfo->HasAttribute(SPELL_ATTR0_IS_TRADESKILL))
        context = int32(ItemContext::Trade_Skill);

    if (context && result.ItemContextOffset)
        context += result.ItemContextOffset;

    ItemBonusMgr::ItemBonusGenerationParams bonusParams{ ItemContext(context) };
    bonusParams.ExtraBonusTreeIDs = result.BonusTreeIDs;
    std::vector<int32> bonusListIDs = ItemBonusMgr::GetBonusListsForItem(effectInfo.ItemType, bonusParams);

    if (!spell->DoCreateItem(effectInfo.ItemType, ItemContext(context), &bonusListIDs))
        return SPELL_FAILED_DONT_REPORT;

    for (auto const& [itemId, count] : result.ReagentsToConsume)
        player->DestroyItemCount(itemId, count, true);

    return SPELL_CAST_OK;
}
}
