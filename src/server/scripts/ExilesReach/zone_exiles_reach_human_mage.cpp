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

#include "Conversation.h"
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
    QUEST_HOME_IS_WHERE_THE_HEARTH_IS = 87555,
    QUEST_WARMING_UP                 = 56775,
    QUEST_BRACE_FOR_IMPACT           = 58208,
    QUEST_EMERGENCY_FIRST_AID        = 54951,
    QUEST_HARPY_CULLING              = 55764
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
    NPC_MOUNT_CREDIT                 = 239009,
    NPC_LIGHTSPAWN                   = 157114,
    NPC_JAINA_SHIP                   = 156280,
    NPC_PRIVATE_COLE_SHIP            = 160664,
    NPC_JAINA_BEACH                  = 156626,
    NPC_RALIA_PIT_EXIT               = 156932,
    NPC_BJORN_PIT                    = 154300,
    NPC_ALARIA                       = 156803,
    NPC_HENRY_GARRICK_HARPIES        = 156859,
    NPC_KEE_LA_HARPIES               = 156860,
    NPC_HENRY_GARRICK_PRISONER       = 153565,
    NPC_CAPTAIN_GARRICK_PRISONER     = 153566,
    NPC_JAINA_PRISONER               = 245399,
    NPC_GORGROTH_CAMP                = 153580,
    NPC_GRUNK_CAMP                   = 154145
};

enum QuestObjectives
{
    OBJECTIVE_TALK_TO_MEREDY         = 396406,
    OBJECTIVE_PRACTICE_POLYMORPH     = 396405,
    OBJECTIVE_OGRE_TRANSFORMATION    = 390131,
    OBJECTIVE_USE_DUNGEON_FINDER     = 394461,
    OBJECTIVE_FIRST_NECROTIC_ENERGY  = 390022,
    OBJECTIVE_SECOND_NECROTIC_ENERGY = 390023,
    OBJECTIVE_THIRD_NECROTIC_ENERGY  = 390024,
    OBJECTIVE_FOURTH_NECROTIC_ENERGY = 390025,
    OBJECTIVE_RESCUE_EXPEDITION      = 391939,
    OBJECTIVE_WAVE_TO_GORGROTH       = 389971,
    OBJECTIVE_DANCE_IN_COOKING_AREA  = 389972,
    OBJECTIVE_REACH_CITADEL_ENTRANCE = 389973,
    OBJECTIVE_FIND_COOKING_AREA      = 390205
};

// conversations seen in the retail sniff (12.0.1.66709, Alliance human mage)
enum Conversations
{
    CONVERSATION_WARMING_UP_REWARDED  = 30343,
    CONVERSATION_BRACE_FOR_IMPACT     = 29823,
    CONVERSATION_EMERGENCY_FIRST_AID  = 30773,
    CONVERSATION_EXPEDITION_RESCUED   = 12120,
    CONVERSATION_RALIA_RESCUED        = 12124,
    CONVERSATION_LIGHTSPAWN_ACCEPT    = 12171,
    CONVERSATION_LIGHTSPAWN_FIRST     = 12167,
    CONVERSATION_LIGHTSPAWN_SECOND    = 12168,
    CONVERSATION_LIGHTSPAWN_THIRD     = 12169,
    CONVERSATION_HARPY_CULLING_ACCEPT = 12115,
    CONVERSATION_MAGE_KNOWLEDGE       = 13685,
    CONVERSATION_MAGE_KNOWLEDGE_DONE  = 13693,
    CONVERSATION_SHEEP_TRAINING_START = 13695,
    CONVERSATION_SHEEP_FIRST_POLYMORPH = 13697,
    CONVERSATION_SHEEP_TRAINING_DONE  = 13698,
    CONVERSATION_END_OF_THE_BEGINNING = 12851,
    CONVERSATION_PRISONERS_DISGUISED  = 11839,
    CONVERSATION_CITADEL_ENTRANCE     = 11511,
    CONVERSATION_COOKING_AREA         = 11408,
    CONVERSATION_WAVE_TO_GORGROTH     = 11512,
    CONVERSATION_DANCE_FOR_THE_OGRES  = 11513,
    CONVERSATION_PRISONERS_FREED      = 11414
};

