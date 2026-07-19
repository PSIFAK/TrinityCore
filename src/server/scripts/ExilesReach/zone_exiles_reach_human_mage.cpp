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

#include "DB2Stores.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "SceneMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "WorldSession.h"

namespace ExilesReachHumanMage
{
enum Quests
{
    QUEST_WHO_LURKS_IN_THE_PIT       = 55639,
    QUEST_GEARED_FOR_REPAIR          = 85678,
    QUEST_LIGHTS_CAMERA_ACTION       = 54933,
    QUEST_MAGE_KNOWLEDGE             = 59352,
    QUEST_THE_ART_OF_TAMING_SHEEP    = 59354,
    QUEST_THE_ENEMY_OF_MY_ENEMY      = 55981,
    QUEST_DUNGEON_DARKMAUL_CITADEL   = 55992,
    QUEST_THE_END_OF_THE_BEGINNING   = 55991,
    QUEST_WHATS_YOUR_SPECIALTY       = 87547,
    QUEST_HOME_IS_WHERE_THE_HEARTH_IS = 87555
};

enum Creatures
{
    NPC_QUARTERMASTER_RICHTER        = 156800,
    NPC_RALIA_DREAMCHASER            = 156929,
    NPC_MEREDY_HUNTSWELL             = 156886,
    NPC_MEREDY_TRANSFORMATION        = 156943,
    NPC_KALECGOS_EXILES_REACH        = 244389,
    NPC_HAPPY_HAL                    = 187412,
    NPC_POLYMORPH_TARGET_COLE        = 168372,
    NPC_POLYMORPH_TARGET_MEREDY      = 168373,
    NPC_POLYMORPH_TRAINING_CREDIT    = 164974,
    NPC_SPECIALIZATION_CREDIT        = 163033,
    NPC_HAPPY_HAL_CREDIT             = 163160,
    NPC_MOUNT_CREDIT                 = 239009
};

enum QuestObjectives
{
    OBJECTIVE_TALK_TO_MEREDY         = 396406,
    OBJECTIVE_OGRE_TRANSFORMATION    = 390131,
    OBJECTIVE_USE_DUNGEON_FINDER     = 394461
};

enum Spells
{
    SPELL_RALIA_INTERACT             = 312463,
    SPELL_MEREDY_POLYMORPH_INTRO     = 321134,
    SPELL_MEREDY_POLYMORPH_OUTRO     = 321139,
    SPELL_POLYMORPH                  = 118,
    SPELL_OGRE_TRANSFORMATION        = 313583,
    SPELL_LIGHTSPAWN_FREED           = 319171,
    SPELL_DRAGON_ISLES_TELEPORT      = 1235041,
    SPELL_HAPPY_HAL_CONVERSATION     = 375316
};

enum Scenes
{
    SCENE_RALIA_RESCUE               = 2379,
    SCENE_LIGHTSPAWN_FREED           = 2402
};

enum GossipMenus
{
    GOSSIP_MENU_MEREDY_TRAINING      = 25321,
    GOSSIP_MENU_OGRE_TRANSFORMATION  = 24550,
    GOSSIP_MENU_KALECGOS_TRAVEL      = 39219,
    GOSSIP_MENU_HAPPY_HAL            = 28240
};

enum Items
{
    ITEM_HEARTHSTONE                 = 6948
};

Position const PolymorphColePosition = { 182.99826f, -2286.684f, 81.84842f, 0.0f };
Position const PolymorphMeredyPosition = { 187.06424f, -2279.7275f, 82.01448f, 1.8322159f };

void FinishRaliaRescue(Player* player)
{
    if (player->GetQuestStatus(QUEST_WHO_LURKS_IN_THE_PIT) != QUEST_STATUS_INCOMPLETE)
        return;

    player->KilledMonsterCredit(NPC_RALIA_DREAMCHASER);
    PhasingHandler::OnConditionChange(player);
}

void GiveHearthstone(Player* player)
{
    if (player->HasItemCount(ITEM_HEARTHSTONE, 1, true))
        return;

    ItemPosCountVec destination;
    if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destination, ITEM_HEARTHSTONE, 1) != EQUIP_ERR_OK)
        return;

    if (Item* item = player->StoreNewItem(destination, ITEM_HEARTHSTONE, true))
        player->SendNewItem(item, 1, true, false);
}
}

using namespace ExilesReachHumanMage;

// 156929 - Ralia Dreamchaser
struct npc_exiles_reach_ralia_dreamchaser : public ScriptedAI
{
    npc_exiles_reach_ralia_dreamchaser(Creature* creature) : ScriptedAI(creature) { }

