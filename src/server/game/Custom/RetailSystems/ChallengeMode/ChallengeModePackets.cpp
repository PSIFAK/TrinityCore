/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChallengeModePackets.h"
#include "PacketOperators.h"
#include <algorithm>
#include <string_view>

namespace WorldPackets::RetailChallengeMode
{
void StartRequest::Read()
{
    _worldPacket >> Bag;
    _worldPacket >> Slot;
    _worldPacket >> GameObjectGUID;
}

void RequestLeaders::Read()
{
    _worldPacket >> LastGuildUpdate;
    _worldPacket >> LastRealmUpdate;
    _worldPacket >> MapID;
    _worldPacket >> ChallengeID;
}

WorldPacket const* Start::Write()
{
    _worldPacket << uint32(MapID);
    _worldPacket << uint32(ChallengeID);
    _worldPacket << int32(ChallengeLevel);
    for (int32 affix : Affixes)
        _worldPacket << int32(affix);
    _worldPacket << int32(DeathCount);
    _worldPacket << uint32(0); // ClientEncounterStartPlayerInfo count
    _worldPacket << Bits<1>(IsKeyCharged);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* UpdateDeathCount::Write()
{
    _worldPacket << int32(DeathCount);
    return &_worldPacket;
}

WorldPacket const* Reset::Write()
{
    _worldPacket << uint32(MapID);
    return &_worldPacket;
}

WorldPacket const* Complete::Write()
{
    _worldPacket << Run;
    _worldPacket << float(NewOverallScore);
    _worldPacket << Size<uint32>(Members);
    _worldPacket << Bits<1>(PracticeRun);
    _worldPacket << Bits<1>(IsAffixRecorded);
    _worldPacket << Bits<1>(IsMapRecord);
    _worldPacket.FlushBits();

    for (CompletionMember const& member : Members)
    {
        std::string_view const name(member.Name.data(), std::min<std::size_t>(member.Name.size(), 63));
        _worldPacket << member.GUID;
        _worldPacket << SizedString::BitsSize<6>(name);
        _worldPacket << Bits<1>(member.IsEligibleForScore);
        _worldPacket.FlushBits();
        _worldPacket << SizedString::Data(name);
    }

    return &_worldPacket;
}

WorldPacket const* CurrentAffixes::Write()
{
    _worldPacket << Size<uint32>(Affixes);
    for (AffixInfo const& affix : Affixes)
    {
        _worldPacket << int32(affix.KeystoneAffixID);
        _worldPacket << int32(affix.RequiredSeason);
    }
    return &_worldPacket;
}

WorldPacket const* SeasonData::Write()
{
    _worldPacket << Bits<1>(IsMythicPlusActive);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* AllMapStats::Write()
{
    _worldPacket << Size<uint32>(Runs);
    _worldPacket << uint32(0); // Reward history count
    _worldPacket << int32(Season);
    _worldPacket << int32(Subseason);
    for (MythicPlus::MythicPlusRun const& run : Runs)
        _worldPacket << run;
    return &_worldPacket;
}

WorldPacket const* ElapsedTimerStart::Write()
{
    _worldPacket << uint64(CurrentDuration);
    _worldPacket << int32(TimerID);
    return &_worldPacket;
}

WorldPacket const* ElapsedTimerStop::Write()
{
    _worldPacket << int32(TimerID);
    _worldPacket << Bits<1>(KeepTimer);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* LeadersResult::Write()
{
    _worldPacket << uint32(MapID);
    _worldPacket << uint32(ChallengeID);
    _worldPacket << uint64(LastGuildUpdate);
    _worldPacket << uint64(LastRealmUpdate);
    _worldPacket << Size<uint32>(GuildLeaders);
    _worldPacket << Size<uint32>(RealmLeaders);

    auto writeAttempt = [this](LeaderboardAttempt const& attempt)
    {
        _worldPacket << uint32(attempt.InstanceRealmAddress);
        _worldPacket << uint32(attempt.AttemptID);
        _worldPacket << uint32(attempt.CompletionTime);
        _worldPacket << attempt.CompletionDate;
        _worldPacket << uint32(attempt.MedalEarned);
        _worldPacket << Size<uint32>(attempt.Members);
        for (LeaderboardMember const& member : attempt.Members)
        {
            _worldPacket << uint32(member.VirtualRealmAddress);
            _worldPacket << uint32(member.NativeRealmAddress);
            _worldPacket << member.GUID;
            _worldPacket << uint32(member.SpecializationID);
        }
    };

    for (LeaderboardAttempt const& attempt : GuildLeaders)
        writeAttempt(attempt);
    for (LeaderboardAttempt const& attempt : RealmLeaders)
        writeAttempt(attempt);

    return &_worldPacket;
}
}
