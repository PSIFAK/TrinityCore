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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "SceneMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SmoothPhasing.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "VendorRepair.h"
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
    QUEST_HARPY_CULLING              = 55764,
    QUEST_FINDING_THE_LOST_EXPEDITION = 54952,
    QUEST_THE_RESCUE_OF_MEREDY       = 55763,
    QUEST_CONTROLLING_THEIR_STONES   = 55990
};

enum Creatures
{
    NPC_QUARTERMASTER_RICHTER        = 156800,
    NPC_RALIA_DREAMCHASER            = 156929,
    NPC_MEREDY_HUNTSWELL             = 156886,
    NPC_MEREDY_TRANSFORMATION        = 156943,
    NPC_KALECGOS_EXILES_REACH        = 244389,
    MAP_DRAGON_ISLES                 = 2444,
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
    NPC_GRUNK_CAMP                   = 154145,
    NPC_MEREDY_CAPTIVE               = 153211,
    NPC_WUG_CAMP                     = 154146,
    NPC_JUGNUG_CAMP                  = 154147,
    NPC_KELRA_CAPTIVE                = 156954
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
    CONVERSATION_PRISONERS_FREED      = 11414,
    CONVERSATION_LOST_EXPEDITION      = 12044
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
    SAY_LIGHTSPAWN_FREED             = 1,
    SAY_JAINA_SEARCH_SURVIVORS       = 2,
    SAY_MEREDY_FREED                 = 0,
    SAY_KELRA_FREED                  = 0
};

enum Spells
{
    SPELL_RALIA_INTERACT             = 312463,
    SPELL_MEREDY_POLYMORPH_INTRO     = 321134,
    SPELL_MEREDY_POLYMORPH_OUTRO     = 321139,
    SPELL_POLYMORPH                  = 118,
    SPELL_OGRE_TRANSFORMATION        = 313583,
    SPELL_LIGHTSPAWN_FREED           = 319171,
    SPELL_DRAGON_ISLES_MOVIE         = 1235041,  // Play Movie, Load (DNT): movie 1038
    SPELL_DRAGON_ISLES_TRAVEL        = 1238030,  // Play Movie, Load (DNT): kill credit 244389, teleport, bind
    SPELL_HAPPY_HAL_CONVERSATION     = 375316,
    SPELL_PIT_CONVERSATION           = 306109,
    SPELL_RALIA_RESCUE_TELEPORT      = 312548,
    SPELL_OGRE_DISGUISE              = 298241,
    SPELL_SUMMON_PRISONERS           = 298232,   // linked summons of Captain Garrick, Henry Garrick and Jaina Proudmoore
    SPELL_PRISONER_CHAINED           = 299155,
    SPELL_PING_FOLLOWERS_TO_FINISH   = 298359,
    SPELL_CANCEL_CHAINED             = 322699,
    SPELL_NECROTIC_RITUAL            = 305513,   // the captive Meredy's prison
    SPELL_ARCANE_BURST               = 313541
};

enum Points
{
    POINT_MEREDY_ESCAPE              = 1,
    POINT_OGRE_LEAVE                 = 2
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

// The Rescue of Meredy Huntswell (55763), from the retail sniff (12.1.5.70688, night elf hunter): talking to the captive starts
// personal waves of harpies and worgs, Kee-La and Henry Garrick come to help and Bloodbeak flies in from the nest last
enum MeredyAmbush
{
    GOSSIP_MENU_MEREDY_CAPTIVE        = 24887,
    GOSSIP_OPTION_MEREDY_CAPTIVE      = 0,

    NPC_HARPY_AMBUSHER                = 155192,
    NPC_HUNTING_WORG                  = 169162,
    NPC_HENRY_GARRICK_AMBUSH          = 155197,
    NPC_KEE_LA_AMBUSH                 = 155199,
    NPC_BLOODBEAK                     = 153964,

    CONVERSATION_AMBUSH_ALLIES        = 11738,
    CONVERSATION_AMBUSH_BLOODBEAK     = 11739,
    ACTOR_HENRY_GARRICK_AMBUSH        = 70728,
    ACTOR_KEE_LA_AMBUSH               = 70729,

    SAY_MEREDY_BREAK_FREE             = 1,
    SAY_HARPY_BITE                    = 0,
    SAY_HARPY_DINNER                  = 1,

    SPELL_SWOOPING_LUNGE              = 317559,
    SPELL_HOLY_SMITE                  = 172673,
    SPELL_HEAL_COVER                  = 222586,
    SPELL_RITUAL_SCENE_OGRE_CITADEL   = 321693,
    SPELL_RITUAL_SCENE_HARPY_BEAM     = 321691,
    SPELL_RITUAL_SCENE_MAIN_BEAM      = 321690,

