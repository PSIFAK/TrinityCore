/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_BATTLE_PAY_SERVICE_H
#define TRINITYCORE_RETAIL_BATTLE_PAY_SERVICE_H

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>

class WorldSession;
class WorldPacket;

namespace RetailSystems::BattlePay
{
void Initialize();
bool IsEnabled();
bool HasCapturedCatalog();
bool GetCapturedDBReply(uint32 tableHash, uint32 recordID, uint8& status, std::vector<uint8>& data, uint32& delayMs);
bool SendCapturedOpenCheckout(WorldSession* session, uint32 clientToken);
bool SendCapturedAvailableHotfixes(WorldSession* session);
bool SendCapturedHotfixConnect(WorldSession* session, std::vector<int32> const& requestedPushIDs);
uint32 GetCapturedRpcDelay(uint32 serviceHash, uint32 methodId);
void SendCapturedPacket(WorldSession* session, WorldPacket const* packet, uint32 delayMs);
void ProcessDelayedPackets(WorldSession* session);
void ClearDelayedPackets(WorldSession* session);

void SendProductList(WorldSession* session);
void SendPurchaseList(WorldSession* session);
void SendDistributionList(WorldSession* session);
void SendCatalogShopLicense(WorldSession* session);
void SendLoginBootstrap(WorldSession* session);
bool SendCapturedCatalogShopLicenseData(WorldSession* session, std::vector<uint32> const& gameDataIDs);
bool SendCapturedStoreGateResponse(WorldSession* session, uint32 requestOpcode, uint32 clientToken);
void SendLastCatalogFetch(WorldSession* session);
void UpdateLastCatalogFetch(uint32 accountID);
void StartPurchase(WorldSession* session, uint32 clientToken, uint32 productID, ObjectGuid targetCharacter, uint32 unknown);
void ConfirmPurchase(WorldSession* session, uint32 serverToken, uint64 clientCurrentPriceFixedPoint, bool confirmed);
void AcknowledgeFailure(WorldSession* session, uint32 serverToken);
void ClearPending(uint32 accountID);
}

#endif // TRINITYCORE_RETAIL_BATTLE_PAY_SERVICE_H
