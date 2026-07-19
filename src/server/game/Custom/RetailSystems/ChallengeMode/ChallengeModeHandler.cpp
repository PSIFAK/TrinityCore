/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ChallengeModePackets.h"
#include "ChallengeModeService.h"
#include "WorldSession.h"

void WorldSession::HandleStartChallengeMode(WorldPackets::RetailChallengeMode::StartRequest& packet)
{
    RetailSystems::ChallengeMode::Start(GetPlayer(), packet.Bag, packet.Slot, packet.GameObjectGUID);
}

void WorldSession::HandleResetChallengeMode(WorldPackets::RetailChallengeMode::ResetRequest& /*packet*/)
{
    RetailSystems::ChallengeMode::Reset(GetPlayer(), false);
}

void WorldSession::HandleResetChallengeModeCheat(WorldPackets::RetailChallengeMode::ResetCheatRequest& /*packet*/)
{
    RetailSystems::ChallengeMode::Reset(GetPlayer(), true);
}

void WorldSession::HandleRequestMythicPlusAffixes(WorldPackets::RetailChallengeMode::RequestAffixes& /*packet*/)
{
    RetailSystems::ChallengeMode::SendCurrentAffixes(GetPlayer());
}

void WorldSession::HandleRequestMythicPlusSeasonData(WorldPackets::RetailChallengeMode::RequestSeasonData& /*packet*/)
{
    RetailSystems::ChallengeMode::SendSeasonData(GetPlayer());
}

void WorldSession::HandleMythicPlusRequestMapStats(WorldPackets::RetailChallengeMode::RequestMapStats& /*packet*/)
{
    RetailSystems::ChallengeMode::SendMapStats(GetPlayer());
}

void WorldSession::HandleChallengeModeRequestLeaders(WorldPackets::RetailChallengeMode::RequestLeaders& packet)
{
    RetailSystems::ChallengeMode::SendLeaders(GetPlayer(), packet.MapID, packet.ChallengeID,
        packet.LastGuildUpdate, packet.LastRealmUpdate);
}