    POINT_AMBUSH_ALLY_ARRIVED         = 3,
    POINT_BLOODBEAK_LANDED            = 4
};

struct MeredyAmbushSpawn
{
    Milliseconds Delay;               // after the gossip option
    uint32 Entry;
    Position Pos;
    Optional<uint8> TextGroup;        // said this long after the spawn
    Milliseconds TextDelay = 0ms;
};

MeredyAmbushSpawn const MeredyAmbushWaves[] =
{
    { 160ms,   NPC_HARPY_AMBUSHER, { 478.99133f, -2358.611f, 160.28699f, 0.0f }, SAY_HARPY_BITE, 4500ms },
    { 160ms,   NPC_HARPY_AMBUSHER, { 484.2205f, -2363.0017f, 159.88463f, 0.0f }, SAY_HARPY_BITE, 2950ms },
    { 14370ms, NPC_HARPY_AMBUSHER, { 503.73856f, -2348.2097f, 161.77324f, 3.838f }, SAY_HARPY_DINNER, 2950ms },
    { 14370ms, NPC_HUNTING_WORG,   { 503.77698f, -2367.946f, 158.9525f, 1.461f } },
    { 29670ms, NPC_HUNTING_WORG,   { 508.35614f, -2364.045f, 159.81624f, 2.708f } },
    { 29670ms, NPC_HUNTING_WORG,   { 512.1088f, -2353.629f, 160.55806f, 4.644f } },
    { 29670ms, NPC_HUNTING_WORG,   { 504.9585f, -2368.7148f, 158.94443f, 0.481f } },
    { 46180ms, NPC_HUNTING_WORG,   { 484.2205f, -2363.0017f, 159.88463f, 1.238f } },
    { 46180ms, NPC_HARPY_AMBUSHER, { 494.88715f, -2370.54f, 158.73819f, 1.621f } },
    { 46180ms, NPC_HUNTING_WORG,   { 491.93228f, -2340.7153f, 161.85861f, 4.790f } }
};

Milliseconds constexpr MeredyAmbushAlliesDelay = 17240ms;
Milliseconds constexpr MeredyAmbushBloodbeakTalkDelay = 51660ms;
Milliseconds constexpr MeredyAmbushBloodbeakDelay = 56660ms;
Position const KeeLaAmbushPosition = { 494.87152f, -2383.691f, 155.85905f, 1.338f };
Position const KeeLaAmbushDestination = { 498.42627f, -2355.3403f, 160.65552f };
Position const HenryGarrickAmbushPosition = { 492.3885f, -2380.0f, 156.9f, 1.400f };
Position const HenryGarrickAmbushDestination = { 495.146f, -2357.8f, 160.13092f };
Position const BloodbeakNestPosition = { 490.625f, -2429.6182f, 174.1879f, 1.518376f };
Position const BloodbeakLandingPosition = { 493.93402f, -2364.8784f, 161.7566f };

// hunter class quests of the camp below the Ogre Ruins, from the retail sniff (12.1.5.70688, night elf hunter)
enum ExilesReachHunter
{
    QUEST_A_HUNTERS_TRAP              = 59355,
    QUEST_HUNTING_THE_STALKER         = 59356,
    QUEST_THE_ART_OF_TAMING           = 60168,

    OBJECTIVE_ICE_MOTE                = 396411,
    OBJECTIVE_TAME_NEW_PET            = 397752,

    NPC_AUSTIN_HUXWORTH_TRAINER       = 161666,
    NPC_FREEZING_TRAP                 = 164986,
    NPC_AUSTIN_HUXWORTH_TRAP          = 164989,
    NPC_STALKING_PROWLER              = 164990,

    SPELL_RECEIVE_ICE_MOTE            = 321160,
    SPELL_FREEZING_TRAP_OVERRIDE      = 321163,
    SPELL_STALKER_FROZEN              = 321177,
    SPELL_RACE_CHECK_FOR_CRITTER      = 325857,

    CONVERSATION_HUNTING_THE_STALKER  = 13707,
    CONVERSATION_STALKER_COMING       = 16330,
    CONVERSATION_STALKER_TRAPPED      = 16331,
    CONVERSATION_STALKER_CAPTURED     = 16332,
    ACTOR_AUSTIN_TRAINER              = 74936,
    ACTOR_AUSTIN_TRAP                 = 74949,

    GOSSIP_OPTION_MEREDY_ICE_MOTE     = 1,
    GOSSIP_MENU_AUSTIN_STABLE         = 24886,
    GOSSIP_OPTION_AUSTIN_STABLE       = 0,
    GOSSIP_OPTION_AUSTIN_TAME_PET     = 2,

    QUEST_TAMING_THE_WILDS_ALLIANCE_HUNTER = 59342,
    NPC_FREED_EXPEDITION_MEMBER       = 160964,
    GO_THICK_COCOON                   = 339568,
    SAY_FREED_EXPEDITION_MEMBER       = 0,
    QUEST_COOKING_MEAT                = 55174,
    NPC_ALARIA_STANDING               = 175031,
    ANIM_KIT_ALARIA_STAND_UP          = 14432,
    SPELL_TAME_BEAST_HUNTER           = 1515,
    SPELL_CALL_PET_1                  = 883,
    FLYOUT_CALL_PET                   = 9,
    FLYOUT_PET_UTILITY                = 103,

    POINT_STALKER_TRAP                = 1,
    POINT_AUSTIN_HOME                 = 2
};

// Austin comes out to watch the trap and walks back; the stalker follows the path of the sniff to the trap and,
// freed again, runs off fading
// where the freed expedition members of Who Lurks in the Pit run off to
Position const PitExitPosition = { 81.58753f, -2266.2449f, 67.227455f };
Position const AustinTrapSpawnPosition = { 183.23785f, -2295.269f, 82.32755f, 0.2302532f };
Position const AustinTrapWatchPosition = { 191.93803f, -2278.9788f, 81.93185f };
Position const AustinTrapCheckPosition = { 195.70566f, -2263.5452f, 81.52985f };
Position const StalkerSpawnPosition = { 208.59793f, -2229.475f, 85.599525f, 3.8677006f };
Position const StalkerFleePosition = { 186.37544f, -2277.9597f, 81.851456f };
// the new pet leaps down to the camp and comes to the player
Position const TamingPetSpawnPosition = { 167.79558f, -2279.749f, 83.07357f, 5.978723f };
Position const TamingPetLandPosition = { 181.26f, -2283.98f, 81.8484f };
Position const TamingPetWaitPosition = { 197.5816f, -2277.2415f, 81.19375f };

Position const PolymorphColePosition = { 182.99826f, -2286.684f, 81.84842f, 0.0f };
Position const PolymorphMeredyPosition = { 187.06424f, -2279.7275f, 82.01448f, 1.8322159f };
Position const MeredyEscapePosition = { 491.2014f, -2388.677f, 155.9098f };

// where the dancing ogres walk off to after the dance in the sniff
Position const GrunkLeavePosition = { 537.41144f, -2037.5555f, 148.88293f };
Position const WugLeavePosition = { 627.42535f, -2098.519f, 158.62624f };
Position const JugnugLeavePosition = { 542.23785f, -2060.8074f, 155.10031f };

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

    // the spellclick spell 312463 (Escape) plays scene 2379 through its aura; playing it from OnSpellClick as well showed the escape twice
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

        // the captive and the freed Ralia and the defeated Hrun are shown by spawn conditions on these objectives
        player->UpdateObjectVisibility();
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

// 55763 - The Rescue of Meredy Huntswell
class q55763_the_rescue_of_meredy_huntswell : public QuestScript
{
public:
    q55763_the_rescue_of_meredy_huntswell() : QuestScript("q55763_the_rescue_of_meredy_huntswell") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_COMPLETE)
            return;