    void OnSpellClick(Unit* clicker, bool spellClickHandled) override
    {
        if (!spellClickHandled)
            return;

        Player* player = clicker->ToPlayer();
        if (!player || player->GetQuestStatus(QUEST_WHO_LURKS_IN_THE_PIT) != QUEST_STATUS_INCOMPLETE)
            return;

        player->GetSceneMgr().PlayScene(SCENE_RALIA_RESCUE);
    }
};

// 2379 - Ralia rescue
class scene_exiles_reach_ralia_rescue : public SceneScript
{
public:
    scene_exiles_reach_ralia_rescue() : SceneScript("scene_exiles_reach_ralia_rescue") { }

    void OnSceneComplete(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        FinishRaliaRescue(player);
    }

    void OnSceneCancel(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        FinishRaliaRescue(player);
    }
};

// 54933 - Light, Camera, Action!
class q54933_lights_camera_action : public QuestScript
{
public:
    q54933_lights_camera_action() : QuestScript("q54933_lights_camera_action") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_COMPLETE)
            return;

        player->GetSceneMgr().PlayScene(SCENE_LIGHTSPAWN_FREED);
        if (Creature* lightspawn = player->FindNearestCreature(157114, 100.0f, true))
            lightspawn->CastSpell(player, SPELL_LIGHTSPAWN_FREED, true);

        PhasingHandler::OnConditionChange(player);
    }
};

// 156800 - Quartermaster Richter
struct npc_exiles_reach_quartermaster_richter : public ScriptedAI
{
    npc_exiles_reach_quartermaster_richter(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        if (player->GetQuestStatus(QUEST_GEARED_FOR_REPAIR) == QUEST_STATUS_INCOMPLETE)
        {
            player->DurabilityRepairAll(false, 0.0f, false);
            player->KilledMonsterCredit(NPC_QUARTERMASTER_RICHTER, me->GetGUID());
        }

        return false;
    }
};

// 156886 - Meredy Huntswell
struct npc_exiles_reach_meredy_polymorph_training : public ScriptedAI
{
    npc_exiles_reach_meredy_polymorph_training(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_MEREDY_TRAINING || gossipListId != 0 ||
            player->GetQuestStatus(QUEST_THE_ART_OF_TAMING_SHEEP) != QUEST_STATUS_INCOMPLETE ||
            player->IsQuestObjectiveComplete(QUEST_THE_ART_OF_TAMING_SHEEP, OBJECTIVE_TALK_TO_MEREDY))
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        player->KilledMonsterCredit(NPC_MEREDY_HUNTSWELL, me->GetGUID());
        me->CastSpell(player, SPELL_MEREDY_POLYMORPH_INTRO, true);

        if (TempSummon* cole = me->SummonCreature(NPC_POLYMORPH_TARGET_COLE, PolymorphColePosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 2min, 0, 0, player->GetGUID()))
        {
            cole->SetFaction(16);
            cole->SetReactState(REACT_PASSIVE);
        }

        if (TempSummon* meredy = me->SummonCreature(NPC_POLYMORPH_TARGET_MEREDY, PolymorphMeredyPosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 2min, 0, 0, player->GetGUID()))
        {
            meredy->SetFaction(16);
            meredy->SetReactState(REACT_PASSIVE);
        }

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(5s, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                me->CastSpell(target, SPELL_MEREDY_POLYMORPH_OUTRO, true);
        });

        return true;
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 118 - Polymorph
class spell_exiles_reach_polymorph_training : public SpellScript
{
    void HandleAfterHit()
    {
        Player* player = GetCaster()->ToPlayer();
        Creature* target = GetHitCreature();
        if (!player || !target || player->GetQuestStatus(QUEST_THE_ART_OF_TAMING_SHEEP) != QUEST_STATUS_INCOMPLETE)
            return;

        if (target->GetEntry() != NPC_POLYMORPH_TARGET_COLE && target->GetEntry() != NPC_POLYMORPH_TARGET_MEREDY)
            return;

        player->KilledMonsterCredit(NPC_POLYMORPH_TRAINING_CREDIT);
        target->DespawnOrUnsummon(2s);
    }

    void Register() override
    {
        AfterHit += SpellHitFn(spell_exiles_reach_polymorph_training::HandleAfterHit);
    }
};