// ConversationActorId values sent by retail for the actors a script has to provide
enum ConversationActors
{
    ACTOR_JAINA_SHIP                 = 109370,
    ACTOR_PRIVATE_COLE_SHIP          = 73556,
    ACTOR_JAINA_BEACH                = 71295,
    ACTOR_RALIA_PIT_EXIT             = 73029,
    ACTOR_BJORN_PIT                  = 71505,
    ACTOR_ALARIA                     = 71504,
    ACTOR_HENRY_GARRICK_HARPIES      = 71478,
    ACTOR_KEE_LA_HARPIES             = 71479,
    ACTOR_MEREDY_MAGE_KNOWLEDGE      = 74940,
    ACTOR_POLYMORPH_TARGET_COLE      = 74943,
    ACTOR_POLYMORPH_TARGET_MEREDY    = 74944,
    ACTOR_HENRY_GARRICK_PRISONER     = 70058,
    ACTOR_CAPTAIN_GARRICK_PRISONER   = 70059,
    ACTOR_GORGROTH_CAMP              = 70041,
    ACTOR_GRUNK_CAMP                 = 70294
};

enum Texts
{
    SAY_MEREDY_POLYMORPH             = 0,
    SAY_MEREDY_OGRE_TRANSFORMATION   = 0,
    SAY_LIGHTSPAWN_WEAKENING         = 0,
    SAY_LIGHTSPAWN_FREED             = 1
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
    SPELL_HAPPY_HAL_CONVERSATION     = 375316,
    SPELL_PIT_CONVERSATION           = 306109,
    SPELL_RALIA_RESCUE_TELEPORT      = 312548,
    SPELL_OGRE_DISGUISE              = 298241,
    SPELL_SUMMON_PRISONERS           = 298232,   // linked summons of Captain Garrick, Henry Garrick and Jaina Proudmoore
    SPELL_PRISONER_CHAINED           = 299155,
    SPELL_PING_FOLLOWERS_TO_FINISH   = 298359,
    SPELL_CANCEL_CHAINED             = 322699
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

struct ConversationActorRef
{
    int32 ActorId;
    uint32 Index;
    uint32 CreatureEntry;
};

// the creature the player sees: its own personal copy first, then the nearest one in the same phase
Creature* FindConversationActor(Player* player, uint32 entry)
{
    std::list<Creature*> creatures;
    GetCreatureListWithEntryInGrid(creatures, player, entry, 100.0f);

    Creature* nearest = nullptr;
    for (Creature* creature : creatures)
    {
        if (!creature->IsAlive() || !player->InSamePhase(creature))
            continue;

        if (creature->GetPrivateObjectOwner() == player->GetGUID())
            return creature;

        if (TempSummon const* summon = creature->ToTempSummon())
            if (summon->GetSummonerGUID() == player->GetGUID())
                return creature;

        if (!creature->GetPrivateObjectOwner().IsEmpty())
            continue;

        if (!nearest || player->GetExactDist2dSq(creature) < player->GetExactDist2dSq(nearest))
            nearest = creature;
    }

    return nearest;
}

// actors listed in conversation_actors are filled in by the core; scripts add the ones that are summons or phased copies
void StartPrivateConversation(Player* player, uint32 conversationId, std::vector<ConversationActorRef> const& actors = {})
{
    Conversation* conversation = Conversation::CreateConversation(conversationId, player, *player, player->GetGUID(), nullptr, false);
    if (!conversation)
        return;

    for (ConversationActorRef const& actor : actors)
        if (Creature* creature = FindConversationActor(player, actor.CreatureEntry))
            conversation->AddActor(actor.ActorId, actor.Index, creature->GetGUID());

    if (!conversation->Start())
        delete conversation;
}

// retail starts these a moment after the quest packet, once the quest giver has summoned its personal copies
void StartPrivateConversationDelayed(Player* player, uint32 conversationId, std::vector<ConversationActorRef> actors = {}, Milliseconds delay = 1s)
{
    player->m_Events.AddEventAtOffset([player, conversationId, actors = std::move(actors)]()
    {
        if (player->IsInWorld())
            StartPrivateConversation(player, conversationId, actors);
    }, delay);
}

void FinishRaliaRescue(Player* player)
{
    if (player->GetQuestStatus(QUEST_WHO_LURKS_IN_THE_PIT) != QUEST_STATUS_INCOMPLETE)
        return;

    player->KilledMonsterCredit(NPC_RALIA_DREAMCHASER);
    PhasingHandler::OnConditionChange(player);
    player->CastSpell(player, SPELL_RALIA_RESCUE_TELEPORT, true);
    StartPrivateConversationDelayed(player, CONVERSATION_RALIA_RESCUED,
    {
        { ACTOR_RALIA_PIT_EXIT, 1, NPC_RALIA_PIT_EXIT },
        { ACTOR_BJORN_PIT, 2, NPC_BJORN_PIT },
        { ACTOR_ALARIA, 3, NPC_ALARIA }
    });
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

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
        {
            StartPrivateConversationDelayed(player, CONVERSATION_LIGHTSPAWN_ACCEPT);
            return;
        }

        if (newStatus != QUEST_STATUS_COMPLETE)
            return;

        player->GetSceneMgr().PlayScene(SCENE_LIGHTSPAWN_FREED);
        if (Creature* lightspawn = player->FindNearestCreature(NPC_LIGHTSPAWN, 100.0f, true))
        {
            lightspawn->CastSpell(player, SPELL_LIGHTSPAWN_FREED, true);
            ObjectGuid playerGuid = player->GetGUID();
            lightspawn->m_Events.AddEventAtOffset([lightspawn, playerGuid]()
            {
                if (Player* target = ObjectAccessor::GetPlayer(*lightspawn, playerGuid))
                    lightspawn->AI()->Talk(SAY_LIGHTSPAWN_FREED, target);
            }, 6s);
        }

        PhasingHandler::OnConditionChange(player);
    }