        // retail: once Bloodbeak is dead the captive (phase 13566) is replaced by a freed Meredy who breaks out and runs off
        std::list<Creature*> captives;
        GetCreatureListWithEntryInGrid(captives, player, NPC_MEREDY_CAPTIVE, 150.0f);
        for (Creature* captive : captives)
        {
            if (captive->IsPrivateObject() || !captive->IsAlive())
                continue;

            captive->SummonPersonalClone(captive->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 30s, 0, 0, player);
            break;
        }

        // retail cancels the three ritual scenes and plays the two that remain without the harpy beam
        player->RemoveAurasDueToSpell(SPELL_RITUAL_SCENE_OGRE_CITADEL);
        player->RemoveAurasDueToSpell(SPELL_RITUAL_SCENE_HARPY_BEAM);
        player->RemoveAurasDueToSpell(SPELL_RITUAL_SCENE_MAIN_BEAM);
        player->CastSpell(player, SPELL_RITUAL_SCENE_OGRE_CITADEL, true);
        player->CastSpell(player, SPELL_RITUAL_SCENE_MAIN_BEAM, true);

        PhasingHandler::OnConditionChange(player);
    }
};

// 55990 - Controlling their Stones
class q55990_controlling_their_stones : public QuestScript
{
public:
    q55990_controlling_their_stones() : QuestScript("q55990_controlling_their_stones") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_COMPLETE)
            return;

        // retail: the captive Kelra (phase 13334, here 13566) is replaced by a personal copy that thanks the player; a few seconds later
        // the phases change and the freed Kelra (phase 13843), who takes the quest, stands in her place
        std::list<Creature*> captives;
        GetCreatureListWithEntryInGrid(captives, player, NPC_KELRA_CAPTIVE, 100.0f);
        for (Creature* captive : captives)
        {
            if (captive->IsPrivateObject() || !captive->IsAlive())
                continue;

            captive->SummonPersonalClone(captive->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 5s, 0, 0, player);
            break;
        }

        player->m_Events.AddEventAtOffset([player]()
        {
            if (player->IsInWorld())
                PhasingHandler::OnConditionChange(player);
        }, 5s);
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

    // retail (12.1.5 sniff) credits Repair Yourself right after CMSG_REPAIR_ITEM; the vendor window skips the gossip
    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        if (id != RetailSystems::NewPlayerExperience::DATA_PLAYER_REPAIRED_ITEMS)
            return;

        Player* player = ObjectAccessor::GetPlayer(*me, guid);
        if (player && player->GetQuestStatus(QUEST_GEARED_FOR_REPAIR) == QUEST_STATUS_INCOMPLETE)
            player->KilledMonsterCredit(NPC_QUARTERMASTER_RICHTER, me->GetGUID());
    }
};

// 153211 - Meredy Huntswell (captive in the harpy nest; the personal clone summoned by q55763 is the freed one)
struct npc_exiles_reach_meredy_captive : public ScriptedAI
{
    npc_exiles_reach_meredy_captive(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!player)
            return;