// 156943 - Meredy Huntswell
struct npc_exiles_reach_meredy_ogre_transformation : public ScriptedAI
{
    npc_exiles_reach_meredy_ogre_transformation(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_OGRE_TRANSFORMATION || gossipListId != 0 ||
            player->GetQuestStatus(QUEST_THE_ENEMY_OF_MY_ENEMY) != QUEST_STATUS_INCOMPLETE ||
            player->IsQuestObjectiveComplete(QUEST_THE_ENEMY_OF_MY_ENEMY, OBJECTIVE_OGRE_TRANSFORMATION))
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        player->KilledMonsterCredit(NPC_MEREDY_TRANSFORMATION, me->GetGUID());
        me->CastSpell(player, SPELL_OGRE_TRANSFORMATION, true);
        PhasingHandler::OnConditionChange(player);
        return true;
    }
};

// 244389 - Kalecgos
struct npc_exiles_reach_kalecgos_travel : public ScriptedAI
{
    npc_exiles_reach_kalecgos_travel(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_KALECGOS_TRAVEL || gossipListId != 0 ||
            player->GetQuestStatus(QUEST_THE_END_OF_THE_BEGINNING) != QUEST_STATUS_INCOMPLETE)
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        player->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
        me->CastSpell(player, SPELL_DRAGON_ISLES_TELEPORT, true);
        PhasingHandler::OnConditionChange(player);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(15s, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                target->TeleportTo(2444, 3713.47f, -1890.45f, 5.77f, 2.7612855f, TELE_TO_NONE, {}, SPELL_DRAGON_ISLES_TELEPORT);
        });

        return true;
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 187412 - Happy Hal
struct npc_exiles_reach_happy_hal : public ScriptedAI
{
    npc_exiles_reach_happy_hal(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        if (player->GetQuestStatus(QUEST_HOME_IS_WHERE_THE_HEARTH_IS) == QUEST_STATUS_INCOMPLETE)
            player->KilledMonsterCredit(NPC_HAPPY_HAL_CREDIT, me->GetGUID());

        return false;
    }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_HAPPY_HAL)
            return false;

        if (gossipListId == 0)
        {
            player->PlayerTalkClass->SendCloseGossip();
            player->CastSpell(player, SPELL_HAPPY_HAL_CONVERSATION, true);
            return true;
        }

        if (gossipListId == 2)
        {
            GiveHearthstone(player);
            player->GetSession()->SendBindPoint(me);
            return true;
        }

        return false;
    }
};

class player_exiles_reach_human_mage : public PlayerScript
{
public:
    player_exiles_reach_human_mage() : PlayerScript("player_exiles_reach_human_mage") { }

    void OnMapChanged(Player* player) override
    {
        if (player->GetMapId() == 2236 &&
            player->GetQuestStatus(QUEST_DUNGEON_DARKMAUL_CITADEL) == QUEST_STATUS_INCOMPLETE &&
            !player->IsQuestObjectiveComplete(QUEST_DUNGEON_DARKMAUL_CITADEL, OBJECTIVE_USE_DUNGEON_FINDER))
        {
            player->UpdateQuestObjectiveProgress(QUEST_OBJECTIVE_CRITERIA_TREE, 83801, 1);
        }
    }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (!spellInfo)
            return;

        if (player->GetQuestStatus(QUEST_WHATS_YOUR_SPECIALTY) == QUEST_STATUS_INCOMPLETE &&
            spellInfo->HasEffect(SPELL_EFFECT_TALENT_SPEC_SELECT))
        {
            player->KilledMonsterCredit(NPC_SPECIALIZATION_CREDIT);
            PhasingHandler::OnConditionChange(player);
        }

        if (player->GetQuestStatus(QUEST_HOME_IS_WHERE_THE_HEARTH_IS) == QUEST_STATUS_INCOMPLETE &&
            sDB2Manager.GetMount(spellInfo->Id))
        {
            player->KilledMonsterCredit(NPC_MOUNT_CREDIT);
        }
    }
};

void AddSC_zone_exiles_reach_human_mage()
{
    RegisterCreatureAI(npc_exiles_reach_ralia_dreamchaser);
    new scene_exiles_reach_ralia_rescue();
    new q54933_lights_camera_action();
    RegisterCreatureAI(npc_exiles_reach_quartermaster_richter);
    RegisterCreatureAI(npc_exiles_reach_meredy_polymorph_training);
    RegisterSpellScript(spell_exiles_reach_polymorph_training);
    RegisterCreatureAI(npc_exiles_reach_meredy_ogre_transformation);
    RegisterCreatureAI(npc_exiles_reach_kalecgos_travel);
    RegisterCreatureAI(npc_exiles_reach_happy_hal);
    new player_exiles_reach_human_mage();
}