    // every dispersed necrotic energy gets a line from the Lightspawn, the last one is said out loud
    void OnQuestObjectiveChange(Player* player, Quest const* /*quest*/, QuestObjective const& objective, int32 oldAmount, int32 newAmount) override
    {
        if (oldAmount != 0 || newAmount <= 0)
            return;

        switch (objective.ID)
        {
            case OBJECTIVE_FIRST_NECROTIC_ENERGY:
                StartPrivateConversation(player, CONVERSATION_LIGHTSPAWN_FIRST);
                break;
            case OBJECTIVE_SECOND_NECROTIC_ENERGY:
                StartPrivateConversation(player, CONVERSATION_LIGHTSPAWN_SECOND);
                break;
            case OBJECTIVE_THIRD_NECROTIC_ENERGY:
                StartPrivateConversation(player, CONVERSATION_LIGHTSPAWN_THIRD);
                break;
            case OBJECTIVE_FOURTH_NECROTIC_ENERGY:
                if (Creature* lightspawn = player->FindNearestCreature(NPC_LIGHTSPAWN, 100.0f, true))
                    lightspawn->AI()->Talk(SAY_LIGHTSPAWN_WEAKENING, player);
                break;
            default:
                break;
        }
    }
};

// 54951 - Emergency First Aid
class q54951_emergency_first_aid : public QuestScript
{
public:
    q54951_emergency_first_aid() : QuestScript("q54951_emergency_first_aid") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_EMERGENCY_FIRST_AID, { { ACTOR_JAINA_BEACH, 0, NPC_JAINA_BEACH } });
    }
};

// 55639 - Who Lurks in the Pit
class q55639_who_lurks_in_the_pit : public QuestScript
{
public:
    q55639_who_lurks_in_the_pit() : QuestScript("q55639_who_lurks_in_the_pit") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
            player->CastSpell(player, SPELL_PIT_CONVERSATION, true);
    }

    void OnQuestObjectiveChange(Player* player, Quest const* /*quest*/, QuestObjective const& objective, int32 oldAmount, int32 newAmount) override
    {
        if (objective.ID == OBJECTIVE_RESCUE_EXPEDITION && oldAmount == 0 && newAmount > 0)
            StartPrivateConversation(player, CONVERSATION_EXPEDITION_RESCUED);
    }
};

// 55764 - Harpy Culling
class q55764_harpy_culling : public QuestScript
{
public:
    q55764_harpy_culling() : QuestScript("q55764_harpy_culling") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_HARPY_CULLING_ACCEPT,
            {
                { ACTOR_HENRY_GARRICK_HARPIES, 0, NPC_HENRY_GARRICK_HARPIES },
                { ACTOR_KEE_LA_HARPIES, 1, NPC_KEE_LA_HARPIES }
            });
    }
};

// 59352 - Mage Knowledge
class q59352_mage_knowledge : public QuestScript
{
public:
    q59352_mage_knowledge() : QuestScript("q59352_mage_knowledge") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_MAGE_KNOWLEDGE);
        else if (newStatus == QUEST_STATUS_REWARDED)
            StartPrivateConversationDelayed(player, CONVERSATION_MAGE_KNOWLEDGE_DONE, { { ACTOR_MEREDY_MAGE_KNOWLEDGE, 1, NPC_MEREDY_HUNTSWELL } });
    }
};

