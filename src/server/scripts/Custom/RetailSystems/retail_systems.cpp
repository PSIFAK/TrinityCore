/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChallengeModeService.h"
#include "AreaPoi.h"
#include "BattlePayService.h"
#include "ChromieTime.h"
#include "Config.h"
#include "FollowerDungeonService.h"
#include "Player.h"
#include "RetailGameTables.h"
#include "ScriptMgr.h"
#include "WarbandGroups.h"
#include "WorldQuestService.h"
#include "WorldSession.h"

namespace
{
class RetailSystemsWorldScript final : public WorldScript
{
public:
    RetailSystemsWorldScript() : WorldScript("RetailSystemsWorldScript") { }

    void OnStartup() override
    {
        std::string clientDataDir = sConfigMgr->GetStringDefault("RetailSystems.ClientDataDir", "", true);
        if (clientDataDir.empty())
            clientDataDir = sConfigMgr->GetStringDefault("DataDir", "./");
        RetailSystems::DB2::LoadGameTables(clientDataDir);
        RetailSystems::BattlePay::Initialize();
        RetailSystems::ChallengeMode::Initialize();
        RetailSystems::WorldQuests::Initialize();
        RetailSystems::FollowerDungeons::Initialize();
        RetailSystems::AreaPoi::Load();
    }

    void OnUpdate(uint32 diff) override
    {
        RetailSystems::ChallengeMode::Update(diff);
        RetailSystems::WorldQuests::Update(diff);
    }
};

class RetailSystemsPlayerScript final : public PlayerScript
{
public:
    RetailSystemsPlayerScript() : PlayerScript("RetailSystemsPlayerScript") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        RetailSystems::ChromieTime::Restore(player);
        RetailSystems::ChallengeMode::RestoreKeystones(player);
        RetailSystems::WorldQuests::OnLogin(player);
    }

    void OnMapChanged(Player* player) override
    {
        RetailSystems::ChromieTime::RefreshConditionalFlags(player);
        RetailSystems::ChallengeMode::SendActiveRun(player);
        RetailSystems::FollowerDungeons::OnPlayerMapChanged(player);
    }

    void OnLogout(Player* player) override
    {
        RetailSystems::BattlePay::ClearPending(player->GetSession()->GetAccountId());
        RetailSystems::WorldQuests::OnLogout(player);
        RetailSystems::FollowerDungeons::OnPlayerLogout(player);
    }

    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        RetailSystems::WorldQuests::OnQuestStatusChange(player, questId);
    }

    void OnDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        RetailSystems::ChromieTime::RemoveCharacterState(guid.GetCounter());
        RetailSystems::ChallengeMode::RemoveCharacterState(guid.GetCounter());
        RetailSystems::WorldQuests::RemoveCharacterState(guid.GetCounter());
        RetailSystems::WarbandGroups::RemoveCharacter(guid);
    }
};

class RetailSystemsGroupScript final : public GroupScript
{
public:
    RetailSystemsGroupScript() : GroupScript("RetailSystemsGroupScript") { }

    void OnDisband(Group* group) override
    {
        RetailSystems::FollowerDungeons::OnGroupDisband(group);
    }
};

class RetailSystemsUnitScript final : public UnitScript
{
public:
    RetailSystemsUnitScript() : UnitScript("RetailSystemsUnitScript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        RetailSystems::ChallengeMode::ScaleDamage(attacker, victim, damage);
    }
};
}

void AddSC_follower_dungeon_npcs();

void AddRetailSystemsScripts()
{
    new RetailSystemsWorldScript();
    new RetailSystemsPlayerScript();
    new RetailSystemsGroupScript();
    new RetailSystemsUnitScript();
    AddSC_follower_dungeon_npcs();
}
