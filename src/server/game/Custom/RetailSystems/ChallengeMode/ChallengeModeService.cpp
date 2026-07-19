/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChallengeModeService.h"
#include "ChallengeModePackets.h"
#include "Config.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "GameEventMgr.h"
#include "GameObject.h"
#include "GameTime.h"
#include "Group.h"
#include "InstanceScenario.h"
#include "InstanceScript.h"
#include "Item.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "MiscPackets.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "RetailDB2Stores.h"
#include "RetailGameTables.h"
#include "Scenario.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RetailSystems::ChallengeMode
{
namespace
{
constexpr uint32 DefaultDeathPenaltyMs = 5 * IN_MILLISECONDS;
constexpr uint32 MaxPartySize = 5;
constexpr int32 AffixTyrannical = 9;
constexpr int32 AffixFortified = 10;
constexpr int32 AffixXalatathBetrayal = 147;
constexpr int32 AffixRiskyBet = 152;
constexpr int32 AffixLindormiGuidance = 165;

enum class RunState : uint8
{
    Countdown,
    Running
};

struct KeystoneData
{
    uint32 ChallengeID = 0;
    uint32 Level = 2;
    std::array<int32, 4> Affixes{};
};

struct MemberSnapshot
{
    ObjectGuid Guid;
    ObjectGuid BnetAccountGuid;
    ObjectGuid GuildGuid;
    std::string Name;
    int32 SpecializationID = 0;
    int8 RaceID = 0;
    int32 ItemLevel = 0;
};

struct ActiveRun
{
    uint32 MapID = 0;
    uint32 InstanceID = 0;
    uint32 ChallengeID = 0;
    uint32 Level = 2;
    std::array<int32, 4> Affixes{};
    ObjectGuid KeyOwnerGuid;
    ObjectGuid KeyItemGuid;
    std::vector<MemberSnapshot> Members;
    std::unordered_set<ObjectGuid> DeadPlayers;
    RunState State = RunState::Countdown;
    uint32 CountdownRemainingMs = 0;
    uint64 ElapsedMs = 0;
    uint32 DeathCount = 0;
    time_t StartDate = 0;
};

bool IsKeystone(Item const* item)
{
    ItemTemplate const* itemTemplate = item ? item->GetTemplate() : nullptr;
    return itemTemplate && itemTemplate->GetClass() == ITEM_CLASS_REAGENT && itemTemplate->GetSubClass() == ITEM_SUBCLASS_KEYSTONE;
}

uint64 EffectiveDuration(ActiveRun const& run)
{
    uint32 deathPenalty = DefaultDeathPenaltyMs;
    if (std::ranges::find(run.Affixes, AffixLindormiGuidance) != run.Affixes.end())
        deathPenalty = 0;
    else if (std::ranges::find(run.Affixes, AffixXalatathBetrayal) != run.Affixes.end() ||
        std::ranges::find(run.Affixes, AffixRiskyBet) != run.Affixes.end())
        deathPenalty = 15 * IN_MILLISECONDS;

    return run.ElapsedMs + uint64(run.DeathCount) * deathPenalty;
}

int32 GetCriteriaCountBonus(ActiveRun const& run)
{
    int32 bonus = 0;
    for (DB2::MapChallengeModeAffixCriteriaEntry const* criteria : DB2::MapChallengeModeAffixCriteriaStore)
    {
        if (criteria->MapChallengeModeID != run.ChallengeID || criteria->KeystoneAffixID <= 0 ||
            std::ranges::find(run.Affixes, criteria->KeystoneAffixID) == run.Affixes.end())
            continue;

        if (criteria->MinChallengeLevel > 0 && run.Level < uint32(criteria->MinChallengeLevel))
            continue;
        if (criteria->MaxChallengeLevel > 0 && run.Level > uint32(criteria->MaxChallengeLevel))
            continue;

        bonus += criteria->CriteriaCountBonus;
    }
    return bonus;
}

WorldPackets::MythicPlus::MythicPlusMember ToPacketMember(MemberSnapshot const& member)
{
    WorldPackets::MythicPlus::MythicPlusMember packetMember;
    packetMember.BnetAccountGUID = member.BnetAccountGuid;
    packetMember.GUID = member.Guid;
    packetMember.GuildGUID = member.GuildGuid;
    packetMember.NativeRealmAddress = GetVirtualRealmAddress();
    packetMember.VirtualRealmAddress = GetVirtualRealmAddress();
    packetMember.ChrSpecializationID = member.SpecializationID;
    packetMember.RaceID = member.RaceID;
    packetMember.ItemLevel = member.ItemLevel;
    return packetMember;
}

class Service
{
public:
    static Service& Instance()
    {
        static Service instance;
        return instance;
    }

    void Initialize()
    {
        RefreshSeasonData();
        RefreshAffixes(true);

        TC_LOG_INFO("server.loading", "RetailSystems: Mythic+ display season {}, mythic season {}, affixes [{}, {}, {}, {}]",
            _displaySeason, _mythicSeason, _currentAffixes[0], _currentAffixes[1], _currentAffixes[2], _currentAffixes[3]);
    }

    bool Start(Player* player, uint8 bag, uint32 slot, ObjectGuid const& receptacleGuid)
    {
        if (!player || slot > std::numeric_limits<uint8>::max())
            return false;

        if (!player->GetGameObjectIfCanInteractWith(receptacleGuid, GAMEOBJECT_TYPE_KEYSTONE_RECEPTACLE))
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
            return false;
        }

        Map* map = player->GetMap();
        InstanceMap* instanceMap = map ? map->ToInstanceMap() : nullptr;
        if (!instanceMap || !map->IsDungeon() || !map->GetInstanceId())
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
            return false;
        }

        if (FindRun(map))
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_IN_PROGRESS);
            return false;
        }

        if (!player->IsAlive() || player->IsInCombat())
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_IN_PROGRESS);
            return false;
        }

        if (Group* group = player->GetGroup())
        {
            if (!group->IsLeader(player->GetGUID()))
            {
                SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
                return false;
            }
        }

        if (map->GetPlayersCountExceptGMs() > MaxPartySize)
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_FULL);
            return false;
        }

        std::vector<MemberSnapshot> members;
        bool partyIsReady = true;
        Group const* group = player->GetGroup();
        map->DoOnPlayers([&](Player* member)
        {
            if (member->IsGameMaster())
                return;

            if (!member->IsAlive() || member->IsInCombat() || (group && member->GetGroup() != group) || (!group && member != player))
            {
                partyIsReady = false;
                return;
            }

            MemberSnapshot& snapshot = members.emplace_back();
            snapshot.Guid = member->GetGUID();
            snapshot.BnetAccountGuid = member->GetSession()->GetBattlenetAccountGUID();
            snapshot.Name = member->GetName();
            snapshot.SpecializationID = AsUnderlyingType(member->GetPrimarySpecialization());
            snapshot.RaceID = int8(member->GetRace());
            snapshot.ItemLevel = int32(member->GetAverageItemLevel());
            if (ObjectGuid::LowType guildId = member->GetGuildId())
                snapshot.GuildGuid = ObjectGuid::Create<HighGuid::Guild>(guildId);
        });

        if (!partyIsReady || members.empty() || members.size() > MaxPartySize)
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_IN_PROGRESS);
            return false;
        }

        InstanceScript* instanceScript = instanceMap->GetInstanceScript();
        InstanceScenario* scenario = instanceMap->GetInstanceScenario();
        if (!instanceScript || instanceScript->IsEncounterInProgress())
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_IN_PROGRESS);
            return false;
        }

        for (uint32 boss = 0; boss < instanceScript->GetEncounterCount(); ++boss)
        {
            EncounterState state = instanceScript->GetBossState(boss);
            if (state == DONE || state == IN_PROGRESS)
            {
                SendError(player, GameError::ERR_CHALLENGE_MODE_ALREADY_COMPLETE);
                return false;
            }
        }

        if (!player->IsValidPos(bag, uint8(slot), true))
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
            return false;
        }

        Item* keystone = player->GetItemByPos(bag, uint8(slot));
        if (!IsKeystone(keystone))
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
            return false;
        }

        RefreshAffixes(false);
        KeystoneData key = ReadKeystone(keystone);
        NormalizeKeystone(key);
        ApplyKeystone(player, keystone, key, true);

        MapChallengeModeEntry const* challenge = sMapChallengeModeStore.LookupEntry(key.ChallengeID);
        if (!challenge || challenge->MapID != map->GetId())
        {
            SendError(player, GameError::ERR_CHALLENGE_MODE_INCORRECT_KEYSTONE);
            return false;
        }

        std::shared_ptr<ActiveRun> run = std::make_shared<ActiveRun>();
        run->MapID = map->GetId();
        run->InstanceID = map->GetInstanceId();
        run->ChallengeID = key.ChallengeID;
        run->Level = key.Level;
        run->Affixes = key.Affixes;
        run->KeyOwnerGuid = player->GetGUID();
        run->KeyItemGuid = keystone->GetGUID();
        run->Members = std::move(members);
        run->CountdownRemainingMs = uint32(std::clamp(sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.CountdownSeconds", 10), 0, 60)) * IN_MILLISECONDS;
        if (!run->CountdownRemainingMs)
        {
            run->State = RunState::Running;
            run->StartDate = GameTime::GetGameTime();
        }

        {
            std::lock_guard lock(_runsLock);
            _runs.emplace(run->InstanceID, run);
        }

        if (scenario)
        {
            scenario->Reset();
            map->DoOnPlayers([scenario](Player* member) { scenario->SendScenarioState(member); });
        }

        WorldPackets::RetailChallengeMode::Reset reset;
        reset.MapID = run->MapID;
        map->SendToPlayers(reset.Write());
        BroadcastStart(map, *run);

        if (run->State == RunState::Countdown)
        {
            WorldPackets::Misc::StartTimer timer;
            timer.TotalTime = Seconds(run->CountdownRemainingMs / IN_MILLISECONDS);
            timer.TimeLeft = Seconds(run->CountdownRemainingMs / IN_MILLISECONDS);
            timer.Type = CountdownTimerType::ChallengeMode;
            map->SendToPlayers(timer.Write());
        }
        else
            BroadcastElapsed(map, *run);

        TC_LOG_INFO("entities.player", "RetailSystems: player {} started Mythic+ challenge {} level {} in instance {}",
            player->GetGUID().ToString(), run->ChallengeID, run->Level, run->InstanceID);
        return true;
    }

    void Reset(Player* player, bool cheat)
    {
        if (!player || !player->GetMap())
            return;

        std::shared_ptr<ActiveRun> run = FindRun(player->GetMap());
        if (!run)
            return;

        if (cheat && !player->IsGameMaster())
            return;

        Group* group = player->GetGroup();
        if (!cheat && player->GetGUID() != run->KeyOwnerGuid && (!group || !group->IsLeader(player->GetGUID())))
            return;

        Map* map = player->GetMap();
        if (!cheat)
            DepleteKeystone(*run);

        {
            std::lock_guard lock(_runsLock);
            _runs.erase(run->InstanceID);
        }

        if (InstanceScenario* scenario = map->ToInstanceMap()->GetInstanceScenario())
            scenario->Reset();

        WorldPackets::RetailChallengeMode::ElapsedTimerStop stop;
        map->SendToPlayers(stop.Write());
        WorldPackets::RetailChallengeMode::Reset reset;
        reset.MapID = map->GetId();
        map->SendToPlayers(reset.Write());
    }

    void Update(uint32 diff)
    {
        std::vector<std::shared_ptr<ActiveRun>> runs;
        {
            std::lock_guard lock(_runsLock);
            runs.reserve(_runs.size());
            for (auto const& [instanceId, run] : _runs)
                runs.push_back(run);
        }

        for (std::shared_ptr<ActiveRun> const& run : runs)
        {
            Map* map = sMapMgr->FindMap(run->MapID, run->InstanceID);
            if (!map)
            {
                std::lock_guard lock(_runsLock);
                _runs.erase(run->InstanceID);
                continue;
            }

            if (run->State == RunState::Countdown)
            {
                if (diff < run->CountdownRemainingMs)
                {
                    run->CountdownRemainingMs -= diff;
                    continue;
                }

                run->CountdownRemainingMs = 0;
                run->State = RunState::Running;
                run->StartDate = GameTime::GetGameTime();
                BroadcastElapsed(map, *run);
                continue;
            }

            run->ElapsedMs += diff;
            CheckDeaths(map, *run);

            InstanceMap* instanceMap = map->ToInstanceMap();
            InstanceScenario* scenario = instanceMap ? instanceMap->GetInstanceScenario() : nullptr;
            bool isComplete = scenario && scenario->IsComplete();

            if (!isComplete)
            {
                if (InstanceScript* instanceScript = instanceMap ? instanceMap->GetInstanceScript() : nullptr;
                    instanceScript && instanceScript->GetEncounterCount())
                {
                    isComplete = true;
                    for (uint32 boss = 0; boss < instanceScript->GetEncounterCount(); ++boss)
                    {
                        if (instanceScript->GetBossState(boss) != DONE)
                        {
                            isComplete = false;
                            break;
                        }
                    }
                }
            }

            if (isComplete)
                Complete(map, run);
        }
    }

    void RestoreKeystones(Player* player)
    {
        if (!player)
            return;

        std::unordered_map<uint64, KeystoneData> savedKeys;
        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT `item_guid`, `map_challenge_mode_id`, `level`, `affix_1`, `affix_2`, `affix_3`, `affix_4` "
            "FROM `retail_mythic_keystone` WHERE `owner_guid` = {}", player->GetGUID().GetCounter()))
        {
            do
            {
                Field* fields = result->Fetch();
                KeystoneData& key = savedKeys[fields[0].GetUInt64()];
                key.ChallengeID = fields[1].GetUInt32();
                key.Level = fields[2].GetUInt32();
                for (uint8 i = 0; i < key.Affixes.size(); ++i)
                    key.Affixes[i] = fields[3 + i].GetInt32();
            } while (result->NextRow());
        }

        RefreshAffixes(false);
        player->ForEachItem(ItemSearchLocation::Everywhere, [&](Item* item)
        {
            if (!IsKeystone(item))
                return ItemSearchCallbackResult::Continue;

            auto saved = savedKeys.find(item->GetGUID().GetCounter());
            KeystoneData key = saved != savedKeys.end() ? saved->second : ReadKeystone(item);
            NormalizeKeystone(key);
            ApplyKeystone(player, item, key, true);
            return ItemSearchCallbackResult::Continue;
        });
    }

    void RemoveCharacterState(uint64 characterGuid)
    {
        CharacterDatabase.PExecute("DELETE FROM `retail_mythic_keystone` WHERE `owner_guid` = {}", characterGuid);
    }

    void SendActiveRun(Player* player)
    {
        if (!player || !player->GetMap())
            return;

        std::shared_ptr<ActiveRun> run = FindRun(player->GetMap());
        if (!run)
            return;

        SendStart(player, *run);
        if (run->State == RunState::Countdown)
        {
            WorldPackets::Misc::StartTimer timer;
            timer.TotalTime = Seconds(run->CountdownRemainingMs / IN_MILLISECONDS);
            timer.TimeLeft = Seconds(run->CountdownRemainingMs / IN_MILLISECONDS);
            timer.Type = CountdownTimerType::ChallengeMode;
            player->SendDirectMessage(timer.Write());
        }
        else
        {
            WorldPackets::RetailChallengeMode::ElapsedTimerStart elapsed;
            elapsed.CurrentDuration = EffectiveDuration(*run) / IN_MILLISECONDS;
            player->SendDirectMessage(elapsed.Write());
        }
    }

    void SendCurrentAffixes(Player* player)
    {
        if (!player)
            return;

        RefreshAffixes(false);
        std::array<int32, 4> affixes = _currentAffixes;
        if (std::shared_ptr<ActiveRun> run = FindRun(player->GetMap()))
            affixes = run->Affixes;

        WorldPackets::RetailChallengeMode::CurrentAffixes response;
        for (int32 affix : affixes)
            if (affix > 0)
                response.Affixes.push_back({ affix, 0 });
        player->SendDirectMessage(response.Write());
    }

    void SendSeasonData(Player* player)
    {
        if (!player)
            return;

        RefreshSeasonData();
        WorldPackets::RetailChallengeMode::SeasonData response;
        response.IsMythicPlusActive = _mythicSeason != 0 && _displaySeason != 0;
        player->SendDirectMessage(response.Write());
    }

    void SendMapStats(Player* player)
    {
        if (!player)
            return;

        RefreshSeasonData();
        WorldPackets::RetailChallengeMode::AllMapStats response;
        response.Season = int32(_displaySeason);
        if (MythicPlusSeasonEntry const* season = sMythicPlusSeasonStore.LookupEntry(_mythicSeason))
            response.Subseason = season->MilestoneSeason;

        int32 historyLimit = std::clamp(sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.MapHistoryLimit", 100, true), 1, 100);
        QueryResult result = CharacterDatabase.PQuery(
            "SELECT r.`id`, r.`challenge_id`, r.`level`, r.`duration_ms`, r.`start_time`, r.`completion_time`, "
            "r.`display_season_id`, r.`score`, r.`affix_1`, r.`affix_2`, r.`affix_3`, r.`affix_4`, "
            "rm.`member_guid`, rm.`guild_id`, rm.`specialization_id`, rm.`race_id`, rm.`item_level` FROM ("
            "SELECT runs.* FROM `retail_mythic_run` runs "
            "INNER JOIN `retail_mythic_run_member` mine ON mine.`run_id` = runs.`id` AND mine.`member_guid` = {} "
            "WHERE runs.`display_season_id` = {} ORDER BY runs.`completion_time` DESC, runs.`id` LIMIT {}) r "
            "INNER JOIN `retail_mythic_run_member` rm ON rm.`run_id` = r.`id` "
            "ORDER BY r.`completion_time` DESC, r.`id`, rm.`member_guid`",
            player->GetGUID().GetCounter(), _displaySeason, historyLimit);

        uint64 currentRunId = 0;
        WorldPackets::MythicPlus::MythicPlusRun* currentRun = nullptr;
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint64 runId = fields[0].GetUInt64();
                if (!currentRun || runId != currentRunId)
                {
                    currentRunId = runId;
                    currentRun = &response.Runs.emplace_back();
                    currentRun->MapChallengeModeID = fields[1].GetInt32();
                    currentRun->Level = fields[2].GetUInt32();
                    currentRun->DurationMs = fields[3].GetInt32();
                    currentRun->StartDate = time_t(fields[4].GetUInt64());
                    currentRun->CompletionDate = time_t(fields[5].GetUInt64());
                    currentRun->Season = fields[6].GetInt32();
                    currentRun->RunScore = fields[7].GetFloat();
                    for (uint8 i = 0; i < currentRun->KeystoneAffixIDs.size(); ++i)
                        currentRun->KeystoneAffixIDs[i] = fields[8 + i].GetInt32();
                    currentRun->Completed = true;
                }

                WorldPackets::MythicPlus::MythicPlusMember& member = currentRun->Members.emplace_back();
                member.GUID = ObjectGuid::Create<HighGuid::Player>(fields[12].GetUInt64());
                if (uint64 guildId = fields[13].GetUInt64())
                    member.GuildGUID = ObjectGuid::Create<HighGuid::Guild>(guildId);
                member.NativeRealmAddress = GetVirtualRealmAddress();
                member.VirtualRealmAddress = GetVirtualRealmAddress();
                member.ChrSpecializationID = fields[14].GetInt32();
                member.RaceID = fields[15].GetInt8();
                member.ItemLevel = fields[16].GetInt32();
            } while (result->NextRow());
        }

        player->SendDirectMessage(response.Write());
    }

    void SendLeaders(Player* player, uint32 mapId, uint32 challengeId, uint64 /*lastGuildUpdate*/, uint64 /*lastRealmUpdate*/)
    {
        if (!player)
            return;

        WorldPackets::RetailChallengeMode::LeadersResult response;
        response.MapID = mapId;
        response.ChallengeID = challengeId;
        uint64 updateTime = uint64(GameTime::GetGameTime());
        response.LastGuildUpdate = updateTime;
        response.LastRealmUpdate = updateTime;
        if (uint64 guildId = player->GetGuildId())
            response.GuildLeaders = LoadLeaders(mapId, challengeId, guildId);
        response.RealmLeaders = LoadLeaders(mapId, challengeId, 0);
        player->SendDirectMessage(response.Write());
    }

    bool IsActive(Map const* map) const
    {
        return bool(FindRun(map));
    }

    uint32 GetLevel(Map const* map) const
    {
        if (std::shared_ptr<ActiveRun> run = FindRun(map))
            return run->Level;
        return 0;
    }

    bool HasAffix(Map const* map, int32 affix) const
    {
        if (std::shared_ptr<ActiveRun> run = FindRun(map))
            return std::ranges::find(run->Affixes, affix) != run->Affixes.end();
        return false;
    }

