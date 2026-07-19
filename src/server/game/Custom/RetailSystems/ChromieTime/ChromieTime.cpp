/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChromieTime.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "RetailDB2Stores.h"
#include "Spell.h"

namespace RetailSystems::ChromieTime
{
namespace
{
void Persist(uint64 characterGuid, uint32 expansionId)
{
    if (expansionId)
    {
        CharacterDatabase.PExecute(
            "INSERT INTO `retail_character_chromie_time` (`guid`, `expansion_id`) VALUES ({}, {}) "
            "ON DUPLICATE KEY UPDATE `expansion_id` = VALUES(`expansion_id`)",
            characterGuid, expansionId);
    }
    else
        CharacterDatabase.PExecute("DELETE FROM `retail_character_chromie_time` WHERE `guid` = {}", characterGuid);
}
}

bool Apply(Player* player, uint32 expansionId, bool persist)
{
    if (!player)
        return false;

    DB2::UIChromieTimeExpansionInfoEntry const* expansion = nullptr;
    if (expansionId)
    {
        expansion = DB2::UIChromieTimeExpansionInfoStore.LookupEntry(expansionId);
        if (!expansion)
        {
            TC_LOG_WARN("entities.player", "Player {} tried to select unknown Chromie Time expansion {}",
                player->GetGUID().ToString(), expansionId);
            return false;
        }
    }

    bool const changed = player->GetChromieTimeExpansionID() != expansionId;
    player->SetChromieTimeExpansion(expansionId, expansion ? uint32(expansion->ExpansionLevelMask) : 0);

    if (persist && changed)
        Persist(player->GetGUID().GetCounter(), expansionId);

    return true;
}

bool Select(Player* player, uint32 expansionId)
{
    if (!Apply(player, expansionId, true))
        return false;

    if (expansionId)
    {
        DB2::UIChromieTimeExpansionInfoEntry const* expansion = DB2::UIChromieTimeExpansionInfoStore.LookupEntry(expansionId);
        if (expansion && expansion->SpellID > 0)
            player->CastSpell(player, uint32(expansion->SpellID), true);
    }

    return true;
}

void Restore(Player* player)
{
    uint32 expansionId = 0;
    if (QueryResult result = CharacterDatabase.PQuery(
        "SELECT `expansion_id` FROM `retail_character_chromie_time` WHERE `guid` = {}", player->GetGUID().GetCounter()))
    {
        expansionId = result->Fetch()[0].GetUInt32();
    }

    if (!Apply(player, expansionId, false))
    {
        Apply(player, 0, false);
        RemoveCharacterState(player->GetGUID().GetCounter());
    }
}

void RemoveCharacterState(uint64 characterGuid)
{
    CharacterDatabase.PExecute("DELETE FROM `retail_character_chromie_time` WHERE `guid` = {}", characterGuid);
}
}

void Player::SetChromieTimeExpansion(uint32 expansionId, uint32 expansionLevelMask)
{
    SetUpdateFieldValue(m_values.ModifyValue(&Player::m_activePlayerData).ModifyValue(&UF::ActivePlayerData::UiChromieTimeExpansionID), int32(expansionId));

    auto options = m_values.ModifyValue(&Player::m_playerData).ModifyValue(&UF::PlayerData::CtrOptions);
    SetUpdateFieldValue(options.ModifyValue(&UF::CTROptions::ConditionalFlags), expansionId ? std::vector<uint32>{ 1u } : std::vector<uint32>{});
    SetUpdateFieldValue(options.ModifyValue(&UF::CTROptions::FactionGroup), uint8(GetTeam() == ALLIANCE ? 3 : 5));
    SetUpdateFieldValue(options.ModifyValue(&UF::CTROptions::ChromieTimeExpansionMask), expansionLevelMask);
}

void Spell::EffectSetChromieTime()
{
    if (effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET || effectInfo->MiscValue < 0)
        return;

    Player* player = unitTarget ? unitTarget->ToPlayer() : nullptr;
    if (!player)
        player = m_caster->ToPlayer();
    if (player)
        RetailSystems::ChromieTime::Apply(player, uint32(effectInfo->MiscValue), true);
}
