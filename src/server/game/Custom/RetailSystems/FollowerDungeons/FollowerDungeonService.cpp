/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "FollowerDungeonService.h"
#include "CompanionAI.h"
#include "Creature.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LfgDungeonRules.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PartyPackets.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "TemporarySummon.h"
#include <array>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace RetailSystems::FollowerDungeons
{
namespace
{
struct FollowerTemplate
{
    uint32 Entry;
    CompanionRole Role;
    uint8 Class;
    uint16 SpecId;
};

// retail follower dungeon NPCs; the first ones of each role are picked
constexpr std::array<FollowerTemplate, 5> FollowerTemplates =
{{
    { 209057, CompanionRole::Tank,   CLASS_PALADIN, 66  },  // Captain Garrick, Protection
    { 209072, CompanionRole::Healer, CLASS_DRUID,   105 },  // Crenna Earth-Daughter, Restoration
    { 209059, CompanionRole::Damage, CLASS_MAGE,    63  },  // Meredy Huntswell, Fire
    { 209065, CompanionRole::Damage, CLASS_HUNTER,  254 },  // Austin Huxworth, Marksmanship
    { 214390, CompanionRole::Damage, CLASS_SHAMAN,  262 }   // Shuja Grimaxe, Elemental
}};

constexpr uint8 TANKS_NEEDED = 1;
constexpr uint8 HEALERS_NEEDED = 1;
constexpr uint8 DAMAGE_NEEDED = 3;

struct PartyMember
{
    ObjectGuid Guid;
    FollowerTemplate const* Template;
    std::string Name;
};

struct FollowerParty
{
    uint32 MapId = 0;
    uint32 InstanceId = 0;
    std::vector<PartyMember> Members;
};

std::mutex PartiesLock;
std::unordered_map<ObjectGuid, FollowerParty> Parties;   // by group
std::unordered_set<ObjectGuid> PartyCreatures;

uint8 GetRoleMask(CompanionRole role)
{
    switch (role)
    {
        case CompanionRole::Tank:
            return lfg::PLAYER_ROLE_TANK;
        case CompanionRole::Healer:
            return lfg::PLAYER_ROLE_HEALER;
        default:
            return lfg::PLAYER_ROLE_DAMAGE;
    }
}

bool IsInFollowerDungeon(Player const* player, Group const* group)
{
    return group && group->isLFGGroup() && IsFollowerMap(player->GetMap())
        && IsFollowerDungeon(sLFGMgr->GetDungeon(group->GetGUID()));
}

// PartiesLock must be held
void ForgetParty(ObjectGuid const& groupGuid)
{
    auto itr = Parties.find(groupGuid);
    if (itr == Parties.end())
        return;

    for (PartyMember const& member : itr->second.Members)
        PartyCreatures.erase(member.Guid);
    Parties.erase(itr);
}

// PartiesLock must be held: forget the NPCs once no player of the group is left in their instance
bool ForgetPartyIfLeft(Group const* group, Player const* leaving)
{
    auto itr = Parties.find(group->GetGUID());
    if (itr == Parties.end())
        return false;

    for (GroupReference const& ref : group->GetMembers())
        if (Player const* member = ref.GetSource())
            if (member != leaving && member->IsInWorld() && member->GetMapId() == itr->second.MapId
                && member->GetInstanceId() == itr->second.InstanceId)
                return false;

    ForgetParty(group->GetGUID());
    return true;
}
}

void Initialize()
{
    for (FollowerTemplate const& follower : FollowerTemplates)
        if (!sObjectMgr->GetCreatureTemplate(follower.Entry))
            TC_LOG_ERROR("server.loading", "RetailSystems: follower dungeon NPC {} has no creature_template", follower.Entry);

    LogMatchRules();
}

void OnPlayerMapChanged(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
        return;

    Map* map = player->GetMap();
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(PartiesLock);
        if (!IsInFollowerDungeon(player, group))
            changed = ForgetPartyIfLeft(group, player);
        else
        {
            FollowerParty& party = Parties[group->GetGUID()];
            if (party.MapId != map->GetId() || party.InstanceId != map->GetInstanceId())
            {
                ForgetParty(group->GetGUID());
                FollowerParty& newParty = Parties[group->GetGUID()];
                newParty.MapId = map->GetId();
                newParty.InstanceId = map->GetInstanceId();
                changed = true;
            }

            FollowerParty& current = Parties[group->GetGUID()];

            // members whose creature is gone (instance reset, despawned) are replaced
            std::erase_if(current.Members, [&](PartyMember const& member)
            {
                Creature* creature = map->GetCreature(member.Guid);
                if (creature && creature->IsAlive())
                    return false;

                PartyCreatures.erase(member.Guid);
                changed = true;
                return true;
            });

            uint8 tanks = 0;
            uint8 healers = 0;
            uint8 damage = 0;
            for (Group::MemberSlot const& slot : group->GetMemberSlots())
            {
                if (slot.roles & lfg::PLAYER_ROLE_TANK)
                    ++tanks;
                else if (slot.roles & lfg::PLAYER_ROLE_HEALER)
                    ++healers;
                else
                    ++damage;
            }

            for (PartyMember const& member : current.Members)
            {
                switch (member.Template->Role)
                {
                    case CompanionRole::Tank: ++tanks; break;
                    case CompanionRole::Healer: ++healers; break;
                    default: ++damage; break;
                }
            }

            uint8 slot = uint8(current.Members.size());
            for (FollowerTemplate const& follower : FollowerTemplates)
            {
                uint8* count = follower.Role == CompanionRole::Tank ? &tanks : follower.Role == CompanionRole::Healer ? &healers : &damage;
                uint8 needed = follower.Role == CompanionRole::Tank ? TANKS_NEEDED : follower.Role == CompanionRole::Healer ? HEALERS_NEEDED : DAMAGE_NEEDED;
                if (*count >= needed)
                    continue;

                if (std::ranges::any_of(current.Members, [&](PartyMember const& member) { return member.Template == &follower; }))
                    continue;

                CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(follower.Entry);
                if (!creatureTemplate)
                    continue;

                Position position = player->GetNearPosition(3.0f, float(M_PI) / 2.0f + float(slot) * float(M_PI) / 4.0f);
                TempSummon* summon = player->SummonCreature(follower.Entry, position, TEMPSUMMON_MANUAL_DESPAWN);
                if (!summon)
                    continue;

                // the follower templates carry no content tuning: they take the player's level
                summon->SetLevel(player->GetLevel());
                summon->UpdateLevelDependantStats();
                summon->SetFullHealth();
                summon->SetFaction(player->GetFaction());
                summon->SetTreatAsRaidUnit(true);
                if (summon->AI())
                    summon->AI()->SetGUID(player->GetGUID(), DATA_COMPANION_LEADER);

                current.Members.push_back({ summon->GetGUID(), &follower, creatureTemplate->Name });
                PartyCreatures.insert(summon->GetGUID());
                ++*count;
                ++slot;
                changed = true;
            }
        }
    }

    if (changed)
        group->SendUpdate();
}