private:
    std::vector<WorldPackets::RetailChallengeMode::LeaderboardAttempt> LoadLeaders(
        uint32 mapId, uint32 challengeId, uint64 guildId) const
    {
        int32 limit = std::clamp(sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.LeaderboardLimit", 100, true), 1, 100);
        int32 guildMembersRequired = std::clamp(
            sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.GuildMembersRequired", 3, true), 1, int32(MaxPartySize));

        QueryResult result;
        if (guildId)
        {
            result = CharacterDatabase.PQuery(
                "SELECT ranked.`id`, ranked.`duration_ms`, ranked.`completion_time`, ranked.`level`, "
                "m.`member_guid`, m.`specialization_id` FROM ("
                "SELECT r.`id`, r.`duration_ms`, r.`completion_time`, r.`level` FROM `retail_mythic_run` r "
                "WHERE r.`map_id` = {} AND r.`challenge_id` = {} AND r.`display_season_id` = {} "
                "AND (SELECT COUNT(*) FROM `retail_mythic_run_member` gm "
                "WHERE gm.`run_id` = r.`id` AND gm.`guild_id` = {}) >= {} "
                "ORDER BY r.`level` DESC, r.`duration_ms`, r.`completion_time`, r.`id` LIMIT {}) ranked "
                "INNER JOIN `retail_mythic_run_member` m ON m.`run_id` = ranked.`id` "
                "ORDER BY ranked.`level` DESC, ranked.`duration_ms`, ranked.`completion_time`, ranked.`id`, m.`member_guid`",
                mapId, challengeId, _displaySeason, guildId, guildMembersRequired, limit);
        }
        else
        {
            result = CharacterDatabase.PQuery(
                "SELECT ranked.`id`, ranked.`duration_ms`, ranked.`completion_time`, ranked.`level`, "
                "m.`member_guid`, m.`specialization_id` FROM ("
                "SELECT r.`id`, r.`duration_ms`, r.`completion_time`, r.`level` FROM `retail_mythic_run` r "
                "WHERE r.`map_id` = {} AND r.`challenge_id` = {} AND r.`display_season_id` = {} "
                "ORDER BY r.`level` DESC, r.`duration_ms`, r.`completion_time`, r.`id` LIMIT {}) ranked "
                "INNER JOIN `retail_mythic_run_member` m ON m.`run_id` = ranked.`id` "
                "ORDER BY ranked.`level` DESC, ranked.`duration_ms`, ranked.`completion_time`, ranked.`id`, m.`member_guid`",
                mapId, challengeId, _displaySeason, limit);
        }

        std::vector<WorldPackets::RetailChallengeMode::LeaderboardAttempt> attempts;
        uint64 currentRunId = 0;
        WorldPackets::RetailChallengeMode::LeaderboardAttempt* currentAttempt = nullptr;
        if (!result)
            return attempts;

        do
        {
            Field* fields = result->Fetch();
            uint64 runId = fields[0].GetUInt64();
            if (!currentAttempt || currentRunId != runId)
            {
                currentRunId = runId;
                currentAttempt = &attempts.emplace_back();
                currentAttempt->InstanceRealmAddress = GetVirtualRealmAddress();
                currentAttempt->AttemptID = uint32(runId);
                currentAttempt->CompletionTime = uint32(std::min<uint64>(
                    fields[1].GetUInt64(), std::numeric_limits<uint32>::max()));
                currentAttempt->CompletionDate.SetUtcTimeFromUnixTime(time_t(fields[2].GetUInt64()));
                currentAttempt->MedalEarned = fields[3].GetUInt32();
            }

            WorldPackets::RetailChallengeMode::LeaderboardMember& member = currentAttempt->Members.emplace_back();
            member.VirtualRealmAddress = GetVirtualRealmAddress();
            member.NativeRealmAddress = GetVirtualRealmAddress();
            member.GUID = ObjectGuid::Create<HighGuid::Player>(fields[4].GetUInt64());
            member.SpecializationID = fields[5].GetUInt32();
        } while (result->NextRow());

        return attempts;
    }

    void RefreshSeasonData()
    {
        uint32 displaySeason = 0;
        for (DB2::MythicPlusSeasonTrackedMapEntry const* trackedMap : DB2::MythicPlusSeasonTrackedMapStore)
            displaySeason = std::max(displaySeason, trackedMap->DisplaySeasonID);

        int32 displayOverride = sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.DisplaySeason", 0, true);
        _displaySeason = displayOverride > 0 ? uint32(displayOverride) : displaySeason;

        uint32 selectedSeason = 0;
        uint32 fallbackSeason = 0;
        bool fallbackHasRewards = false;
        int32 bestExpansion = std::numeric_limits<int32>::min();
        for (MythicPlusSeasonEntry const* season : sMythicPlusSeasonStore)
        {
            if (season->StartTimeEvent > 0 && season->StartTimeEvent <= std::numeric_limits<uint16>::max()
                && sGameEventMgr->IsActiveEvent(uint16(season->StartTimeEvent)))
            {
                selectedSeason = std::max(selectedSeason, season->ID);
                continue;
            }

            bool hasRewards = std::ranges::any_of(DB2::MythicPlusSeasonRewardLevelsStore,
                [season](DB2::MythicPlusSeasonRewardLevelsEntry const* reward)
                {
                    return reward->MythicPlusSeasonID == season->ID;
                });
            if (season->ExpansionLevel > bestExpansion ||
                (season->ExpansionLevel == bestExpansion && hasRewards != fallbackHasRewards && hasRewards) ||
                (season->ExpansionLevel == bestExpansion && hasRewards == fallbackHasRewards && season->ID > fallbackSeason))
            {
                bestExpansion = season->ExpansionLevel;
                fallbackSeason = season->ID;
                fallbackHasRewards = hasRewards;
            }
        }

        _mythicSeason = selectedSeason ? selectedSeason : fallbackSeason;

        int32 seasonOverride = sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.MythicSeason", 0, true);
        if (seasonOverride > 0 && sMythicPlusSeasonStore.LookupEntry(uint32(seasonOverride)))
            _mythicSeason = uint32(seasonOverride);
    }

    void RefreshAffixes(bool force)
    {
        uint64 week = uint64(GameTime::GetGameTime()) / WEEK;
        if (!force && week == _affixWeek && std::ranges::any_of(_currentAffixes, [](int32 affix) { return affix > 0; }))
            return;

        RefreshSeasonData();
        _affixWeek = week;
        _currentAffixes.fill(0);

        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT `affix_1`, `affix_2`, `affix_3`, `affix_4` FROM `retail_mythic_affix_rotation` "
            "WHERE `display_season_id` = {} AND `week_start` <= {} ORDER BY `week_start` DESC LIMIT 1",
            _displaySeason, uint64(GameTime::GetGameTime())))
        {
            Field* fields = result->Fetch();
            for (uint8 i = 0; i < _currentAffixes.size(); ++i)
                if (int32 affix = fields[i].GetInt32(); affix > 0 && sKeystoneAffixStore.LookupEntry(uint32(affix)))
                    _currentAffixes[i] = affix;
        }

        if (std::ranges::any_of(_currentAffixes, [](int32 affix) { return affix > 0; }))
            return;

        std::vector<int32> trackedAffixes;
        for (DB2::MythicPlusSeasonTrackedAffixEntry const* trackedAffix : DB2::MythicPlusSeasonTrackedAffixStore)
            if (trackedAffix->DisplaySeasonID == _displaySeason && trackedAffix->KeystoneAffixID > 0
                && sKeystoneAffixStore.LookupEntry(uint32(trackedAffix->KeystoneAffixID)))
                trackedAffixes.push_back(trackedAffix->KeystoneAffixID);

        int32 majorAffix = week % 2 ? 9 : 10;
        if (std::ranges::find(trackedAffixes, majorAffix) == trackedAffixes.end())
            majorAffix = trackedAffixes.empty() ? 0 : trackedAffixes.front();
        _currentAffixes[0] = majorAffix;

        std::erase(trackedAffixes, AffixTyrannical);
        std::erase(trackedAffixes, AffixFortified);
        std::erase(trackedAffixes, AffixXalatathBetrayal);
        std::erase(trackedAffixes, AffixLindormiGuidance);
        if (!trackedAffixes.empty())
            _currentAffixes[1] = trackedAffixes[week % trackedAffixes.size()];
    }

    std::vector<uint32> GetTrackedChallenges() const
    {
        std::vector<uint32> challenges;
        for (DB2::MythicPlusSeasonTrackedMapEntry const* trackedMap : DB2::MythicPlusSeasonTrackedMapStore)
            if (trackedMap->DisplaySeasonID == _displaySeason && trackedMap->MapChallengeModeID > 0
                && sMapChallengeModeStore.LookupEntry(uint32(trackedMap->MapChallengeModeID)))
                challenges.push_back(uint32(trackedMap->MapChallengeModeID));
        return challenges;
    }

    uint32 SelectChallenge(uint32 previous = 0) const
    {
        std::vector<uint32> challenges = GetTrackedChallenges();
        if (challenges.size() > 1)
            std::erase(challenges, previous);
        if (challenges.empty())
            return previous;
        return challenges[urand(0, uint32(challenges.size() - 1))];
    }

    KeystoneData ReadKeystone(Item const* item) const
    {
        KeystoneData key;
        key.ChallengeID = item->GetModifier(ITEM_MODIFIER_CHALLENGE_MAP_CHALLENGE_MODE_ID);
        key.Level = item->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL);
        key.Affixes[0] = int32(item->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_1));
        key.Affixes[1] = int32(item->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_2));
        key.Affixes[2] = int32(item->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_3));
        key.Affixes[3] = int32(item->GetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_4));
        return key;
    }

    void NormalizeKeystone(KeystoneData& key) const
    {
        if (!sMapChallengeModeStore.LookupEntry(key.ChallengeID))
            key.ChallengeID = SelectChallenge();

        uint32 maxLevel = uint32(std::clamp(sConfigMgr->GetIntDefault("RetailSystems.MythicPlus.MaxKeyLevel", 50), 2, 100));
        key.Level = std::clamp(key.Level, 2u, maxLevel);

        for (uint8 i = 0; i < key.Affixes.size(); ++i)
            if (key.Affixes[i] <= 0 || !sKeystoneAffixStore.LookupEntry(uint32(key.Affixes[i])))
                key.Affixes[i] = _currentAffixes[i];
    }

    void ApplyKeystone(Player* owner, Item* item, KeystoneData const& key, bool persist) const
    {
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_MAP_CHALLENGE_MODE_ID, key.ChallengeID);
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL, key.Level);
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_1, uint32(std::max(key.Affixes[0], 0)));
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_2, uint32(std::max(key.Affixes[1], 0)));
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_3, uint32(std::max(key.Affixes[2], 0)));
        item->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_AFFIX_ID_4, uint32(std::max(key.Affixes[3], 0)));
        item->SetState(ITEM_CHANGED, owner);

        if (persist)
            CharacterDatabase.PExecute(
                "INSERT INTO `retail_mythic_keystone` (`item_guid`, `owner_guid`, `map_challenge_mode_id`, `level`, "
                "`affix_1`, `affix_2`, `affix_3`, `affix_4`, `updated_at`) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}) "
                "ON DUPLICATE KEY UPDATE `owner_guid` = VALUES(`owner_guid`), `map_challenge_mode_id` = VALUES(`map_challenge_mode_id`), "
                "`level` = VALUES(`level`), `affix_1` = VALUES(`affix_1`), `affix_2` = VALUES(`affix_2`), "
                "`affix_3` = VALUES(`affix_3`), `affix_4` = VALUES(`affix_4`), `updated_at` = VALUES(`updated_at`)",
                item->GetGUID().GetCounter(), owner->GetGUID().GetCounter(), key.ChallengeID, key.Level,
                key.Affixes[0], key.Affixes[1], key.Affixes[2], key.Affixes[3], uint64(GameTime::GetGameTime()));
    }

    void DepleteKeystone(ActiveRun const& run) const
    {
        uint32 newLevel = std::max(run.Level - 1, 2u);
        if (Player* owner = ObjectAccessor::FindConnectedPlayer(run.KeyOwnerGuid))
        {
            if (Item* item = owner->GetItemByGuid(run.KeyItemGuid))
            {
                KeystoneData key = ReadKeystone(item);
                key.Level = newLevel;
                ApplyKeystone(owner, item, key, true);
                return;
            }
        }

        CharacterDatabase.PExecute(
            "UPDATE `retail_mythic_keystone` SET `level` = {}, `updated_at` = {} WHERE `item_guid` = {}",
            newLevel, uint64(GameTime::GetGameTime()), run.KeyItemGuid.GetCounter());
    }

    void UpgradeKeystone(ActiveRun const& run, uint8 upgradeLevel) const
    {
        uint32 newLevel = upgradeLevel ? run.Level + upgradeLevel : std::max(run.Level - 1, 2u);
        uint32 nextChallenge = SelectChallenge(run.ChallengeID);

        if (Player* owner = ObjectAccessor::FindConnectedPlayer(run.KeyOwnerGuid))
        {
            if (Item* item = owner->GetItemByGuid(run.KeyItemGuid))
            {
                KeystoneData key = ReadKeystone(item);
                key.ChallengeID = nextChallenge;
                key.Level = newLevel;
                key.Affixes = _currentAffixes;
                NormalizeKeystone(key);
                ApplyKeystone(owner, item, key, true);
                return;
            }
        }

        CharacterDatabase.PExecute(
            "UPDATE `retail_mythic_keystone` SET `map_challenge_mode_id` = {}, `level` = {}, "
            "`affix_1` = {}, `affix_2` = {}, `affix_3` = {}, `affix_4` = {}, `updated_at` = {} WHERE `item_guid` = {}",
            nextChallenge, newLevel, _currentAffixes[0], _currentAffixes[1], _currentAffixes[2], _currentAffixes[3],
            uint64(GameTime::GetGameTime()), run.KeyItemGuid.GetCounter());
    }

    void CheckDeaths(Map* map, ActiveRun& run)
    {
        bool changed = false;
        map->DoOnPlayers([&](Player* player)
        {
            if (player->IsGameMaster())
                return;

            if (player->IsAlive())
                run.DeadPlayers.erase(player->GetGUID());
            else if (run.DeadPlayers.insert(player->GetGUID()).second)
            {
                ++run.DeathCount;
                changed = true;
            }
        });

        if (!changed)
            return;

        WorldPackets::RetailChallengeMode::UpdateDeathCount deaths;
        deaths.DeathCount = int32(run.DeathCount);
        map->SendToPlayers(deaths.Write());
        BroadcastElapsed(map, run);
    }

    uint8 GetUpgradeLevel(ActiveRun const& run, MapChallengeModeEntry const* challenge) const
    {
        if (!challenge)
            return 0;

        uint64 duration = EffectiveDuration(run);
        int32 timeBonus = GetCriteriaCountBonus(run);
        if (challenge->CriteriaCount[2] + timeBonus > 0 &&
            duration < uint64(challenge->CriteriaCount[2] + timeBonus) * IN_MILLISECONDS)
            return 3;
        if (challenge->CriteriaCount[1] + timeBonus > 0 &&
            duration < uint64(challenge->CriteriaCount[1] + timeBonus) * IN_MILLISECONDS)
            return 2;
        if (challenge->CriteriaCount[0] + timeBonus > 0 &&
            duration < uint64(challenge->CriteriaCount[0] + timeBonus) * IN_MILLISECONDS)
            return 1;
        return 0;
    }

    float CalculateScore(ActiveRun const& run, MapChallengeModeEntry const* challenge, uint8 upgradeLevel) const
    {
        if (!challenge || challenge->CriteriaCount[0] <= 0)
            return 0.0f;

        int32 timeBonus = GetCriteriaCountBonus(run);
        float baseTime = float(challenge->CriteriaCount[0] + timeBonus) * float(IN_MILLISECONDS);
        float maxBonusTime = baseTime * 0.4f;
        float bonus = (baseTime - float(EffectiveDuration(run))) / maxBonusTime;
        if (bonus <= -1.0f)
            return 0.0f;

        float adjustedLevel = float(run.Level) + std::clamp(bonus, -1.0f, 1.0f);
        if (!upgradeLevel)
            adjustedLevel -= 1.0f;
        int32 levelsAboveTen = std::max(int32(run.Level) - 10, 0);
        uint32 affixCount = std::ranges::count_if(run.Affixes, [](int32 affix) { return affix > 0; });
        return 20.0f + adjustedLevel * 5.0f + float(levelsAboveTen * 2) + float(affixCount * 10);
    }

    void Complete(Map* map, std::shared_ptr<ActiveRun> const& run)
    {
        MapChallengeModeEntry const* challenge = sMapChallengeModeStore.LookupEntry(run->ChallengeID);
        uint8 upgradeLevel = GetUpgradeLevel(*run, challenge);
        float score = CalculateScore(*run, challenge, upgradeLevel);
        time_t completionTime = GameTime::GetGameTime();
        uint64 duration = std::min<uint64>(EffectiveDuration(*run), std::numeric_limits<int32>::max());

        WorldPackets::MythicPlus::MythicPlusRun packetRun;
        packetRun.MapChallengeModeID = int32(run->ChallengeID);
        packetRun.Completed = true;
        packetRun.Level = run->Level;
        packetRun.DurationMs = int32(duration);
        packetRun.StartDate = run->StartDate;
        packetRun.CompletionDate = completionTime;
        packetRun.Season = int32(_displaySeason);
        packetRun.RunScore = score;
        packetRun.KeystoneAffixIDs = run->Affixes;
        for (MemberSnapshot const& member : run->Members)
            packetRun.Members.push_back(ToPacketMember(member));

        PersistRun(*run, duration, completionTime, score, upgradeLevel);
        UpgradeKeystone(*run, upgradeLevel);

        WorldPackets::RetailChallengeMode::ElapsedTimerStop stop;
        map->SendToPlayers(stop.Write());

        for (MemberSnapshot const& recipient : run->Members)
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(recipient.Guid);
            if (!player || player->GetMap() != map)
                continue;

            float previousMapScore = 0.0f;
            float overallScore = GetOverallScore(recipient.Guid.GetCounter(), run->ChallengeID, previousMapScore);
            overallScore += std::max(score - previousMapScore, 0.0f);

            WorldPackets::RetailChallengeMode::Complete complete;
            complete.Run = packetRun;
            complete.NewOverallScore = overallScore;
            complete.IsMapRecord = score > previousMapScore;
            complete.IsAffixRecorded = complete.IsMapRecord;
            for (MemberSnapshot const& member : run->Members)
                complete.Members.push_back({ member.Guid, member.Name, true });
            player->SendDirectMessage(complete.Write());
        }

        {
            std::lock_guard lock(_runsLock);
            _runs.erase(run->InstanceID);
        }

        TC_LOG_INFO("entities.player", "RetailSystems: completed Mythic+ challenge {} level {} in {} ms (score {:.2f})",
            run->ChallengeID, run->Level, duration, score);
    }

    void PersistRun(ActiveRun const& run, uint64 duration, time_t completionTime, float score, uint8 upgradeLevel) const
    {
        uint64 runId = (uint64(completionTime) << 32) | run.InstanceID;
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        transaction->PAppend(
            "INSERT INTO `retail_mythic_run` (`id`, `map_id`, `challenge_id`, `level`, `duration_ms`, `start_time`, "
            "`completion_time`, `mythic_season_id`, `display_season_id`, `score`, `affix_1`, `affix_2`, `affix_3`, "
            "`affix_4`, `death_count`, `upgrade_level`) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
            runId, run.MapID, run.ChallengeID, run.Level, duration, uint64(run.StartDate), uint64(completionTime),
            _mythicSeason, _displaySeason, score, run.Affixes[0], run.Affixes[1], run.Affixes[2], run.Affixes[3],
            run.DeathCount, upgradeLevel);

        for (MemberSnapshot const& member : run.Members)
        {
            std::string escapedName = member.Name;
            CharacterDatabase.EscapeString(escapedName);
            transaction->PAppend(
                "INSERT INTO `retail_mythic_run_member` (`run_id`, `member_guid`, `guild_id`, `specialization_id`, "
                "`race_id`, `item_level`, `name`, `eligible_for_score`) VALUES ({}, {}, {}, {}, {}, {}, '{}', 1)",
                runId, member.Guid.GetCounter(), member.GuildGuid.GetCounter(), member.SpecializationID,
                member.RaceID, member.ItemLevel, escapedName);
        }

        CharacterDatabase.CommitTransaction(transaction);
    }

    float GetOverallScore(uint64 memberGuid, uint32 currentChallenge, float& currentMapScore) const
    {
        float overallScore = 0.0f;
        currentMapScore = 0.0f;
        if (QueryResult result = CharacterDatabase.PQuery(
            "SELECT r.`challenge_id`, MAX(r.`score`) FROM `retail_mythic_run` r "
            "INNER JOIN `retail_mythic_run_member` m ON m.`run_id` = r.`id` "
            "WHERE m.`member_guid` = {} AND r.`display_season_id` = {} GROUP BY r.`challenge_id`",
            memberGuid, _displaySeason))
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 challengeId = fields[0].GetUInt32();
                float mapScore = fields[1].GetFloat();
                overallScore += mapScore;
                if (challengeId == currentChallenge)
                    currentMapScore = mapScore;
            } while (result->NextRow());
        }
        return overallScore;
    }

    static void SendError(Player* player, GameError error)
    {
        player->SendDirectMessage(WorldPackets::Misc::DisplayGameError(error).Write());
    }

    static void SendStart(Player* player, ActiveRun const& run)
    {
        WorldPackets::RetailChallengeMode::Start start;
        start.MapID = run.MapID;
        start.ChallengeID = run.ChallengeID;
        start.ChallengeLevel = int32(run.Level);
        start.Affixes = run.Affixes;
        start.DeathCount = int32(run.DeathCount);
        start.IsKeyCharged = true;
        player->SendDirectMessage(start.Write());
    }

    static void BroadcastStart(Map* map, ActiveRun const& run)
    {
        WorldPackets::RetailChallengeMode::Start start;
        start.MapID = run.MapID;
        start.ChallengeID = run.ChallengeID;
        start.ChallengeLevel = int32(run.Level);
        start.Affixes = run.Affixes;
        start.DeathCount = int32(run.DeathCount);
        start.IsKeyCharged = true;
        map->SendToPlayers(start.Write());
    }

    static void BroadcastElapsed(Map* map, ActiveRun const& run)
    {
        WorldPackets::RetailChallengeMode::ElapsedTimerStart elapsed;
        elapsed.CurrentDuration = EffectiveDuration(run) / IN_MILLISECONDS;
        map->SendToPlayers(elapsed.Write());
    }

    std::shared_ptr<ActiveRun> FindRun(Map const* map) const
    {
        if (!map || !map->GetInstanceId())
            return nullptr;

        std::lock_guard lock(_runsLock);
        auto itr = _runs.find(map->GetInstanceId());
        if (itr == _runs.end() || itr->second->MapID != map->GetId())
            return nullptr;
        return itr->second;
    }

    mutable std::mutex _runsLock;
    std::unordered_map<uint32, std::shared_ptr<ActiveRun>> _runs;
    uint32 _displaySeason = 0;
    uint32 _mythicSeason = 0;
    uint64 _affixWeek = std::numeric_limits<uint64>::max();
    std::array<int32, 4> _currentAffixes{};
};
}

