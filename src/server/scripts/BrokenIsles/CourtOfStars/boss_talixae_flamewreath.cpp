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
#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "court_of_stars.h"

// Note: there is no creature_text for any Court of Stars creature in TDB, so the Talk() calls of the original script were dropped.
// The Fel Covenant guards and Felbound Enforcer are regular TDB spawns now (the original script summoned them), the
// guard bookkeeping (Bond of Flame removal, guards answering a dead enforcer) lives in the instance script.

enum TalixaeSpells
{
    SPELL_BURNING_INTENSITY     = 207906,
    SPELL_WITHERING_SOUL        = 208165,
    SPELL_INFERNAL_ERUPTION     = 207881,

    // Bond auras, applied by creature_template_addon. Their AreaTriggers (6655, 6368, 6654, 6653) have no
    // create properties in TDB yet, so the bonds currently have no gameplay effect.
    SPELL_BOND_OF_FLAME         = 209723,
    SPELL_BOND_OF_STRENGTH      = 207819, // Jazshariu
    SPELL_BOND_OF_CRUELTY       = 209717, // Baalgar the Watchful
    SPELL_BOND_OF_CUNNING       = 209712, // Imacu'tya

    // Guards
    SPELL_WHIRLING_BLADES       = 209378, // Imacu'tya
    SPELL_DISINTEGRATION_BEAM   = 207980, // Baalgar the Watchful
    SPELL_SHOCKWAVE             = 207979  // Jazshariu
};

enum TalixaeEvents
{
    EVENT_BURNING_INTENSITY = 1,
    EVENT_WITHERING_SOUL,
    EVENT_INFERNAL_ERUPTION,

    EVENT_GUARD_ABILITY
};

static constexpr uint32 TalixaeGuardData[3] =
{
    DATA_JAZSHARIU,
    DATA_BAALGAR_THE_WATCHFUL,
    DATA_IMACUTYA
};

// 104217 - Talixae Flamewreath
struct boss_talixae_flamewreath : public BossAI
{
    boss_talixae_flamewreath(Creature* creature) : BossAI(creature, DATA_TALIXAE_FLAMEWREATH) { }

    void UpdateBondOfFlame()
    {
        if (instance->GetData(DATA_TALIXAE_GUARDS_KILLED) >= 3)
            me->RemoveAurasDueToSpell(SPELL_BOND_OF_FLAME);
        else if (!me->HasAura(SPELL_BOND_OF_FLAME))
            DoCastSelf(SPELL_BOND_OF_FLAME, true);
    }

    void JustAppeared() override
    {
        BossAI::JustAppeared();
        UpdateBondOfFlame();
    }

    void Reset() override
    {
        _Reset();
        UpdateBondOfFlame();
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);

        for (uint32 guardData : TalixaeGuardData)
            if (Creature* guard = instance->GetCreature(guardData))
                if (guard->IsAlive() && !guard->IsInCombat() && guard->IsAIEnabled())
                    guard->AI()->AttackStart(who);

        events.ScheduleEvent(EVENT_BURNING_INTENSITY, 6s);
        events.ScheduleEvent(EVENT_WITHERING_SOUL, 12s);
        events.ScheduleEvent(EVENT_INFERNAL_ERUPTION, 20s);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        summons.DespawnAll();
        _EnterEvadeMode();
        _DespawnAtEvade();
    }

    void JustDied(Unit* /*killer*/) override
    {
        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        _JustDied();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BURNING_INTENSITY:
                    DoCastSelf(SPELL_BURNING_INTENSITY);
                    break;
                case EVENT_WITHERING_SOUL:
                    DoCastSelf(SPELL_WITHERING_SOUL);
                    events.Repeat(15s);
                    break;
                case EVENT_INFERNAL_ERUPTION:
                    DoCastSelf(SPELL_INFERNAL_ERUPTION);
                    events.Repeat(32s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }
    }
};

// 104273 - Jazshariu
// 104274 - Baalgar the Watchful
// 104275 - Imacu'tya
struct npc_talixae_fel_covenant_guard : public ScriptedAI
{
    npc_talixae_fel_covenant_guard(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
    {
        switch (me->GetEntry())
        {
            case NPC_JAZSHARIU:
                _bondSpellId = SPELL_BOND_OF_STRENGTH;
                _abilitySpellId = SPELL_SHOCKWAVE;
                _firstAbilityTimer = 7s;
                _abilityTimer = 11s;
                break;
            case NPC_BAALGAR_THE_WATCHFUL:
                _bondSpellId = SPELL_BOND_OF_CRUELTY;
                _abilitySpellId = SPELL_DISINTEGRATION_BEAM;
                _firstAbilityTimer = 3s;
                _abilityTimer = 13s;
                break;
            case NPC_IMACUTYA:
            default:
                _bondSpellId = SPELL_BOND_OF_CUNNING;
                _abilitySpellId = SPELL_WHIRLING_BLADES;
                _firstAbilityTimer = 3s;
                _abilityTimer = 17s;
                break;
        }
    }

    void Reset() override
    {
        _events.Reset();
        if (!me->HasAura(_bondSpellId))
            DoCastSelf(_bondSpellId, true);
    }

    void JustEngagedWith(Unit* who) override
    {
        _events.ScheduleEvent(EVENT_GUARD_ABILITY, _firstAbilityTimer);

        if (Creature* talixae = _instance->GetCreature(DATA_TALIXAE_FLAMEWREATH))
            if (talixae->IsAlive() && !talixae->IsInCombat() && talixae->IsAIEnabled() && me->IsWithinDistInMap(talixae, 50.0f))
                talixae->AI()->AttackStart(who);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (_events.ExecuteEvent() == EVENT_GUARD_ABILITY)
        {
            DoCastVictim(_abilitySpellId);
            _events.Repeat(_abilityTimer);
        }
    }

private:
    InstanceScript* _instance;
    EventMap _events;
    uint32 _bondSpellId;
    uint32 _abilitySpellId;
    Milliseconds _firstAbilityTimer;
    Milliseconds _abilityTimer;
};

void AddSC_boss_talixae_flamewreath()
{
    RegisterCourtOfStarsCreatureAI(boss_talixae_flamewreath);
    RegisterCourtOfStarsCreatureAI(npc_talixae_fel_covenant_guard);
}
