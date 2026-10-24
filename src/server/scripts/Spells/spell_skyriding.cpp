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
 * Scripts for skyriding (advanced flying) abilities.
 * Impulse vectors are taken from retail 12.0.5.67186 captures of SMSG_MOVE_ADD_IMPULSE.
 * Ported and reworked from lun-core (DekkCore) spell_dragonriding.cpp.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include <cmath>

enum SkyridingSpells
{
    SPELL_FLIGHT_STYLE_SKYRIDING    = 404464,
    SPELL_FLIGHT_STYLE_STEADY       = 404468
};

namespace
{
// Skyward Ascent pushes up and slightly forward along the facing direction
constexpr float SKYWARD_ASCENT_VERTICAL_IMPULSE   = 49.0f;
constexpr float SKYWARD_ASCENT_HORIZONTAL_IMPULSE = 12.25f;

// Surge Forward pushes along the aim direction (facing and pitch)
constexpr float SURGE_FORWARD_IMPULSE             = 18.0f;
}

// 374763 - Lift Off
class spell_skyriding_lift_off : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        // retail sends a straight vertical impulse of EffectBasePoints / 10
        GetCaster()->AddMoveImpulse(Position(0.0f, 0.0f, GetEffectInfo().BasePoints / 10.0f));
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_skyriding_lift_off::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 372610 - Skyward Ascent
class spell_skyriding_skyward_ascent : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        float orientation = caster->GetOrientation();
        caster->AddMoveImpulse(Position(std::cos(orientation) * SKYWARD_ASCENT_HORIZONTAL_IMPULSE, std::sin(orientation) * SKYWARD_ASCENT_HORIZONTAL_IMPULSE,
            SKYWARD_ASCENT_VERTICAL_IMPULSE));
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_skyriding_skyward_ascent::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 372608 - Surge Forward
class spell_skyriding_surge_forward : public SpellScript
{
    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        float orientation = caster->GetOrientation();
        float pitch = caster->m_movementInfo.pitch;
        caster->AddMoveImpulse(Position(std::cos(pitch) * std::cos(orientation) * SURGE_FORWARD_IMPULSE,
            std::cos(pitch) * std::sin(orientation) * SURGE_FORWARD_IMPULSE, std::sin(pitch) * SURGE_FORWARD_IMPULSE));
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_skyriding_surge_forward::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// 436854 - Switch Flight Style
class spell_skyriding_switch_flight_style : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_FLIGHT_STYLE_SKYRIDING, SPELL_FLIGHT_STYLE_STEADY });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (caster->HasAura(SPELL_FLIGHT_STYLE_STEADY))
        {
            caster->RemoveAurasDueToSpell(SPELL_FLIGHT_STYLE_STEADY);
            caster->CastSpell(caster, SPELL_FLIGHT_STYLE_SKYRIDING, TRIGGERED_FULL_MASK);
        }
        else
        {
            caster->RemoveAurasDueToSpell(SPELL_FLIGHT_STYLE_SKYRIDING);
            caster->CastSpell(caster, SPELL_FLIGHT_STYLE_STEADY, TRIGGERED_FULL_MASK);
        }
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_skyriding_switch_flight_style::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_skyriding_spell_scripts()
{
    RegisterSpellScript(spell_skyriding_lift_off);
    RegisterSpellScript(spell_skyriding_skyward_ascent);
    RegisterSpellScript(spell_skyriding_surge_forward);
    RegisterSpellScript(spell_skyriding_switch_flight_style);
}