void Initialize()
{
    Service::Instance().Initialize();
}

void Update(uint32 diff)
{
    Service::Instance().Update(diff);
}

bool Start(Player* player, uint8 bag, uint32 slot, ObjectGuid const& receptacleGuid)
{
    return Service::Instance().Start(player, bag, slot, receptacleGuid);
}

void Reset(Player* player, bool cheat)
{
    Service::Instance().Reset(player, cheat);
}

void RestoreKeystones(Player* player)
{
    Service::Instance().RestoreKeystones(player);
}

void RemoveCharacterState(uint64 characterGuid)
{
    Service::Instance().RemoveCharacterState(characterGuid);
}

void SendActiveRun(Player* player)
{
    Service::Instance().SendActiveRun(player);
}

void SendCurrentAffixes(Player* player)
{
    Service::Instance().SendCurrentAffixes(player);
}

void SendSeasonData(Player* player)
{
    Service::Instance().SendSeasonData(player);
}

void SendMapStats(Player* player)
{
    Service::Instance().SendMapStats(player);
}

void SendLeaders(Player* player, uint32 mapId, uint32 challengeId, uint64 lastGuildUpdate, uint64 lastRealmUpdate)
{
    Service::Instance().SendLeaders(player, mapId, challengeId, lastGuildUpdate, lastRealmUpdate);
}

