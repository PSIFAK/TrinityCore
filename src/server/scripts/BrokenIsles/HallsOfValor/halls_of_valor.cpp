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

/*
 * Notes:
 * - The four Valarjar kings are friendly gossip creatures in the world database; talking to one turns it hostile.
 *   Once all four are dead the instance script summons God-King Skovald.
 * - The original "Field of the Eternal Hunt" (101697) gossip teleport is not ported: the creature has no gossip flag
 *   in the world database.
 */

#include "Creature.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "halls_of_valor.h"

enum ValarjarKingSpells
{
    SPELL_SEVER             = 199652, // King Haldor
    SPELL_WICKED_DAGGER     = 199674, // King Bjorn
    SPELL_UNRULY_YELL       = 199726, // King Ranulf
    SPELL_CALL_ANCESTOR     = 200969  // King Tor
};

enum ValarjarKingEvents
{
    EVENT_KING_ABILITY = 1
};

// 95843 - King Haldor
// 97081 - King Bjorn
// 97083 - King Ranulf
// 97084 - King Tor
struct npc_hov_valarjar_king : public ScriptedAI
{
    npc_hov_valarjar_king(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        CloseGossipMenuFor(player);

        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        me->SetFaction(FACTION_MONSTER);
        me->SetImmuneToPC(false);
        me->SetReactState(REACT_AGGRESSIVE);
        AttackStart(player);
        return true;
    }

    void Reset() override
    {
        _events.Reset();
    }

    void JustReachedHome() override
    {
        // Wiped on this king: make it talkable again
        me->RestoreFaction();
        me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_KING_ABILITY, me->GetEntry() == NPC_KING_TOR ? 1s : 3s);
    }

    void CastKingAbility()
    {
        switch (me->GetEntry())
        {
            case NPC_KING_HALDOR:
                DoCastVictim(SPELL_SEVER);
                break;
            case NPC_KING_BJORN:
                DoCastVictim(SPELL_WICKED_DAGGER);
                break;
            case NPC_KING_RANULF:
                DoCastAOE(SPELL_UNRULY_YELL);
                break;
            case NPC_KING_TOR:
                DoCastSelf(SPELL_CALL_ANCESTOR);
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_KING_ABILITY:
                    CastKingAbility();
                    _events.Repeat(16s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    EventMap _events;
};

void AddSC_halls_of_valor()
{
    RegisterHallsOfValorCreatureAI(npc_hov_valarjar_king);
}