        me->RemoveAurasDueToSpell(SPELL_NECROTIC_RITUAL);
        me->SetSessile(false);
        me->SetControlled(false, UNIT_STATE_ROOT);
        me->SetDisableGravity(false);
        me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
        me->SetReactState(REACT_PASSIVE);
        DoCastSelf(SPELL_ARCANE_BURST);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(1s + 300ms, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                Talk(SAY_MEREDY_FREED, target);
        });
        _scheduler.Schedule(7s, [this](TaskContext const& /*context*/)
        {
            me->GetMotionMaster()->MovePoint(POINT_MEREDY_ESCAPE, MeredyEscapePosition);
        });
    }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_MEREDY_CAPTIVE || gossipListId != GOSSIP_OPTION_MEREDY_CAPTIVE)
            return false;

        player->PlayerTalkClass->SendCloseGossip();
        if (me->IsPrivateObject() || player->GetQuestStatus(QUEST_THE_RESCUE_OF_MEREDY) != QUEST_STATUS_INCOMPLETE ||
            _ambushes.contains(player->GetGUID()))
            return true;

        StartAmbush(player);
        return true;
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() != NPC_BLOODBEAK)
            return;

        ObjectGuid playerGuid = summon->GetPrivateObjectOwner();
        auto ambush = _ambushes.find(playerGuid);
        if (ambush == _ambushes.end() || ambush->second.Finished)
            return;

        // the kill counts even when the allies land the last hit; the quest script then frees Meredy
        ambush->second.Finished = true;
        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
            player->KilledMonsterCredit(NPC_BLOODBEAK, summon->GetGUID());

        // retail: the allies leave 12 s after the kill, the remaining harpies and worgs 18 s after it
        uint32 ambushId = ambush->second.Id;
        _scheduler.Schedule(12s, [this, playerGuid, ambushId](TaskContext const& /*context*/)
        {
            if (Ambush* current = GetAmbush(playerGuid, ambushId))
                for (ObjectGuid const& guid : current->Summons)
                    if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                        if (summon->GetEntry() == NPC_HENRY_GARRICK_AMBUSH || summon->GetEntry() == NPC_KEE_LA_AMBUSH)
                            summon->DespawnOrUnsummon();
        });
        _scheduler.Schedule(18s, [this, playerGuid, ambushId](TaskContext const& /*context*/)
        {
            if (GetAmbush(playerGuid, ambushId))
                EndAmbush(playerGuid);
        });
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == POINT_MEREDY_ESCAPE)
            me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    struct Ambush
    {
        uint32 Id = 0;
        bool Finished = false;
        std::vector<ObjectGuid> Summons;
    };

    Ambush* GetAmbush(ObjectGuid const& playerGuid, uint32 ambushId)
    {
        auto itr = _ambushes.find(playerGuid);
        return itr != _ambushes.end() && itr->second.Id == ambushId ? &itr->second : nullptr;
    }

    // every creature of the event belongs to the player alone, as on retail
    Creature* SummonForAmbush(ObjectGuid const& playerGuid, uint32 entry, Position const& pos)
    {
        Creature* summon = me->SummonCreature(entry, pos, TEMPSUMMON_TIMED_DESPAWN, 3min, 0, 0, playerGuid);
        if (summon)
            _ambushes[playerGuid].Summons.push_back(summon->GetGUID());
        return summon;
    }

    void StartAmbush(Player* player)
    {
        ObjectGuid playerGuid = player->GetGUID();
        uint32 ambushId = ++_ambushCounter;
        _ambushes[playerGuid].Id = ambushId;

        Talk(SAY_MEREDY_BREAK_FREE, player);

        for (MeredyAmbushSpawn const& spawn : MeredyAmbushWaves)
        {
            _scheduler.Schedule(spawn.Delay, [this, playerGuid, ambushId, &spawn](TaskContext const& /*context*/)
            {
                Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (!target || !GetAmbush(playerGuid, ambushId))
                    return;

                Creature* enemy = SummonForAmbush(playerGuid, spawn.Entry, spawn.Pos);
                if (!enemy)
                    return;

                enemy->SetReactState(REACT_AGGRESSIVE);
                enemy->AI()->AttackStart(target);
                if (spawn.TextGroup)
                {
                    ObjectGuid enemyGuid = enemy->GetGUID();
                    uint8 textGroup = *spawn.TextGroup;
                    _scheduler.Schedule(spawn.TextDelay, [this, playerGuid, enemyGuid, textGroup](TaskContext const& /*context*/)
                    {
                        Creature* speaker = ObjectAccessor::GetCreature(*me, enemyGuid);
                        Player* listener = ObjectAccessor::GetPlayer(*me, playerGuid);
                        if (speaker && speaker->IsAlive() && listener)
                            speaker->AI()->Talk(textGroup, listener);
                    });
                }
            });
        }

        _scheduler.Schedule(MeredyAmbushAlliesDelay, [this, playerGuid, ambushId](TaskContext const& /*context*/)
        {
            Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!target || !GetAmbush(playerGuid, ambushId))
                return;

            if (Creature* keeLa = SummonForAmbush(playerGuid, NPC_KEE_LA_AMBUSH, KeeLaAmbushPosition))
                keeLa->GetMotionMaster()->MovePoint(POINT_AMBUSH_ALLY_ARRIVED, KeeLaAmbushDestination);
            if (Creature* henry = SummonForAmbush(playerGuid, NPC_HENRY_GARRICK_AMBUSH, HenryGarrickAmbushPosition))
                henry->GetMotionMaster()->MovePoint(POINT_AMBUSH_ALLY_ARRIVED, HenryGarrickAmbushDestination);

            StartPrivateConversation(target, CONVERSATION_AMBUSH_ALLIES,
            {
                { ACTOR_KEE_LA_AMBUSH, 0, NPC_KEE_LA_AMBUSH },
                { ACTOR_HENRY_GARRICK_AMBUSH, 1, NPC_HENRY_GARRICK_AMBUSH }
            });
        });

        _scheduler.Schedule(MeredyAmbushBloodbeakTalkDelay, [this, playerGuid, ambushId](TaskContext const& /*context*/)
        {
            Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (target && GetAmbush(playerGuid, ambushId))
                StartPrivateConversation(target, CONVERSATION_AMBUSH_BLOODBEAK,
                {
                    { ACTOR_KEE_LA_AMBUSH, 0, NPC_KEE_LA_AMBUSH },
                    { ACTOR_HENRY_GARRICK_AMBUSH, 1, NPC_HENRY_GARRICK_AMBUSH }
                });
        });

        // Bloodbeak glides down from the nest (retail spline: 4.2 s) and opens with Swooping Lunge on the allies
        _scheduler.Schedule(MeredyAmbushBloodbeakDelay, [this, playerGuid, ambushId](TaskContext const& /*context*/)
        {
            if (!ObjectAccessor::GetPlayer(*me, playerGuid) || !GetAmbush(playerGuid, ambushId))
                return;

            Creature* bloodbeak = SummonForAmbush(playerGuid, NPC_BLOODBEAK, BloodbeakNestPosition);
            if (!bloodbeak)
                return;

            bloodbeak->SetReactState(REACT_PASSIVE);
            bloodbeak->SetDisableGravity(true);
            bloodbeak->GetMotionMaster()->MovePoint(POINT_BLOODBEAK_LANDED, BloodbeakLandingPosition, false, {}, 15.5f);

            ObjectGuid bloodbeakGuid = bloodbeak->GetGUID();
            _scheduler.Schedule(4200ms, [this, playerGuid, bloodbeakGuid](TaskContext const& /*context*/)
            {
                Creature* landed = ObjectAccessor::GetCreature(*me, bloodbeakGuid);
                Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (!landed || !landed->IsAlive() || !target)
                    return;

                landed->SetDisableGravity(false);
                landed->SetReactState(REACT_AGGRESSIVE);
                landed->CastSpell(landed, SPELL_SWOOPING_LUNGE);
                landed->AI()->AttackStart(target);
            });
        });

        // the event stops when the player dies, leaves the nest or drops the quest
        _scheduler.Schedule(1s, [this, playerGuid, ambushId](TaskContext context)
        {
            Ambush* ambush = GetAmbush(playerGuid, ambushId);
            if (!ambush || ambush->Finished)
                return;

            Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!target || !target->IsAlive() || !target->IsWithinDist(me, 80.0f) ||
                target->GetQuestStatus(QUEST_THE_RESCUE_OF_MEREDY) != QUEST_STATUS_INCOMPLETE)
            {
                EndAmbush(playerGuid);
                return;
            }

            context.Repeat();
        });
    }

    void EndAmbush(ObjectGuid const& playerGuid)
    {
        auto itr = _ambushes.find(playerGuid);
        if (itr == _ambushes.end())
            return;

        for (ObjectGuid const& guid : itr->second.Summons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                summon->DespawnOrUnsummon();

        _ambushes.erase(itr);
    }

    TaskScheduler _scheduler;
    std::unordered_map<ObjectGuid, Ambush> _ambushes;
    uint32 _ambushCounter = 0;
};

// 155197 - Henry Garrick, 155199 - Kee-La (allies of the Meredy rescue; Henry heals and smites, Kee-La fights in melee)
struct npc_exiles_reach_meredy_ambush_ally : public ScriptedAI
{
    npc_exiles_reach_meredy_ambush_ally(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _scheduler.CancelAll();
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_AMBUSH_ALLY_ARRIVED)
            return;