bool IsActive(Map const* map)
{
    return Service::Instance().IsActive(map);
}

uint32 GetLevel(Map const* map)
{
    return Service::Instance().GetLevel(map);
}

void ScaleDamage(Unit* attacker, Unit* victim, uint32& damage)
{
    if (!attacker || !victim || !damage || attacker->GetMap() != victim->GetMap())
        return;

    uint32 level = GetLevel(victim->GetMap());
    if (!level)
        return;

    bool attackerIsPlayerControlled = attacker->GetCharmerOrOwnerPlayerOrPlayerItself() != nullptr;
    bool victimIsPlayerControlled = victim->GetCharmerOrOwnerPlayerOrPlayerItself() != nullptr;
    if (attackerIsPlayerControlled == victimIsPlayerControlled)
        return;

    Creature const* enemy = attackerIsPlayerControlled ? victim->ToCreature() : attacker->ToCreature();
    bool isBoss = enemy && enemy->IsDungeonBoss();
    bool tyrannical = Service::Instance().HasAffix(victim->GetMap(), AffixTyrannical);
    bool fortified = Service::Instance().HasAffix(victim->GetMap(), AffixFortified);

    double scaledDamage = damage;
    if (attackerIsPlayerControlled)
    {
        double healthScalar = DB2::GetChallengeHealthScalar(level);
        if (isBoss && tyrannical)
            healthScalar *= 1.25;
        else if (!isBoss && fortified)
            healthScalar *= 1.20;
        scaledDamage /= healthScalar;
    }
    else
    {
        double damageScalar = DB2::GetChallengeDamageScalar(level);
        if (isBoss && tyrannical)
            damageScalar *= 1.15;
        else if (!isBoss && fortified)
            damageScalar *= 1.20;
        scaledDamage *= damageScalar;
    }

    damage = uint32(std::clamp(scaledDamage, 0.0, double(std::numeric_limits<uint32>::max())));
}
}
