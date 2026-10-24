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

#ifndef DEF_HALLS_OF_VALOR_H_
#define DEF_HALLS_OF_VALOR_H_

#include "CreatureAIImpl.h"
#include "Position.h"

#define DataHeader "HallsOfValor"
#define HOVScriptName "instance_halls_of_valor"

uint32 const EncounterCount = 5;

enum HOVDataTypes
{
    // Encounters
    DATA_HYMDALL            = 0,
    DATA_HYRJA              = 1,
    DATA_FENRYR             = 2,
    DATA_GOD_KING_SKOVALD   = 3,
    DATA_ODYN               = 4,

    // Additional objects
    DATA_OLMYR_THE_ENLIGHTENED,
    DATA_SOLSTEN,
    DATA_AEGIS_OF_AGGRAMAR,
    DATA_RUNIC_BRAND_PURE,
    DATA_RUNIC_BRAND_RED,
    DATA_RUNIC_BRAND_YELLOW,
    DATA_RUNIC_BRAND_BLUE,
    DATA_RUNIC_BRAND_GREEN,

    // Additional data
    DATA_FENRYR_EVENT,          // trash Fenryr fled to his lair, boss Fenryr is available
    DATA_KINGS_EVENT,           // all four kings defeated, God-King Skovald is available
    DATA_SKOVALD_INTRO,         // Skovald intro finished
    DATA_RUNE_ACTIVATED,        // value = rune index (0-4)
    DATA_RUNE_DEACTIVATED       // value = rune index (0-4)
};

enum HOVCreatureIds
{
    // Hymdall
    BOSS_HYMDALL                    = 94960,
    NPC_STORM_DRAKE                 = 97788,
    NPC_STATIC_FIELD                = 97795,
    NPC_DANCING_BLADE               = 97960,

    // Hyrja
    BOSS_HYRJA                      = 95833,
    NPC_OLMYR_THE_ENLIGHTENED       = 97202,
    NPC_SOLSTEN                     = 97219,

    // Fenryr
    BOSS_FENRYR                     = 99868,
    NPC_FENRYR                      = 95674,
    NPC_EBONCLAW_PACKMATE           = 99922,

    // God-King Skovald
    BOSS_GOD_KING_SKOVALD           = 95675,
    NPC_KING_HALDOR                 = 95843,
    NPC_KING_BJORN                  = 97081,
    NPC_KING_RANULF                 = 97083,
    NPC_KING_TOR                    = 97084,
    NPC_AEGIS_OF_AGGRAMAR           = 98364,
    NPC_HONORED_ANCESTOR            = 101326,
    NPC_FLAME_OF_WOE                = 104822,

    // Odyn
    BOSS_ODYN                       = 95676,
    NPC_SPEAR_OF_LIGHT              = 100575,
    NPC_STORMFORGED_OBLITERATOR     = 102019,

    // Trash
    NPC_VALARJAR_THUNDERCALLER      = 95842
};

enum HOVGameObjectIds
{
    GO_HYMDALL_ENTRANCE_DOOR        = 245700,
    GO_HYMDALL_EXIT_DOOR            = 245701,
    GO_HYRJA_DOOR                   = 245702,
    GO_GATEWAY_OF_SPLENDOR          = 244664,
    GO_ODYN_DOOR                    = 246145,

    GO_RUNIC_BRAND_PURE             = 245698,
    GO_RUNIC_BRAND_RED              = 245697,
    GO_RUNIC_BRAND_YELLOW           = 245695,
    GO_RUNIC_BRAND_BLUE             = 245696,
    GO_RUNIC_BRAND_GREEN            = 245699
};

enum HOVSharedActions
{
    ACTION_DEFENDER_DEFEATED        = 1,
    ACTION_ODYN_INTRO               = 2,
    ACTION_AEGIS_DROPPED            = 3,
    ACTION_SKOVALD_INTRO            = 4
};

enum HOVSharedSpells
{
    SPELL_ODYNS_BLESSING            = 202160, // speed buff after Hyrja and Fenryr are defeated
    SPELL_REMOVE_ODYNS_BLESSING     = 202366,
    SPELL_AEGIS_OF_AGGRAMAR_SUMMON  = 193781
};

uint8 const MaxRunicBrands = 5;

Position const HOVHallCenterPos     = { 2429.13f, 528.55f, 748.99f, 0.0f };
Position const SkovaldArrivePos     = { 2411.03f, 528.72f, 748.99f, 3.14f };
Position const FenryrLairPos        = { 3100.44f, 3216.60f, 593.64f, 0.0f };

template <class AI, class T>
inline AI* GetHallsOfValorAI(T* obj)
{
    return GetInstanceAI<AI>(obj, HOVScriptName);
}

#define RegisterHallsOfValorCreatureAI(ai_name) RegisterCreatureAIWithFactory(ai_name, GetHallsOfValorAI)

#endif // DEF_HALLS_OF_VALOR_H_
