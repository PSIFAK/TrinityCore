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

#include "Creature.h"
#include "DBCEnums.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "court_of_stars.h"

// Trash and event scripts of Court of Stars.
// Note: there is no creature_text for any Court of Stars creature in TDB, so the Talk() calls of the original script were dropped.

enum CourtOfStarsSpells
{
    // Arcane Beacon
    SPELL_DISABLE_BEACON            = 210253,

    // Duskwatch Sentry
    SPELL_SENTRY_ATTACK             = 209036,
    SPELL_SOUND_ALARM               = 210261, // summons Duskwatch Reinforcement (107073)

    // Signal Lantern / Nightborne Boat
    SPELL_SIGNAL_LANTERN            = 209524, // triggers 208631 (summon Nightborne Boat), effect 1 is a marker aura
    SPELL_RIDE_BOAT                 = 213507,
    SPELL_BOAT_TELEPORT             = 208703, // forces 208702 (teleport with spell_target_position)

    // Suspicious Noble
    SPELL_NOBLE_DISGUISE            = 213105,
    SPELL_MASQUERADE                = 213213,
    SPELL_RIGHTEOUS_INDIGNATION     = 213304,
    SPELL_RIGHTEOUS_INDIGNATION_END = 213233,
    SPELL_SPY_FOUND_CREDIT          = 219722,
    SPELL_SPY_ABILITY_1             = 214688,
    SPELL_SPY_ABILITY_2             = 214690,
    SPELL_SPY_ABILITY_3             = 214692
};

enum CourtOfStarsEvents
{
    EVENT_SPY_ABILITY_1 = 1,
    EVENT_SPY_ABILITY_2,
    EVENT_SPY_ABILITY_3
};

enum CourtOfStarsMisc
{
    POINT_BEACON                = 1,

    GOSSIP_MENU_SIGNAL_LANTERN  = 19515,
    GOSSIP_OPTION_SIGNAL        = 0
};

// 105729 - Signal Lantern
struct npc_cos_signal_lantern : public ScriptedAI
{
    npc_cos_signal_lantern(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
    {
        if (menuId != GOSSIP_MENU_SIGNAL_LANTERN || gossipListId != GOSSIP_OPTION_SIGNAL)
            return false;

        CloseGossipMenuFor(player);
        if (!player->HasAura(SPELL_SIGNAL_LANTERN))
            player->CastSpell(player, SPELL_SIGNAL_LANTERN);
        return true;
    }
};

// 105264 - Nightborne Boat
// The original script moved the boat along custom waypoint paths (9100402/9100403) that do not exist in TDB,
// so the boat now only takes the player aboard and then hands over to the teleport spell.
struct npc_cos_nightborne_boat : public ScriptedAI
{
    npc_cos_nightborne_boat(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        _summonerGUID = summoner->GetGUID();

        _scheduler.Schedule(1s, [this](TaskContext /*task*/)
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _summonerGUID))
                player->CastSpell(me, SPELL_RIDE_BOAT, true);
        });

        // Safety net: never leave the boat around if nobody boards it
        _scheduler.Schedule(30s, [this](TaskContext /*task*/)
        {
            me->DespawnOrUnsummon();
        });
    }

    void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
    {
        if (!apply || !passenger->IsPlayer())
            return;

        _scheduler.CancelAll();
        _scheduler.Schedule(3s, [this](TaskContext /*task*/)
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _summonerGUID))
                player->CastSpell(player, SPELL_BOAT_TELEPORT, true);

            me->DespawnOrUnsummon(3s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        _scheduler.Update(diff);
    }

private:
    ObjectGuid _summonerGUID;
    TaskScheduler _scheduler;
};

// 104251 - Duskwatch Sentry
struct npc_cos_duskwatch_sentry : public ScriptedAI
{
    npc_cos_duskwatch_sentry(Creature* creature) : ScriptedAI(creature), _alarmRaised(false) { }

    void Reset() override
    {
        _alarmRaised = false;
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        DoCast(who, SPELL_SENTRY_ATTACK);

        if (_alarmRaised)
            return;

        Creature* beacon = me->FindNearestCreature(NPC_ARCANE_BEACON, 100.0f);
        if (!beacon || beacon->HasAura(SPELL_BEACON_DISABLED))
            return;

        _alarmRaised = true;
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        me->GetMotionMaster()->MovePoint(POINT_BEACON, beacon->GetRandomNearPosition(7.0f));
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || id != POINT_BEACON)
            return;

        DoCastSelf(SPELL_SOUND_ALARM);
        me->SetReactState(REACT_AGGRESSIVE);
    }

private:
    bool _alarmRaised;
};

