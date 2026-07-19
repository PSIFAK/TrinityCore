/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlePayPackets.h"
#include "BattlePayService.h"
#include "WorldSession.h"
#include <utility>

void WorldSession::HandleBattlePayGetProductList(WorldPackets::RetailBattlePay::GetProductList& /*packet*/)
{
    if (!RetailSystems::BattlePay::HasCapturedCatalog())
        RetailSystems::BattlePay::SendCatalogShopLicense(this);
    RetailSystems::BattlePay::SendProductList(this);
}

void WorldSession::HandleBattlePayGetPurchaseList(WorldPackets::RetailBattlePay::GetPurchaseList& /*packet*/)
{
    RetailSystems::BattlePay::SendPurchaseList(this);
}

void WorldSession::HandleBattlePayOpenCheckout(WorldPackets::RetailBattlePay::OpenCheckout& packet)
{
    RetailSystems::BattlePay::SendCapturedOpenCheckout(this, packet.ClientToken);
}

void WorldSession::HandleBattlePayGetLastCatalogFetch(WorldPackets::RetailBattlePay::GetLastCatalogFetch& /*packet*/)
{
    // Capture mode sends this after entitlements and the product list, matching retail ordering.
    if (!RetailSystems::BattlePay::HasCapturedCatalog())
        RetailSystems::BattlePay::SendLastCatalogFetch(this);
}

void WorldSession::HandleBattlePayUpdateLastCatalogFetch(WorldPackets::RetailBattlePay::UpdateLastCatalogFetch& /*packet*/)
{
    RetailSystems::BattlePay::UpdateLastCatalogFetch(GetAccountId());
    RetailSystems::BattlePay::SendLastCatalogFetch(this);
}

void WorldSession::HandleBattlePayCatalogShopLicenseGameDataRequest(
    WorldPackets::RetailBattlePay::CatalogShopLicenseGameDataRequest& packet)
{
    if (RetailSystems::BattlePay::SendCapturedCatalogShopLicenseData(this, packet.GameDataIDs))
        return;

    WorldPackets::RetailBattlePay::CatalogShopLicenseData response;
    response.MissingGameDataIDs = std::move(packet.GameDataIDs);
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayCommerceTokenGetMarketPrice(
    WorldPackets::RetailBattlePay::CommerceTokenGetMarketPrice& packet)
{
    RetailSystems::BattlePay::SendCapturedStoreGateResponse(this, CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE,
        packet.ClientToken);
}

void WorldSession::HandleBattlePayConsumableTokenCanVeteranBuy(
    WorldPackets::RetailBattlePay::ConsumableTokenCanVeteranBuy& packet)
{
    RetailSystems::BattlePay::SendCapturedStoreGateResponse(this, CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY,
        packet.ClientToken);
}

void WorldSession::HandleBattlePayGetDecorRefundList(WorldPackets::RetailBattlePay::GetDecorRefundList& /*packet*/)
{
    if (RetailSystems::BattlePay::SendCapturedStoreGateResponse(this, CMSG_GET_DECOR_REFUND_LIST, 0))
        return;

    WorldPackets::RetailBattlePay::DecorRefundListResponse response;
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayGetAllLicensedDecorQuantities(
    WorldPackets::RetailBattlePay::GetAllLicensedDecorQuantities& /*packet*/)
{
    if (RetailSystems::BattlePay::SendCapturedStoreGateResponse(this, CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES, 0))
        return;

    WorldPackets::RetailBattlePay::AllLicensedDecorQuantitiesResponse response;
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayRequestStoreFrontInfoUpdate(
    WorldPackets::RetailBattlePay::RequestStoreFrontInfoUpdate& packet)
{
    WorldPackets::RetailBattlePay::AccountStoreFrontUpdate response;
    response.StoreFrontID = packet.StoreFrontID;
    SendPacket(response.Write());
}

void WorldSession::HandleBattlePayStartPurchase(WorldPackets::RetailBattlePay::StartPurchase& packet)
{
    RetailSystems::BattlePay::StartPurchase(this, packet.ClientToken, packet.ProductID, packet.TargetCharacter, packet.Unknown);
}

void WorldSession::HandleBattlePayConfirmPurchase(WorldPackets::RetailBattlePay::ConfirmPurchaseResponse& packet)
{
    RetailSystems::BattlePay::ConfirmPurchase(this, packet.ServerToken, packet.ClientCurrentPriceFixedPoint, packet.ConfirmPurchase);
}

void WorldSession::HandleBattlePayAckFailed(WorldPackets::RetailBattlePay::AckFailedResponse& packet)
{
    RetailSystems::BattlePay::AcknowledgeFailure(this, packet.ServerToken);
}

void WorldSession::HandleBattlePayRequestPriceInfo(WorldPackets::RetailBattlePay::RequestPriceInfo& /*packet*/)
{
    RetailSystems::BattlePay::SendProductList(this);
}