        // join the fight against whatever the player is facing
        if (Player* player = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner()))
            if (Unit* enemy = player->GetVictim() ? player->GetVictim() : player->getAttackerForHelper())
                AttackStart(enemy);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetEntry() != NPC_HENRY_GARRICK_AMBUSH)
            return;

        _scheduler.Schedule(1s, [this](TaskContext context)
        {
            if (Unit* victim = me->GetVictim())
                DoCast(victim, SPELL_HOLY_SMITE);
            context.Repeat(2500ms);
        });
        _scheduler.Schedule(2s, [this](TaskContext context)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner());
            if (player && player->IsAlive() && player->HealthBelowPct(80))
            {
                DoCast(player, SPELL_HEAL_COVER);
                context.Repeat(12s);
                return;
            }
            context.Repeat(2s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
        {
            // help the player as soon as something attacks them
            if (Player* player = ObjectAccessor::GetPlayer(*me, me->GetPrivateObjectOwner()))
                if (Unit* enemy = player->getAttackerForHelper())
                    if (!me->isMoving())
                        AttackStart(enemy);
            return;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 156954 - Captain Kelra (captive at the citadel entrance; the personal copy summoned by q55990 thanks the player)
struct npc_exiles_reach_kelra_captive : public ScriptedAI
{
    npc_exiles_reach_kelra_captive(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!player)
            return;

        me->RemoveAurasDueToSpell(SPELL_NECROTIC_RITUAL);
        me->SetReactState(REACT_PASSIVE);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(1s + 500ms, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                Talk(SAY_KELRA_FREED, target);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 156886 - Meredy Huntswell
struct npc_exiles_reach_meredy_polymorph_training : public ScriptedAI
{
    npc_exiles_reach_meredy_polymorph_training(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        // hunter quest 59355: Meredy hands over an ice mote for the trap (retail option 51803)
        if (menuId == GOSSIP_MENU_MEREDY_TRAINING && gossipListId == GOSSIP_OPTION_MEREDY_ICE_MOTE)
        {
            if (player->GetQuestStatus(QUEST_A_HUNTERS_TRAP) != QUEST_STATUS_INCOMPLETE || player->IsQuestObjectiveComplete(QUEST_A_HUNTERS_TRAP, OBJECTIVE_ICE_MOTE))
                return false;

            player->PlayerTalkClass->SendCloseGossip();
            player->CastSpell(player, SPELL_RECEIVE_ICE_MOTE, true);
            return true;
        }

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
                // retail: the assault camp with Jaina, who takes the quest, is phased in a few seconds later
                player->m_Events.AddEventAtOffset([player]()
                {
                    if (player->IsInWorld())
                        PhasingHandler::OnConditionChange(player);
                }, 8s);
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
                // retail: Grunk, Wug and Jugnug are replaced by personal copies that dance along, then walk off
                for (uint32 entry : { NPC_GRUNK_CAMP, NPC_WUG_CAMP, NPC_JUGNUG_CAMP })
                {
                    std::list<Creature*> ogres;
                    GetCreatureListWithEntryInGrid(ogres, player, entry, 60.0f);
                    for (Creature* ogre : ogres)
                    {
                        if (ogre->IsPrivateObject() || !ogre->IsAlive() || !player->InSamePhase(ogre))
                            continue;

                        ogre->SummonPersonalClone(ogre->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 40s, 0, 0, player);
                        break;
                    }
                }
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

        me->SetReactState(REACT_PASSIVE);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(2s, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                DoCast(target, SPELL_PRISONER_CHAINED, true);
        });
    }

    // CreatureAI::JustAppeared makes every summon follow at the same pet angle, so the three prisoners walked inside each other
    Unit* GetFollowTarget() const
    {
        Unit* owner = me->GetCharmerOrOwner();
        if (!owner && me->IsSummon())
            owner = me->ToTempSummon()->GetSummonerUnit();
        return owner;
    }

    void JustAppeared() override
    {
        Unit* owner = GetFollowTarget();
        if (!owner)
            return;

        float angle = float(M_PI) / 2.0f;
        if (me->GetEntry() == NPC_HENRY_GARRICK_PRISONER)
            angle = -float(M_PI) / 2.0f;
        else if (me->GetEntry() == NPC_JAINA_PRISONER)
            angle = float(M_PI);

        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveFollow(owner, 2.5f, angle);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        JustAppeared();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);

        // retail keeps the chained prisoners right behind the player; at the same speed the follow movement fell behind
        if (Unit* owner = GetFollowTarget())
        {
            float rate = owner->GetSpeedRate(MOVE_RUN);
            if (me->GetExactDist2d(owner) > 5.0f)
                rate *= 1.5f;
            me->SetSpeedRate(MOVE_RUN, rate);
            me->SetSpeedRate(MOVE_WALK, owner->GetSpeedRate(MOVE_WALK) * (rate / owner->GetSpeedRate(MOVE_RUN)));
        }
    }

private:
    TaskScheduler _scheduler;
};

// 154145 - Grunk, 154146 - Wug, 154147 - Jugnug (the personal copies summoned when the player dances in the cooking area)
struct npc_exiles_reach_dancing_ogre : public ScriptedAI
{
    npc_exiles_reach_dancing_ogre(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        _scheduler.Schedule(1s + 500ms, [this](TaskContext const& /*context*/)
        {
            me->SetEmoteState(EMOTE_STATE_DANCE);
        });
        _scheduler.Schedule(12s, [this](TaskContext const& /*context*/)
        {
            me->SetEmoteState(EMOTE_ONESHOT_NONE);
            Position const* leave = &GrunkLeavePosition;
            if (me->GetEntry() == NPC_WUG_CAMP)
                leave = &WugLeavePosition;
            else if (me->GetEntry() == NPC_JUGNUG_CAMP)
                leave = &JugnugLeavePosition;
            me->SetWalk(true);
            me->GetMotionMaster()->MovePoint(POINT_OGRE_LEAVE, *leave);
        });
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == POINT_OGRE_LEAVE)
            me->DespawnOrUnsummon();
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 326651 - Catapult (55989 Catapult Destruction)
struct go_exiles_reach_catapult : public GameObjectAI
{
    // the spawn tracking state of a destroyed catapult shows the burning wreck; before it retail shows the bomb and the explosion
    static constexpr uint32 VISUAL_BOMB = 100772;
    static constexpr uint32 VISUAL_EXPLOSION = 100773;

    go_exiles_reach_catapult(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        GameObjectData const* data = me->GetGameObjectData();
        if (!data || !data->spawnTrackingData ||
            player->GetSpawnTrackingStateByObjectives(data->spawnTrackingData->SpawnTrackingId, data->spawnTrackingQuestObjectives) != SpawnTrackingState::Active)
            return false;

        me->SetStateSpellVisualFor(VISUAL_BOMB, player);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(2s, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                me->SetStateSpellVisualFor(VISUAL_EXPLOSION, target);
        });
        _scheduler.Schedule(3s + 200ms, [this, playerGuid](TaskContext const& /*context*/)
        {
            if (Player* target = ObjectAccessor::GetPlayer(*me, playerGuid))
                me->SetStateSpellVisualFor(0, target);
        });
        return false;
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

        // retail (12.0.1 sniff): the gossip plays the movie (1235041); 14 s later Play Movie, Load (1238030) gives the kill
        // credit, teleports to the Wild Coast behind a loading screen and binds the hearthstone there. Nothing changes on the
        // island before that, so its people stay in view while the movie plays.
        player->PlayerTalkClass->SendCloseGossip();
        me->CastSpell(player, SPELL_DRAGON_ISLES_MOVIE, true);

        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(14s, [this, playerGuid](TaskContext const& /*context*/)
        {
            Player* target = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!target || target->GetQuestStatus(QUEST_THE_END_OF_THE_BEGINNING) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_DRAGON_ISLES_TRAVEL, true);
            if (target->GetQuestStatus(QUEST_THE_END_OF_THE_BEGINNING) == QUEST_STATUS_INCOMPLETE)
                target->KilledMonsterCredit(NPC_KALECGOS_EXILES_REACH);
            if (target->GetMapId() != MAP_DRAGON_ISLES && !target->IsBeingTeleportedFar())
                target->TeleportTo(MAP_DRAGON_ISLES, 3713.47f, -1890.45f, 5.77f, 2.7612855f, TELE_TO_NONE, {}, SPELL_DRAGON_ISLES_TRAVEL);
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

// the beast each race brings from home, as on the ship
uint32 GetHomePetEntry(Player const* player)
{
    switch (player->GetRace())
    {
        case RACE_DWARF:            return 167375; // Bear
        case RACE_NIGHTELF:         return 167347; // Tiger
        case RACE_GNOME:            return 167337; // Mechanical Bunny
        case RACE_DRAENEI:          return 167342; // Moth
        case RACE_WORGEN:           return 167352; // Dog
        case RACE_PANDAREN_NEUTRAL:
        case RACE_PANDAREN_ALLIANCE:
        case RACE_PANDAREN_HORDE:   return 167348; // Turtle
        case RACE_ORC:              return 167346; // Wolf
        case RACE_UNDEAD_PLAYER:    return 167351; // Bat
        case RACE_TAUREN:           return 167349; // Plainstrider
        case RACE_TROLL:            return 167350; // Raptor
        case RACE_BLOODELF:         return 167343; // Dragonhawk
        case RACE_GOBLIN:           return 167344; // Scorpion
        default:                    return 167345; // Wolf
    }
}

// 59356 - Hunting the Stalker
class q59356_hunting_the_stalker : public QuestScript
{
public:
    q59356_hunting_the_stalker() : QuestScript("q59356_hunting_the_stalker") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus oldStatus, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_INCOMPLETE)
        {
            player->RemoveAurasDueToSpell(SPELL_FREEZING_TRAP_OVERRIDE);
            return;
        }

        // Freezing Trap on the action bar places the quest's own trap
        player->CastSpell(player, SPELL_FREEZING_TRAP_OVERRIDE, true);
        if (oldStatus == QUEST_STATUS_NONE)
            StartPrivateConversation(player, CONVERSATION_HUNTING_THE_STALKER, { { ACTOR_AUSTIN_TRAINER, 0, NPC_AUSTIN_HUXWORTH_TRAINER } });
    }
};

// 164986 - Freezing Trap (placed by the player on quest 59356)
struct npc_exiles_reach_freezing_trap : public ScriptedAI
{
    npc_exiles_reach_freezing_trap(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        Player* player = summoner->ToPlayer();
        if (!player || player->GetQuestStatus(QUEST_HUNTING_THE_STALKER) != QUEST_STATUS_INCOMPLETE)
            return;

        // retail: Austin comes out and the stalker appears 3.3 s after the trap is placed
        ObjectGuid playerGuid = player->GetGUID();
        _scheduler.Schedule(3300ms, [this, playerGuid](TaskContext const& /*context*/)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
            if (!player || player->GetQuestStatus(QUEST_HUNTING_THE_STALKER) != QUEST_STATUS_INCOMPLETE || HasStalkerFor(player))
                return;

            Map* map = me->GetMap();
            TempSummon* austin = nullptr;
            if (Creature* trainer = player->FindNearestCreature(NPC_AUSTIN_HUXWORTH_TRAINER, 60.0f))
            {
                // replaces the quest giver for this player while it lasts, like the sniff
                SmoothPhasingInfo smoothPhasing{ trainer->GetGUID(), true, true };
                austin = map->SummonCreature(NPC_AUSTIN_HUXWORTH_TRAP, AustinTrapSpawnPosition, nullptr, 2min, player, 0, 0, player->GetGUID(), &smoothPhasing);
            }
            else
                austin = map->SummonCreature(NPC_AUSTIN_HUXWORTH_TRAP, AustinTrapSpawnPosition, nullptr, 2min, player, 0, 0, player->GetGUID());

            TempSummon* stalker = map->SummonCreature(NPC_STALKING_PROWLER, StalkerSpawnPosition, nullptr, 2min, player, 0, 0, player->GetGUID());
            if (!stalker)
                return;

            if (austin)
            {
                austin->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
                stalker->AI()->SetGUID(austin->GetGUID(), NPC_AUSTIN_HUXWORTH_TRAP);
                StartPrivateConversationDelayed(player, CONVERSATION_STALKER_COMING, { { ACTOR_AUSTIN_TRAP, 0, NPC_AUSTIN_HUXWORTH_TRAP } }, 800ms);
            }

            stalker->AI()->SetGUID(me->GetGUID(), NPC_FREEZING_TRAP);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    static bool HasStalkerFor(Player* player)
    {
        std::list<Creature*> stalkers;
        GetCreatureListWithEntryInGrid(stalkers, player, NPC_STALKING_PROWLER, 100.0f);
        return std::ranges::any_of(stalkers, [player](Creature const* stalker) { return stalker->GetPrivateObjectOwner() == player->GetGUID(); });
    }

    TaskScheduler _scheduler;
};

// 164989 - Austin Huxworth (watches the trap)
struct npc_exiles_reach_austin_huxworth_trap : public ScriptedAI
{
    npc_exiles_reach_austin_huxworth_trap(Creature* creature) : ScriptedAI(creature) { }

    void JustAppeared() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->GetMotionMaster()->MovePoint(0, AustinTrapWatchPosition, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == POINT_AUSTIN_HOME)
            me->DespawnOrUnsummon();
    }
};

// 164990 - Stalking Prowler
struct npc_exiles_reach_stalking_prowler : public ScriptedAI
{
    npc_exiles_reach_stalking_prowler(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _playerGuid = summoner->GetGUID();
        me->SetReactState(REACT_PASSIVE);
    }

    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        if (id == NPC_AUSTIN_HUXWORTH_TRAP)
            _austinGuid = guid;
        else if (id == NPC_FREEZING_TRAP)
        {
            _trapGuid = guid;
            // the sniff: 31 yd in 9.8 s
            if (Creature* trap = ObjectAccessor::GetCreature(*me, guid))
                me->GetMotionMaster()->MovePoint(POINT_STALKER_TRAP, trap->GetPosition(), true, {}, 3.1f);
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_STALKER_TRAP)
            return;

        DoCastSelf(SPELL_STALKER_FROZEN, true);
        if (Creature* trap = ObjectAccessor::GetCreature(*me, _trapGuid))
            trap->DespawnOrUnsummon();

        Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid);
        if (!player)
            return;

        StartPrivateConversation(player, CONVERSATION_STALKER_TRAPPED, { { ACTOR_AUSTIN_TRAP, 0, NPC_AUSTIN_HUXWORTH_TRAP } });
        if (Creature* austin = ObjectAccessor::GetCreature(*me, _austinGuid))
            austin->GetMotionMaster()->MovePoint(0, AustinTrapCheckPosition, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);

        _scheduler.Schedule(2s, [this](TaskContext const& /*context*/)
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
            {
                player->KilledMonsterCredit(NPC_STALKING_PROWLER, me->GetGUID());
                StartPrivateConversation(player, CONVERSATION_STALKER_CAPTURED, { { ACTOR_AUSTIN_TRAP, 0, NPC_AUSTIN_HUXWORTH_TRAP } });
            }
        });

        // Austin walks back to the camp, the stalker breaks free and runs off fading
        _scheduler.Schedule(8s, [this](TaskContext const& /*context*/)
        {
            if (Creature* austin = ObjectAccessor::GetCreature(*me, _austinGuid))
                austin->GetMotionMaster()->MovePoint(POINT_AUSTIN_HOME, AustinTrapSpawnPosition, true, AustinTrapSpawnPosition.GetOrientation());
        });

        _scheduler.Schedule(8400ms, [this](TaskContext const& /*context*/)
        {
            me->RemoveAurasDueToSpell(SPELL_STALKER_FROZEN);
            me->GetMotionMaster()->MovePoint(0, StalkerFleePosition, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun, {}, MovementFadeObject(1s));
            me->DespawnOrUnsummon(2500ms);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
    ObjectGuid _playerGuid;
    ObjectGuid _austinGuid;
    ObjectGuid _trapGuid;
};

// 161666 - Austin Huxworth (hunter trainer at the camp, stable master)
struct npc_exiles_reach_austin_huxworth_pet_trainer : public ScriptedAI
{
    npc_exiles_reach_austin_huxworth_pet_trainer(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_AUSTIN_STABLE)
            return false;

        if (gossipListId == GOSSIP_OPTION_AUSTIN_STABLE)
        {
            // quest 60168: learning about the stables; the core opens the stable itself
            if (player->GetQuestStatus(QUEST_THE_ART_OF_TAMING) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonsterCredit(NPC_AUSTIN_HUXWORTH_TRAINER, me->GetGUID());
            return false;
        }

        if (gossipListId != GOSSIP_OPTION_AUSTIN_TAME_PET)
            return false;

        CloseGossipMenuFor(player);
        if (player->GetQuestStatus(QUEST_THE_ART_OF_TAMING) != QUEST_STATUS_INCOMPLETE || player->IsQuestObjectiveComplete(QUEST_THE_ART_OF_TAMING, OBJECTIVE_TAME_NEW_PET))
            return true;

        // the beast of the player's race leaps down to the camp and waits to be tamed
        player->CastSpell(player, SPELL_RACE_CHECK_FOR_CRITTER, true);
        TempSummon* pet = player->SummonCreature(GetHomePetEntry(player), TamingPetSpawnPosition, TEMPSUMMON_TIMED_DESPAWN, 2min, 0, 0, player->GetGUID());
        if (!pet)
            return true;

        pet->SetFaction(FACTION_CREATURE);
        pet->SetReactState(REACT_PASSIVE);
        pet->GetMotionMaster()->MoveJump(0, TamingPetLandPosition, 886ms);
        ObjectGuid petGuid = pet->GetGUID();
        _scheduler.Schedule(1200ms, [this, petGuid](TaskContext const& /*context*/)
        {
            if (Creature* pet = ObjectAccessor::GetCreature(*me, petGuid))
                pet->GetMotionMaster()->MovePoint(0, TamingPetWaitPosition);
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

// 160964 - Freed Expedition Member: thanks the player at the cocoon, then runs for the way out (retail 12.1.5 sniff)
struct npc_exiles_reach_freed_expedition_member : public ScriptedAI
{
    npc_exiles_reach_freed_expedition_member(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        me->SetReactState(REACT_PASSIVE);
        if (Player* player = summoner->ToPlayer())
        {
            me->SetFacingToObject(player);
            Talk(SAY_FREED_EXPEDITION_MEMBER, player);
        }

        _scheduler.Schedule(4200ms, [this](TaskContext const& /*context*/)
        {
            me->GetMotionMaster()->MovePoint(0, PitExitPosition, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    TaskScheduler _scheduler;
};

// 313324 - Freed Expedition Member (DNT): cast by opening a Thick Cocoon; the freed one stands where the cocoon was
class spell_exiles_reach_freed_expedition_member : public SpellScript
{
    void PreventSummon(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
    }

    void SummonAtCocoon()
    {
        Player* player = Object::ToPlayer(GetCaster());
        if (!player)
            return;

        GameObject* cocoon = player->FindNearestGameObject(GO_THICK_COCOON, 8.0f);
        Position position = cocoon ? cocoon->GetPosition() : player->GetPosition();
        player->SummonCreature(NPC_FREED_EXPEDITION_MEMBER, position, TEMPSUMMON_TIMED_DESPAWN, 7s, 0, 0, player->GetGUID());
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_exiles_reach_freed_expedition_member::PreventSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
        OnEffectHit += SpellEffectFn(spell_exiles_reach_freed_expedition_member::PreventSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
        AfterCast += SpellCastFn(spell_exiles_reach_freed_expedition_member::SummonAtCocoon);
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
        if (questId == QUEST_FINDING_THE_LOST_EXPEDITION)
        {
            // 12.x: no follower; Jaina sends the player ahead to Captain Garrick (personal text, she is phased out already)
            if (status == QUEST_STATUS_INCOMPLETE)
            {
                std::list<Creature*> jainas;
                GetCreatureListWithEntryInGrid(jainas, player, NPC_JAINA_BEACH, 50.0f);
                for (Creature* jaina : jainas)
                {
                    if (jaina->IsPrivateObject())
                        continue;

                    jaina->AI()->Talk(SAY_JAINA_SEARCH_SURVIVORS, player);
                    break;
                }

                StartPrivateConversationDelayed(player, CONVERSATION_LOST_EXPEDITION);
            }
            else if (status == QUEST_STATUS_COMPLETE)
                PhasingHandler::OnConditionChange(player);
            return;
        }

        // Cooking Meat: retail shows Alaria getting up. A personal copy of the standing Alaria (175031) replaces her for
        // the player and plays the stand-up anim kit, the real one takes over 2.3 s later
        if (questId == QUEST_COOKING_MEAT && status == QUEST_STATUS_REWARDED)
        {
            player->m_Events.AddEventAtOffset([player]()
            {
                if (!player->IsInWorld())
                    return;

                Creature* alaria = player->FindNearestCreatureWithOptions(60.0f, { .CreatureId = NPC_ALARIA_STANDING, .IgnorePhases = true });
                if (!alaria || !player->CanSeeOrDetect(alaria))
                    return;

                if (TempSummon* clone = alaria->SummonPersonalClone(alaria->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 2300ms, 0, 0, player))
                    clone->PlayOneShotAnimKitId(ANIM_KIT_ALARIA_STAND_UP);
            }, 0ms);
        }

        // Taming the Wilds: retail's tutorial puts the Call Pet and Pet Utility flyouts on the bar where the client
        // pushed the plain spells (Tame Beast, Call Pet 1)
        if (questId == QUEST_TAMING_THE_WILDS_ALLIANCE_HUNTER && status == QUEST_STATUS_REWARDED)
        {
            bool changed = false;
            for (auto const& [slot, button] : player->GetActionButtons())
            {
                if (button.uState == ACTIONBUTTON_DELETED || button.GetType() != ACTION_BUTTON_SPELL)
                    continue;

                if (button.GetAction() == SPELL_TAME_BEAST_HUNTER)
                    changed = player->AddActionButton(slot, FLYOUT_PET_UTILITY, ACTION_BUTTON_DROPDOWN) || changed;
                else if (button.GetAction() == SPELL_CALL_PET_1)
                    changed = player->AddActionButton(slot, FLYOUT_CALL_PET, ACTION_BUTTON_DROPDOWN) || changed;
            }

            if (changed)
                player->SendActionButtons(1);
            return;
        }

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
    new q55763_the_rescue_of_meredy_huntswell();
    new q55764_harpy_culling();
    new q55990_controlling_their_stones();
    new q59352_mage_knowledge();
    new q59354_the_art_of_taming_sheep();
    new q55991_the_end_of_the_beginning();
    new q55981_the_enemy_of_my_enemy();
    RegisterCreatureAI(npc_exiles_reach_ogre_camp_prisoner);
    RegisterCreatureAI(npc_exiles_reach_dancing_ogre);
    RegisterGameObjectAI(go_exiles_reach_catapult);
    RegisterCreatureAI(npc_exiles_reach_quartermaster_richter);
    RegisterCreatureAI(npc_exiles_reach_meredy_captive);
    RegisterCreatureAI(npc_exiles_reach_meredy_ambush_ally);
    RegisterCreatureAI(npc_exiles_reach_kelra_captive);
    RegisterCreatureAI(npc_exiles_reach_meredy_polymorph_training);
    RegisterSpellScript(spell_exiles_reach_polymorph_training);
    RegisterCreatureAI(npc_exiles_reach_meredy_ogre_transformation);
    RegisterCreatureAI(npc_exiles_reach_kalecgos_travel);
    RegisterCreatureAI(npc_exiles_reach_happy_hal);
    RegisterCreatureAI(npc_exiles_reach_freed_expedition_member);
    RegisterSpellScript(spell_exiles_reach_freed_expedition_member);
    new q59356_hunting_the_stalker();
    RegisterCreatureAI(npc_exiles_reach_freezing_trap);
    RegisterCreatureAI(npc_exiles_reach_austin_huxworth_trap);
    RegisterCreatureAI(npc_exiles_reach_stalking_prowler);
    RegisterCreatureAI(npc_exiles_reach_austin_huxworth_pet_trainer);
    new player_exiles_reach_human_mage();
}