void OnPlayerLogout(Player* player)
{
    Group* group = player->GetGroup();
    if (!group)
        return;

    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(PartiesLock);
        changed = ForgetPartyIfLeft(group, player);
    }

    if (changed)
        group->SendUpdate();
}

void OnGroupDisband(Group const* group)
{
    std::lock_guard<std::mutex> lock(PartiesLock);
    ForgetParty(group->GetGUID());
}

bool IsPartyMember(Creature const* creature)
{
    std::lock_guard<std::mutex> lock(PartiesLock);
    return PartyCreatures.contains(creature->GetGUID());
}

void AppendPartyMembers(Group const& group, WorldPackets::Party::PartyUpdate& partyUpdate)
{
    std::lock_guard<std::mutex> lock(PartiesLock);
    auto itr = Parties.find(group.GetGUID());
    if (itr == Parties.end())
        return;

    for (PartyMember const& member : itr->second.Members)
    {
        WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
        info.GUID = member.Guid;
        info.Name = member.Name;
        info.Class = member.Template->Class;
        info.FactionGroup = partyUpdate.LeaderFactionGroup;
        info.Connected = true;
        info.RolesAssigned = GetRoleMask(member.Template->Role);
    }
}

bool FillMemberState(Player const* requester, ObjectGuid const& target, WorldPackets::Party::PartyMemberFullState& state)
{
    Group const* group = requester->GetGroup();
    if (!group)
        return false;

    FollowerTemplate const* follower = nullptr;
    {
        std::lock_guard<std::mutex> lock(PartiesLock);
        auto itr = Parties.find(group->GetGUID());
        if (itr == Parties.end())
            return false;

        for (PartyMember const& member : itr->second.Members)
            if (member.Guid == target)
                follower = member.Template;
    }

    if (!follower)
        return false;

    state.ForEnemy = false;
    state.MemberGuid = target;

    Creature const* creature = ObjectAccessor::GetCreature(*requester, target);
    if (!creature)
    {
        state.MemberStats.Status = MEMBER_STATUS_OFFLINE;
        return true;
    }

    WorldPackets::Party::PartyMemberStats& stats = state.MemberStats;
    stats.Status = MEMBER_STATUS_ONLINE;
    if (!creature->IsAlive())
        stats.Status |= MEMBER_STATUS_DEAD;

    stats.Level = creature->GetLevel();
    stats.CurrentHealth = creature->GetHealth();
    stats.MaxHealth = creature->GetMaxHealth();
    stats.PowerType = creature->GetPowerType();
    stats.CurrentPower = creature->GetPower(creature->GetPowerType());
    stats.MaxPower = creature->GetMaxPower(creature->GetPowerType());
    stats.ZoneID = creature->GetZoneId();
    stats.PositionX = int16(creature->GetPositionX());
    stats.PositionY = int16(creature->GetPositionY());
    stats.PositionZ = int16(creature->GetPositionZ());
    stats.SpecID = follower->SpecId;
    stats.PartyType[0] = requester->m_playerData->PartyType[0];
    stats.PartyType[1] = requester->m_playerData->PartyType[1];

    for (AuraApplication const* aurApp : creature->GetVisibleAuras())
    {
        WorldPackets::Party::PartyMemberAuraStates& aura = stats.Auras.emplace_back();
        aura.SpellID = aurApp->GetBase()->GetId();
        aura.ActiveFlags = aurApp->GetEffectMask();
        aura.Flags = aurApp->GetFlags();

        if (aurApp->GetFlags() & AFLAG_SCALABLE)
            for (AuraEffect const* aurEff : aurApp->GetBase()->GetAuraEffects())
                if (aurApp->HasEffect(aurEff->GetEffIndex()))
                    aura.Points.push_back(float(aurEff->GetAmount()));
    }

    PhasingHandler::FillPartyMemberPhase(&stats.Phases, creature->GetPhaseShift());
    return true;
}
}