// 59354 - The Art of Taming Sheep
class q59354_the_art_of_taming_sheep : public QuestScript
{
public:
    q59354_the_art_of_taming_sheep() : QuestScript("q59354_the_art_of_taming_sheep") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        // after the first sheep conversation is over
        if (newStatus == QUEST_STATUS_COMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_SHEEP_TRAINING_DONE,
            {
                { ACTOR_POLYMORPH_TARGET_MEREDY, 0, NPC_POLYMORPH_TARGET_MEREDY },
                { ACTOR_POLYMORPH_TARGET_COLE, 1, NPC_POLYMORPH_TARGET_COLE }
            }, 6s);
    }
};

// 55991 - The End of the Beginning
class q55991_the_end_of_the_beginning : public QuestScript
{
public:
    q55991_the_end_of_the_beginning() : QuestScript("q55991_the_end_of_the_beginning") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (oldStatus == QUEST_STATUS_NONE && newStatus == QUEST_STATUS_INCOMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_END_OF_THE_BEGINNING);
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
        Talk(SAY_MEREDY_POLYMORPH, player);
        me->CastSpell(player, SPELL_MEREDY_POLYMORPH_INTRO, true);

        TempSummon* cole = me->SummonCreature(NPC_POLYMORPH_TARGET_COLE, PolymorphColePosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 2min, 0, 0, player->GetGUID());
        if (cole)
        {
            cole->SetFaction(16);
            cole->SetReactState(REACT_PASSIVE);
        }

        TempSummon* meredy = me->SummonCreature(NPC_POLYMORPH_TARGET_MEREDY, PolymorphMeredyPosition, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 2min, 0, 0, player->GetGUID());
        if (meredy)
        {
            meredy->SetFaction(16);
            meredy->SetReactState(REACT_PASSIVE);
        }

        if (cole && meredy)
        {
            if (Conversation* conversation = Conversation::CreateConversation(CONVERSATION_SHEEP_TRAINING_START, player, *player, player->GetGUID(), nullptr, false))
            {
                conversation->AddActor(ACTOR_POLYMORPH_TARGET_COLE, 0, cole->GetGUID());
                conversation->AddActor(ACTOR_POLYMORPH_TARGET_MEREDY, 1, meredy->GetGUID());
                if (!conversation->Start())
                    delete conversation;
            }
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

        // Cole comments on the first sheep
        if (!player->IsQuestObjectiveComplete(QUEST_THE_ART_OF_TAMING_SHEEP, OBJECTIVE_PRACTICE_POLYMORPH))
            StartPrivateConversation(player, CONVERSATION_SHEEP_FIRST_POLYMORPH, { { ACTOR_POLYMORPH_TARGET_COLE, 0, NPC_POLYMORPH_TARGET_COLE } });

        player->KilledMonsterCredit(NPC_POLYMORPH_TRAINING_CREDIT);
        target->DespawnOrUnsummon(15s);
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
        Talk(SAY_MEREDY_OGRE_TRANSFORMATION, player);
        me->CastSpell(player, SPELL_OGRE_TRANSFORMATION, true);
        PhasingHandler::OnConditionChange(player);

        // retail: the disguise and the chained prisoners come once the citadel phase is loaded
        player->m_Events.AddEventAtOffset([player]()
        {
            if (!player->IsInWorld() || player->GetQuestStatus(QUEST_THE_ENEMY_OF_MY_ENEMY) != QUEST_STATUS_INCOMPLETE)
                return;

            player->CastSpell(player, SPELL_OGRE_DISGUISE, true);
            player->CastSpell(player, SPELL_SUMMON_PRISONERS, true);
            StartPrivateConversationDelayed(player, CONVERSATION_PRISONERS_DISGUISED,
            {
                { ACTOR_CAPTAIN_GARRICK_PRISONER, 0, NPC_CAPTAIN_GARRICK_PRISONER },
                { ACTOR_HENRY_GARRICK_PRISONER, 1, NPC_HENRY_GARRICK_PRISONER }
            });
        }, 6s);
        return true;
    }
};