// 104245 - Arcane Beacon
struct npc_cos_arcane_beacon : public ScriptedAI
{
    npc_cos_arcane_beacon(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void JustAppeared() override
    {
        // TDB has no npcflag for the beacon; players disable it by interacting with it
        if (!me->HasAura(SPELL_BEACON_DISABLED))
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    }

    void Reset() override { }

    bool OnGossipHello(Player* player) override
    {
        CloseGossipMenuFor(player);
        if (!me->HasAura(SPELL_BEACON_DISABLED))
            player->CastSpell(me, SPELL_DISABLE_BEACON);
        return true;
    }

    void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id != SPELL_DISABLE_BEACON)
            return;

        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        DoCastSelf(SPELL_BEACON_DISABLED, true);
    }

    void UpdateAI(uint32 /*diff*/) override { }
};

// 107435 - Suspicious Noble
// The original script let the spy flee along custom waypoint paths (9100405-9100408) that do not exist in TDB,
// the revealed spy now attacks the player right away. Only players wearing the Masquerade disguise can question
// nobles; nothing in our data hands out that disguise yet (Ly'leth Lunastre 106468 is not spawned).
struct npc_cos_suspicious_noble : public ScriptedAI
{
    npc_cos_suspicious_noble(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()) { }

    void JustAppeared() override
    {
        ScriptedAI::JustAppeared();
        me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    }

    void Reset() override
    {
        _events.Reset();
    }

    bool OnGossipHello(Player* player) override
    {
        InitGossipMenuFor(player, 0);
        if (player->HasAura(SPELL_MASQUERADE) && !player->HasAura(SPELL_RIGHTEOUS_INDIGNATION))
            AddGossipItemFor(player, GossipOptionNpc::None, "We are here to help you!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, uint32 /*menuId*/, uint32 gossipListId) override
    {
        uint32 const action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        CloseGossipMenuFor(player);
        if (action != GOSSIP_ACTION_INFO_DEF + 1)
            return true;

        if (_instance->GetData(DATA_SPY_EVENT) == NOT_STARTED)
            _instance->SetData(DATA_SPY_EVENT, IN_PROGRESS);

        if (me->GetGUID() != _instance->GetGuidData(DATA_SPY_GUID))
        {
            player->CastSpell(player, SPELL_RIGHTEOUS_INDIGNATION, true);
            return true;
        }

        me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
        me->RemoveAurasDueToSpell(SPELL_NOBLE_DISGUISE);
        me->SetFaction(FACTION_MONSTER_2);
        me->SetImmuneToPC(false);
        me->SetReactState(REACT_AGGRESSIVE);
        AttackStart(player);
        return true;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_SPY_ABILITY_1, 3s);
        _events.ScheduleEvent(EVENT_SPY_ABILITY_2, 9s);
        _events.ScheduleEvent(EVENT_SPY_ABILITY_3, 16s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetGUID() != _instance->GetGuidData(DATA_SPY_GUID))
            return;

        _instance->SetData(DATA_SPY_EVENT, DONE);
        _instance->DoUpdateCriteria(CriteriaType::BeSpellTarget, SPELL_SPY_FOUND_CREDIT);
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
                case EVENT_SPY_ABILITY_1:
                    DoCastVictim(SPELL_SPY_ABILITY_1);
                    _events.Repeat(25s);
                    break;
                case EVENT_SPY_ABILITY_2:
                    DoCastVictim(SPELL_SPY_ABILITY_2);
                    _events.Repeat(25s);
                    break;
                case EVENT_SPY_ABILITY_3:
                    DoCastSelf(SPELL_SPY_ABILITY_3);
                    _events.Repeat(25s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }

private:
    InstanceScript* _instance;
    EventMap _events;
};

// 213304 - Righteous Indignation
class spell_cos_righteous_indignation : public AuraScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_MASQUERADE, SPELL_RIGHTEOUS_INDIGNATION_END });
    }

    void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/) const
    {
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
            return;

        Unit* target = GetTarget();
        target->RemoveAurasDueToSpell(SPELL_MASQUERADE);
        target->CastSpell(target, SPELL_RIGHTEOUS_INDIGNATION_END, true);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_cos_righteous_indignation::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_court_of_stars()
{
    RegisterCourtOfStarsCreatureAI(npc_cos_signal_lantern);
    RegisterCourtOfStarsCreatureAI(npc_cos_nightborne_boat);
    RegisterCourtOfStarsCreatureAI(npc_cos_duskwatch_sentry);
    RegisterCourtOfStarsCreatureAI(npc_cos_arcane_beacon);
    RegisterCourtOfStarsCreatureAI(npc_cos_suspicious_noble);

    RegisterSpellScript(spell_cos_righteous_indignation);
}
