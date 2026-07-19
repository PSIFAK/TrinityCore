/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_CHALLENGE_MODE_PACKETS_H
#define TRINITYCORE_RETAIL_CHALLENGE_MODE_PACKETS_H

#include "MythicPlusPacketsCommon.h"
#include "Packet.h"
#include "WowTime.h"

namespace WorldPackets::MythicPlus
{
ByteBuffer& operator<<(ByteBuffer& data, MythicPlusMember const& mythicPlusMember);
ByteBuffer& operator<<(ByteBuffer& data, MythicPlusRun const& mythicPlusRun);
}

namespace WorldPackets::RetailChallengeMode
{
class StartRequest final : public ClientPacket
{
public:
    explicit StartRequest(WorldPacket&& packet) : ClientPacket(CMSG_START_CHALLENGE_MODE, std::move(packet)) { }
    void Read() override;

    uint8 Bag = 0;
    uint32 Slot = 0;
    ObjectGuid GameObjectGUID;
};

class ResetRequest final : public ClientPacket
{
public:
    explicit ResetRequest(WorldPacket&& packet) : ClientPacket(CMSG_RESET_CHALLENGE_MODE, std::move(packet)) { }
    void Read() override { }
};

class ResetCheatRequest final : public ClientPacket
{
public:
    explicit ResetCheatRequest(WorldPacket&& packet) : ClientPacket(CMSG_RESET_CHALLENGE_MODE_CHEAT, std::move(packet)) { }
    void Read() override { }
};

class RequestAffixes final : public ClientPacket
{
public:
    explicit RequestAffixes(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_MYTHIC_PLUS_AFFIXES, std::move(packet)) { }
    void Read() override { }
};

class RequestSeasonData final : public ClientPacket
{
public:
    explicit RequestSeasonData(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_MYTHIC_PLUS_SEASON_DATA, std::move(packet)) { }
    void Read() override { }
};

class RequestMapStats final : public ClientPacket
{
public:
    explicit RequestMapStats(WorldPacket&& packet) : ClientPacket(CMSG_MYTHIC_PLUS_REQUEST_MAP_STATS, std::move(packet)) { }
    void Read() override { }
};

class RequestLeaders final : public ClientPacket
{
public:
    explicit RequestLeaders(WorldPacket&& packet) : ClientPacket(CMSG_CHALLENGE_MODE_REQUEST_LEADERS, std::move(packet)) { }
    void Read() override;

    uint64 LastGuildUpdate = 0;
    uint64 LastRealmUpdate = 0;
    uint32 MapID = 0;
    uint32 ChallengeID = 0;
};

class Start final : public ServerPacket
{
public:
    Start() : ServerPacket(SMSG_CHALLENGE_MODE_START, 41) { }
    WorldPacket const* Write() override;

    uint32 MapID = 0;
    uint32 ChallengeID = 0;
    int32 ChallengeLevel = 0;
    std::array<int32, 4> Affixes{};
    int32 DeathCount = 0;
    bool IsKeyCharged = false;
};

class UpdateDeathCount final : public ServerPacket
{
public:
    UpdateDeathCount() : ServerPacket(SMSG_CHALLENGE_MODE_UPDATE_DEATH_COUNT, 4) { }
    WorldPacket const* Write() override;

    int32 DeathCount = 0;
};

class Reset final : public ServerPacket
{
public:
    Reset() : ServerPacket(SMSG_CHALLENGE_MODE_RESET, 4) { }
    WorldPacket const* Write() override;

    uint32 MapID = 0;
};

struct CompletionMember
{
    ObjectGuid GUID;
    std::string Name;
    bool IsEligibleForScore = true;
};

class Complete final : public ServerPacket
{
public:
    Complete() : ServerPacket(SMSG_CHALLENGE_MODE_COMPLETE) { }
    WorldPacket const* Write() override;

    MythicPlus::MythicPlusRun Run;
    float NewOverallScore = 0.0f;
    std::vector<CompletionMember> Members;
    bool PracticeRun = false;
    bool IsAffixRecorded = false;
    bool IsMapRecord = false;
};

struct AffixInfo
{
    int32 KeystoneAffixID = 0;
    int32 RequiredSeason = 0;
};

class CurrentAffixes final : public ServerPacket
{
public:
    CurrentAffixes() : ServerPacket(SMSG_MYTHIC_PLUS_CURRENT_AFFIXES) { }
    WorldPacket const* Write() override;

    std::vector<AffixInfo> Affixes;
};

class SeasonData final : public ServerPacket
{
public:
    SeasonData() : ServerPacket(SMSG_MYTHIC_PLUS_SEASON_DATA, 1) { }
    WorldPacket const* Write() override;

    bool IsMythicPlusActive = true;
};

class AllMapStats final : public ServerPacket
{
public:
    AllMapStats() : ServerPacket(SMSG_MYTHIC_PLUS_ALL_MAP_STATS) { }
    WorldPacket const* Write() override;

    std::vector<MythicPlus::MythicPlusRun> Runs;
    int32 Season = 0;
    int32 Subseason = 0;
};

class ElapsedTimerStart final : public ServerPacket
{
public:
    ElapsedTimerStart() : ServerPacket(SMSG_START_ELAPSED_TIMER, 12) { }
    WorldPacket const* Write() override;

    uint64 CurrentDuration = 0;
    int32 TimerID = 1;
};

class ElapsedTimerStop final : public ServerPacket
{
public:
    ElapsedTimerStop() : ServerPacket(SMSG_STOP_ELAPSED_TIMER, 5) { }
    WorldPacket const* Write() override;

    int32 TimerID = 1;
    bool KeepTimer = false;
};

struct LeaderboardMember
{
    uint32 VirtualRealmAddress = 0;
    uint32 NativeRealmAddress = 0;
    ObjectGuid GUID;
    uint32 SpecializationID = 0;
};

struct LeaderboardAttempt
{
    uint32 InstanceRealmAddress = 0;
    uint32 AttemptID = 0;
    uint32 CompletionTime = 0;
    WowTime CompletionDate;
    uint32 MedalEarned = 0;
    std::vector<LeaderboardMember> Members;
};

class LeadersResult final : public ServerPacket
{
public:
    LeadersResult() : ServerPacket(SMSG_CHALLENGE_MODE_REQUEST_LEADERS_RESULT, 32) { }
    WorldPacket const* Write() override;

    uint32 MapID = 0;
    uint32 ChallengeID = 0;
    uint64 LastGuildUpdate = 0;
    uint64 LastRealmUpdate = 0;
    std::vector<LeaderboardAttempt> GuildLeaders;
    std::vector<LeaderboardAttempt> RealmLeaders;
};
}

#endif // TRINITYCORE_RETAIL_CHALLENGE_MODE_PACKETS_H