// 55981 - The Enemy of My Enemy
class q55981_the_enemy_of_my_enemy : public QuestScript
{
public:
    q55981_the_enemy_of_my_enemy() : QuestScript("q55981_the_enemy_of_my_enemy") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        switch (newStatus)
        {
            case QUEST_STATUS_COMPLETE:
                player->CastSpell(player, SPELL_PING_FOLLOWERS_TO_FINISH, true);
                player->CastSpell(player, SPELL_CANCEL_CHAINED, true);
                StartPrivateConversation(player, CONVERSATION_PRISONERS_FREED,
                {
                    { ACTOR_HENRY_GARRICK_PRISONER, 0, NPC_HENRY_GARRICK_PRISONER },
                    { ACTOR_CAPTAIN_GARRICK_PRISONER, 1, NPC_CAPTAIN_GARRICK_PRISONER }
                });
                break;
            case QUEST_STATUS_NONE:     // abandoned
            case QUEST_STATUS_REWARDED:
                player->RemoveAurasDueToSpell(SPELL_SUMMON_PRISONERS);
                player->RemoveAurasDueToSpell(SPELL_OGRE_DISGUISE);
                break;
            default:
                break;
        }
    }

    void OnQuestObjectiveChange(Player* player, Quest const* /*quest*/, QuestObjective const& objective, int32 oldAmount, int32 newAmount) override
    {
        if (oldAmount != 0 || newAmount <= 0)
            return;

        switch (objective.ID)
        {
            case OBJECTIVE_REACH_CITADEL_ENTRANCE:
                StartPrivateConversation(player, CONVERSATION_CITADEL_ENTRANCE, { { ACTOR_CAPTAIN_GARRICK_PRISONER, 1, NPC_CAPTAIN_GARRICK_PRISONER } });
                break;
            case OBJECTIVE_FIND_COOKING_AREA:
                StartPrivateConversation(player, CONVERSATION_COOKING_AREA);
                break;
            case OBJECTIVE_WAVE_TO_GORGROTH:
                StartPrivateConversation(player, CONVERSATION_WAVE_TO_GORGROTH, { { ACTOR_GORGROTH_CAMP, 0, NPC_GORGROTH_CAMP } });
                break;
            case OBJECTIVE_DANCE_IN_COOKING_AREA:
                StartPrivateConversation(player, CONVERSATION_DANCE_FOR_THE_OGRES,
                {
                    { ACTOR_GRUNK_CAMP, 0, NPC_GRUNK_CAMP },
                    { ACTOR_CAPTAIN_GARRICK_PRISONER, 2, NPC_CAPTAIN_GARRICK_PRISONER }
                });
                break;
            default:
                break;
        }
    }
};

// 153565 - Henry Garrick, 153566 - Captain Garrick, 245399 - Lady Jaina Proudmoore (prisoners following the disguised player)
struct npc_exiles_reach_ogre_camp_prisoner : public ScriptedAI
{
    npc_exiles_reach_ogre_camp_prisoner(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!player)
            return;

        float angle = float(M_PI) / 2.0f;
        if (me->GetEntry() == NPC_HENRY_GARRICK_PRISONER)
            angle = -float(M_PI) / 2.0f;
        else if (me->GetEntry() == NPC_JAINA_PRISONER)
            angle = float(M_PI);

        me->SetReactState(REACT_PASSIVE);
        me->GetMotionMaster()->MoveFollow(player, 2.5f, angle);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(2s, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                DoCast(target, SPELL_PRISONER_CHAINED, true);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
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

    // quests that already have an upstream QuestScript: only the conversations seen in the sniff are added here
    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        QuestStatus status = player->GetQuestStatus(questId);
        if (questId == QUEST_WARMING_UP && status == QUEST_STATUS_REWARDED)
            StartPrivateConversationDelayed(player, CONVERSATION_WARMING_UP_REWARDED, { { ACTOR_JAINA_SHIP, 0, NPC_JAINA_SHIP } });
        else if (questId == QUEST_BRACE_FOR_IMPACT && status == QUEST_STATUS_INCOMPLETE)
            StartPrivateConversationDelayed(player, CONVERSATION_BRACE_FOR_IMPACT, { { ACTOR_PRIVATE_COLE_SHIP, 0, NPC_PRIVATE_COLE_SHIP } });
    }

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
    new q54951_emergency_first_aid();
    new q55639_who_lurks_in_the_pit();
    new q55764_harpy_culling();
    new q59352_mage_knowledge();
    new q59354_the_art_of_taming_sheep();
    new q55991_the_end_of_the_beginning();
    new q55981_the_enemy_of_my_enemy();
    RegisterCreatureAI(npc_exiles_reach_ogre_camp_prisoner);
    RegisterCreatureAI(npc_exiles_reach_quartermaster_richter);
    RegisterCreatureAI(npc_exiles_reach_meredy_polymorph_training);
    RegisterSpellScript(spell_exiles_reach_polymorph_training);
    RegisterCreatureAI(npc_exiles_reach_meredy_ogre_transformation);
    RegisterCreatureAI(npc_exiles_reach_kalecgos_travel);
    RegisterCreatureAI(npc_exiles_reach_happy_hal);
    new player_exiles_reach_human_mage();
}
