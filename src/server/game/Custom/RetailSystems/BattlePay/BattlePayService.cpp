/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlePayService.h"
#include "BattlePayPackets.h"
#include "CollectionMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Item.h"
#include "ItemEnchantmentMgr.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "SpellMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace RetailSystems::BattlePay
{
namespace
{
enum class Error : uint32
{
    Ok = 0,
    PurchaseDenied = 1,
    PaymentFailed = 2,
    BattlePayDisabled = 13,
    InsufficientBalance = 28
};

enum class PurchaseStatus : uint32
{
    Finish = 3,
    Loading = 9
};

enum class DeliveryType : uint8
{
    Item = 0,
    Spell = 1,
    Mount = 2,
    Toy = 3
};

struct DeliveryAction
{
    uint32 DeliverableID = 0;
    DeliveryType Type = DeliveryType::Item;
    uint32 Entry = 0;
    uint32 Quantity = 1;
};

struct CatalogDeliverable
{
    WorldPackets::RetailBattlePay::Deliverable Packet;
    DeliveryAction Action;
};

struct CatalogProduct
{
    WorldPackets::RetailBattlePay::Product Packet;
    WorldPackets::RetailBattlePay::ShopEntry Shop;
    std::vector<CatalogDeliverable> Deliverables;
    std::vector<DeliveryAction> Fulfillment;
};

struct PendingPurchase
{
    uint64 PurchaseID = 0;
    uint32 AccountID = 0;
    uint32 ClientToken = 0;
    uint32 ServerToken = 0;
    uint32 ProductID = 0;
    ObjectGuid TargetCharacter;
    uint64 BasePrice = 0;
    uint64 CurrentPrice = 0;
    uint64 CreatedAt = 0;
    std::chrono::steady_clock::time_point ExpiresAt;
};

struct CapturedCatalogShopRoute
{
    std::vector<uint32> RequestIDs;
    std::vector<uint32> SortedRequestIDs;
    std::vector<uint8> ResponsePayload;
    uint32 ResponseDelayMs = 0;
};

struct CapturedBootstrapPacket
{
    uint32 Opcode = 0;
    std::vector<uint8> Payload;
    uint32 DelayMs = 0;
};

struct CapturedStoreGateRoute
{
    uint32 RequestOpcode = 0;
    uint32 ResponseOpcode = 0;
    std::vector<uint8> ResponsePayload;
    uint32 ResponseDelayMs = 0;
};

struct CapturedDBReply
{
    uint8 Status = 0;
    std::vector<uint8> Data;
};

struct CapturedDBQueryRoute
{
    uint32 TableHash = 0;
    uint32 DefaultStatus = std::numeric_limits<uint32>::max();
    std::unordered_map<uint32, CapturedDBReply> Replies;
    uint32 ResponseDelayMs = 0;
};

struct CapturedCatalogData
{
    uint32 ClientBuild = 0;
    uint32 LicenseID = 0;
    uint64 CatalogVersion = 0;
    std::vector<uint8> ProductListPayload;
    uint32 ProductResponseDelayMs = 0;
    std::vector<CapturedBootstrapPacket> PreProductPackets;
    std::vector<CapturedStoreGateRoute> StoreGateRoutes;
    std::vector<uint8> OpenCheckoutResponsePayload;
    uint32 OpenCheckoutResponseDelayMs = 0;
    std::vector<CapturedDBQueryRoute> DBQueryRoutes;
    uint32 LastCatalogResponseDelayMs = 0;
    std::vector<uint8> AvailableHotfixesPayload;
    std::vector<uint8> HotfixConnectPayload;
    std::vector<int32> HotfixPushIDs;
    uint32 HotfixConnectDelayMs = 0;
    std::unordered_map<uint64, uint32> RpcResponseDelays;
    std::vector<CapturedBootstrapPacket> LoginBootstrapPackets;
    std::vector<CapturedCatalogShopRoute> CatalogShopRoutes;
};

struct DelayedCapturedPacket
{
    std::chrono::steady_clock::time_point DueAt;
    uint64 Sequence = 0;
    uint32 Opcode = 0;
    ConnectionType Connection = CONNECTION_TYPE_DEFAULT;
    std::vector<uint8> Payload;
};

std::atomic<bool> Enabled = false;
std::atomic<uint32> CatalogShopLicenseID = 0x0011A691;
uint32 CurrencyID = 1;
std::string WalletName = "Battle Coins";
std::vector<WorldPackets::RetailBattlePay::Group> Groups;
std::unordered_map<uint32, CatalogProduct> Products;
std::vector<uint32> ProductOrder;
CapturedCatalogData CapturedCatalog;
std::shared_mutex CatalogLock;

std::unordered_map<uint32, PendingPurchase> PendingPurchases;
std::mutex PendingLock;
std::atomic<uint32> PurchaseSequence = 0;

std::unordered_map<WorldSession*, std::vector<DelayedCapturedPacket>> DelayedCapturedPackets;
std::mutex DelayedCapturedPacketsLock;
std::atomic<uint64> DelayedPacketSequence = 0;

bool ReadCaptureUInt32(std::vector<uint8> const& data, std::size_t& position, uint32& value)
{
    if (position + sizeof(uint32) > data.size())
        return false;

    value = uint32(data[position]) |
        (uint32(data[position + 1]) << 8) |
        (uint32(data[position + 2]) << 16) |
        (uint32(data[position + 3]) << 24);
    position += sizeof(uint32);
    return true;
}

bool ReadCaptureUInt64(std::vector<uint8> const& data, std::size_t& position, uint64& value)
{
    uint32 low = 0;
    uint32 high = 0;
    if (!ReadCaptureUInt32(data, position, low) || !ReadCaptureUInt32(data, position, high))
        return false;

    value = uint64(low) | (uint64(high) << 32);
    return true;
}

bool ReadCaptureBytes(std::vector<uint8> const& data, std::size_t& position, uint32 size, std::vector<uint8>& value)
{
    if (size > 32 * 1024 * 1024 || position + size > data.size())
        return false;

    value.assign(data.begin() + position, data.begin() + position + size);
    position += size;
    return true;
}

bool LoadCapturedCatalog(std::string const& fileName, CapturedCatalogData& capture)
{
    std::ifstream file(std::filesystem::path(fileName), std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    std::streamsize fileSize = file.tellg();
    if (fileSize < 32 || fileSize > 64 * 1024 * 1024)
        return false;

    file.seekg(0);
    std::vector<uint8> data;
    data.resize(static_cast<std::size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(data.data()), fileSize))
        return false;

    if (!std::equal(data.begin(), data.begin() + 4, "TCBP"))
        return false;

    std::size_t position = 4;
    uint32 formatVersion = 0;
    uint32 productPayloadSize = 0;
    if (!ReadCaptureUInt32(data, position, formatVersion) ||
        (formatVersion != 5 && formatVersion != 6 && formatVersion != 7 && formatVersion != 8 && formatVersion != 9) ||
        !ReadCaptureUInt32(data, position, capture.ClientBuild) ||
        !ReadCaptureUInt32(data, position, capture.LicenseID) || !capture.LicenseID ||
        !ReadCaptureUInt64(data, position, capture.CatalogVersion) || !capture.CatalogVersion ||
        !ReadCaptureUInt32(data, position, productPayloadSize) ||
        !ReadCaptureBytes(data, position, productPayloadSize, capture.ProductListPayload))
        return false;

    if (formatVersion >= 8 &&
        (!ReadCaptureUInt32(data, position, capture.ProductResponseDelayMs) ||
         capture.ProductResponseDelayMs > 60 * 1000))
        return false;

    uint32 bootstrapCount = 0;
    if (!ReadCaptureUInt32(data, position, bootstrapCount) || !bootstrapCount || bootstrapCount > 16)
        return false;

    capture.PreProductPackets.reserve(bootstrapCount);
    for (uint32 bootstrapIndex = 0; bootstrapIndex < bootstrapCount; ++bootstrapIndex)
    {
        CapturedBootstrapPacket& bootstrap = capture.PreProductPackets.emplace_back();
        uint32 payloadSize = 0;
        if (!ReadCaptureUInt32(data, position, bootstrap.Opcode) ||
            (bootstrap.Opcode != SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED &&
             bootstrap.Opcode != SMSG_SYNC_WOW_ENTITLEMENTS &&
             bootstrap.Opcode != SMSG_CATALOG_SHOP_OBTAIN_LICENSE) ||
            !ReadCaptureUInt32(data, position, payloadSize) ||
            !ReadCaptureBytes(data, position, payloadSize, bootstrap.Payload))
            return false;

        if (formatVersion >= 8 &&
            (!ReadCaptureUInt32(data, position, bootstrap.DelayMs) || bootstrap.DelayMs > 60 * 1000))
            return false;
    }

    uint32 gateCount = 0;
    if (!ReadCaptureUInt32(data, position, gateCount) || gateCount != 4)
        return false;

    capture.StoreGateRoutes.reserve(gateCount);
    for (uint32 gateIndex = 0; gateIndex < gateCount; ++gateIndex)
    {
        CapturedStoreGateRoute& gate = capture.StoreGateRoutes.emplace_back();
        uint32 payloadSize = 0;
        if (!ReadCaptureUInt32(data, position, gate.RequestOpcode) ||
            !ReadCaptureUInt32(data, position, gate.ResponseOpcode) ||
            ((gate.RequestOpcode != CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE ||
              gate.ResponseOpcode != SMSG_COMMERCE_TOKEN_GET_MARKET_PRICE_RESPONSE) &&
             (gate.RequestOpcode != CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY ||
              gate.ResponseOpcode != SMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY_RESPONSE) &&
             (gate.RequestOpcode != CMSG_GET_DECOR_REFUND_LIST ||
              gate.ResponseOpcode != SMSG_GET_DECOR_REFUND_LIST_RESPONSE) &&
             (gate.RequestOpcode != CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES ||
              gate.ResponseOpcode != SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE)) ||
            !ReadCaptureUInt32(data, position, payloadSize) || payloadSize < sizeof(uint32) ||
            !ReadCaptureBytes(data, position, payloadSize, gate.ResponsePayload))
            return false;

        if (formatVersion >= 8 &&
            (!ReadCaptureUInt32(data, position, gate.ResponseDelayMs) || gate.ResponseDelayMs > 60 * 1000))
            return false;
    }

    if (formatVersion >= 7)
    {
        uint32 openCheckoutPayloadSize = 0;
        if (!ReadCaptureUInt32(data, position, openCheckoutPayloadSize) || openCheckoutPayloadSize < 25 ||
            !ReadCaptureBytes(data, position, openCheckoutPayloadSize, capture.OpenCheckoutResponsePayload))
            return false;

        if (formatVersion >= 8 &&
            (!ReadCaptureUInt32(data, position, capture.OpenCheckoutResponseDelayMs) ||
             capture.OpenCheckoutResponseDelayMs > 60 * 1000))
            return false;
    }

    if (formatVersion >= 6)
    {
        uint32 dbQueryRouteCount = 0;
        if (!ReadCaptureUInt32(data, position, dbQueryRouteCount) || !dbQueryRouteCount || dbQueryRouteCount > 16)
            return false;

        capture.DBQueryRoutes.reserve(dbQueryRouteCount);
        for (uint32 routeIndex = 0; routeIndex < dbQueryRouteCount; ++routeIndex)
        {
            CapturedDBQueryRoute& route = capture.DBQueryRoutes.emplace_back();
            uint32 recordCount = 0;
            if (!ReadCaptureUInt32(data, position, route.TableHash) || !route.TableHash ||
                !ReadCaptureUInt32(data, position, route.DefaultStatus) ||
                (route.DefaultStatus != std::numeric_limits<uint32>::max() && route.DefaultStatus > 4) ||
                !ReadCaptureUInt32(data, position, recordCount) || !recordCount || recordCount > 4096)
                return false;

            route.Replies.reserve(recordCount);
            for (uint32 recordIndex = 0; recordIndex < recordCount; ++recordIndex)
            {
                uint32 recordID = 0;
                uint32 status = 0;
                uint32 payloadSize = 0;
                CapturedDBReply reply;
                if (!ReadCaptureUInt32(data, position, recordID) ||
                    !ReadCaptureUInt32(data, position, status) || status > 4 ||
                    !ReadCaptureUInt32(data, position, payloadSize) ||
                    !ReadCaptureBytes(data, position, payloadSize, reply.Data))
                    return false;

                reply.Status = uint8(status);
                if (!route.Replies.emplace(recordID, std::move(reply)).second)
                    return false;
            }

            if (formatVersion >= 8 &&
                (!ReadCaptureUInt32(data, position, route.ResponseDelayMs) ||
                 route.ResponseDelayMs > 60 * 1000))
                return false;
        }
    }

    if (formatVersion >= 8)
    {
        uint32 availableHotfixesPayloadSize = 0;
        uint32 hotfixConnectPayloadSize = 0;
        uint32 rpcRouteCount = 0;
        if (!ReadCaptureUInt32(data, position, capture.LastCatalogResponseDelayMs) ||
            capture.LastCatalogResponseDelayMs > 60 * 1000 ||
            !ReadCaptureUInt32(data, position, availableHotfixesPayloadSize) ||
            !ReadCaptureBytes(data, position, availableHotfixesPayloadSize, capture.AvailableHotfixesPayload) ||
            capture.AvailableHotfixesPayload.size() < 8 ||
            !ReadCaptureUInt32(data, position, hotfixConnectPayloadSize) ||
            !ReadCaptureBytes(data, position, hotfixConnectPayloadSize, capture.HotfixConnectPayload) ||
            capture.HotfixConnectPayload.size() < 8 ||
            !ReadCaptureUInt32(data, position, capture.HotfixConnectDelayMs) ||
            capture.HotfixConnectDelayMs > 60 * 1000 ||
            !ReadCaptureUInt32(data, position, rpcRouteCount) || !rpcRouteCount || rpcRouteCount > 32)
            return false;

        uint32 availableHotfixCount = uint32(capture.AvailableHotfixesPayload[4]) |
            (uint32(capture.AvailableHotfixesPayload[5]) << 8) |
            (uint32(capture.AvailableHotfixesPayload[6]) << 16) |
            (uint32(capture.AvailableHotfixesPayload[7]) << 24);
        if (capture.AvailableHotfixesPayload.size() != 8 + std::size_t(availableHotfixCount) * 8)
            return false;

        uint32 hotfixRecordCount = uint32(capture.HotfixConnectPayload[0]) |
            (uint32(capture.HotfixConnectPayload[1]) << 8) |
            (uint32(capture.HotfixConnectPayload[2]) << 16) |
            (uint32(capture.HotfixConnectPayload[3]) << 24);
        constexpr std::size_t HotfixRecordHeaderSize = 21;
        if (hotfixRecordCount > (capture.HotfixConnectPayload.size() - 8) / HotfixRecordHeaderSize)
            return false;

        std::size_t hotfixContentSizePosition = 4 + std::size_t(hotfixRecordCount) * HotfixRecordHeaderSize;
        uint32 hotfixContentSize = uint32(capture.HotfixConnectPayload[hotfixContentSizePosition]) |
            (uint32(capture.HotfixConnectPayload[hotfixContentSizePosition + 1]) << 8) |
            (uint32(capture.HotfixConnectPayload[hotfixContentSizePosition + 2]) << 16) |
            (uint32(capture.HotfixConnectPayload[hotfixContentSizePosition + 3]) << 24);
        if (hotfixContentSizePosition + 4 + hotfixContentSize != capture.HotfixConnectPayload.size())
            return false;

        capture.HotfixPushIDs.reserve(hotfixRecordCount);
        for (uint32 recordIndex = 0; recordIndex < hotfixRecordCount; ++recordIndex)
        {
            std::size_t recordPosition = 4 + std::size_t(recordIndex) * HotfixRecordHeaderSize;
            uint32 pushID = uint32(capture.HotfixConnectPayload[recordPosition]) |
                (uint32(capture.HotfixConnectPayload[recordPosition + 1]) << 8) |
                (uint32(capture.HotfixConnectPayload[recordPosition + 2]) << 16) |
                (uint32(capture.HotfixConnectPayload[recordPosition + 3]) << 24);
            capture.HotfixPushIDs.push_back(int32(pushID));
        }
        std::ranges::sort(capture.HotfixPushIDs);
        capture.HotfixPushIDs.erase(std::unique(capture.HotfixPushIDs.begin(), capture.HotfixPushIDs.end()),
            capture.HotfixPushIDs.end());

        for (uint32 routeIndex = 0; routeIndex < rpcRouteCount; ++routeIndex)
        {
            uint32 serviceHash = 0;
            uint32 methodID = 0;
            uint32 delayMs = 0;
            if (!ReadCaptureUInt32(data, position, serviceHash) || !serviceHash ||
                !ReadCaptureUInt32(data, position, methodID) || !methodID ||
                !ReadCaptureUInt32(data, position, delayMs) || delayMs > 60 * 1000 ||
                !capture.RpcResponseDelays.emplace((uint64(serviceHash) << 32) | methodID, delayMs).second)
                return false;
        }

        if (formatVersion >= 9)
        {
            uint32 loginBootstrapCount = 0;
            if (!ReadCaptureUInt32(data, position, loginBootstrapCount) || loginBootstrapCount != 5)
                return false;

            capture.LoginBootstrapPackets.reserve(loginBootstrapCount);
            for (uint32 packetIndex = 0; packetIndex < loginBootstrapCount; ++packetIndex)
            {
                CapturedBootstrapPacket& packet = capture.LoginBootstrapPackets.emplace_back();
                uint32 payloadSize = 0;
                if (!ReadCaptureUInt32(data, position, packet.Opcode) ||
                    (packet.Opcode != SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE &&
                     packet.Opcode != SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED &&
                     packet.Opcode != SMSG_CATALOG_SHOP_OBTAIN_LICENSE) ||
                    !ReadCaptureUInt32(data, position, payloadSize) ||
                    !ReadCaptureBytes(data, position, payloadSize, packet.Payload) ||
                    !ReadCaptureUInt32(data, position, packet.DelayMs) || packet.DelayMs > 60 * 1000)
                    return false;
            }

            if (capture.LoginBootstrapPackets[0].Opcode != SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE ||
                capture.LoginBootstrapPackets[1].Opcode != SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED ||
                capture.LoginBootstrapPackets[2].Opcode != SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED ||
                capture.LoginBootstrapPackets[3].Opcode != SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED ||
                capture.LoginBootstrapPackets[4].Opcode != SMSG_CATALOG_SHOP_OBTAIN_LICENSE)
                return false;
        }
    }

    uint32 routeCount = 0;
    if (!ReadCaptureUInt32(data, position, routeCount) || !routeCount || routeCount > 64)
        return false;

    capture.CatalogShopRoutes.reserve(routeCount);
    for (uint32 routeIndex = 0; routeIndex < routeCount; ++routeIndex)
    {
        uint32 requestCount = 0;
        if (!ReadCaptureUInt32(data, position, requestCount) || !requestCount || requestCount > 4096)
            return false;

        CapturedCatalogShopRoute& route = capture.CatalogShopRoutes.emplace_back();
        route.RequestIDs.resize(requestCount);
        for (uint32& requestID : route.RequestIDs)
            if (!ReadCaptureUInt32(data, position, requestID))
                return false;

        route.SortedRequestIDs = route.RequestIDs;
        std::ranges::sort(route.SortedRequestIDs);

        uint32 responsePayloadSize = 0;
        if (!ReadCaptureUInt32(data, position, responsePayloadSize) ||
            !ReadCaptureBytes(data, position, responsePayloadSize, route.ResponsePayload))
            return false;

        if (formatVersion >= 8 &&
            (!ReadCaptureUInt32(data, position, route.ResponseDelayMs) || route.ResponseDelayMs > 60 * 1000))
            return false;
    }

    return position == data.size();
}

Optional<uint32> ReadOptionalUInt32(Field const& field)
{
    if (field.IsNull())
        return {};
    return field.GetUInt32();
}

bool HasDisplayContent(WorldPackets::RetailBattlePay::DisplayInfo const& display)
{
    return display.FileDataID || display.ModelSceneID || !display.Name1.empty() || !display.Name2.empty() ||
        !display.Name3.empty() || !display.Tooltip.empty() || !display.Instructions.empty() || display.Flags ||
        display.OverrideTextColor || display.OverrideTexture || display.OverrideBackground ||
        !display.Disclaimer.empty() || !display.NydusLink.empty();
}

bool ValidateDelivery(DeliveryAction const& action)
{
    if (!action.Entry || !action.Quantity)
        return false;

    switch (action.Type)
    {
        case DeliveryType::Item:
            return sObjectMgr->GetItemTemplate(action.Entry) != nullptr;
        case DeliveryType::Spell:
            return sSpellMgr->GetSpellInfo(action.Entry, DIFFICULTY_NONE) != nullptr;
        case DeliveryType::Mount:
            return sDB2Manager.GetMount(action.Entry) != nullptr;
        case DeliveryType::Toy:
            return sObjectMgr->GetItemTemplate(action.Entry) && sDB2Manager.IsToyItem(action.Entry);
    }

    return false;
}

bool IsAlreadyOwned(WorldSession* session, DeliveryAction const& action)
{
    Player* player = session ? session->GetPlayer() : nullptr;
    if (!player)
        return false;

    switch (action.Type)
    {
        case DeliveryType::Item:
            return false;
        case DeliveryType::Spell:
            return player->HasSpell(action.Entry);
        case DeliveryType::Mount:
            return session->GetCollectionMgr()->GetAccountMounts().contains(action.Entry) || player->HasSpell(action.Entry);
        case DeliveryType::Toy:
            return session->GetCollectionMgr()->HasToy(action.Entry);
    }

    return false;
}

bool CanDeliver(WorldSession* session, CatalogProduct const& product)
{
    Player* player = session ? session->GetPlayer() : nullptr;
    if (!player || product.Fulfillment.empty())
        return false;

    std::vector<std::unique_ptr<Item>> itemStorage;
    std::vector<Item*> itemPointers;
    itemStorage.reserve(product.Fulfillment.size());
    itemPointers.reserve(product.Fulfillment.size());

    for (DeliveryAction const& action : product.Fulfillment)
    {
        if (!ValidateDelivery(action) || IsAlreadyOwned(session, action))
            return false;

        if (action.Type != DeliveryType::Item)
            continue;

        std::unique_ptr<Item> item(Item::CreateItem(action.Entry, action.Quantity, ItemContext::NONE, player));
        if (!item)
            return false;

        itemPointers.push_back(item.get());
        itemStorage.push_back(std::move(item));
    }

    if (!itemPointers.empty())
    {
        uint32 offendingItemID = 0;
        if (player->CanStoreItems(itemPointers.data(), int(itemPointers.size()), &offendingItemID) != EQUIP_ERR_OK)
            return false;
    }

    return true;
}

bool Deliver(WorldSession* session, CatalogProduct const& product)
{
    Player* player = session ? session->GetPlayer() : nullptr;
    if (!player)
        return false;

    for (DeliveryAction const& action : product.Fulfillment)
    {
        switch (action.Type)
        {
            case DeliveryType::Item:
            {
                ItemPosCountVec destinations;
                if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destinations, action.Entry, action.Quantity) != EQUIP_ERR_OK)
                    return false;

                Item* item = player->StoreNewItem(destinations, action.Entry, true, GenerateItemRandomBonusListId(action.Entry));
                if (!item)
                    return false;

                player->SendNewItem(item, action.Quantity, true, true);
                break;
            }
            case DeliveryType::Spell:
                if (!player->HasSpell(action.Entry))
                    player->LearnSpell(action.Entry, false);
                break;
            case DeliveryType::Mount:
                if (!session->GetCollectionMgr()->AddMount(action.Entry, MOUNT_STATUS_NONE) &&
                    !session->GetCollectionMgr()->GetAccountMounts().contains(action.Entry))
                    return false;
                break;
            case DeliveryType::Toy:
                if (!session->GetCollectionMgr()->AddToy(action.Entry, false, true))
                    return false;
                break;
        }
    }

    return true;
}

bool ResolveFulfillment(uint32 productID, std::unordered_map<uint32, CatalogProduct>& products,
    std::unordered_map<uint32, uint8>& states)
{
    uint8& state = states[productID];
    if (state == 2)
        return !products.at(productID).Fulfillment.empty();
    if (state == 1)
    {
        TC_LOG_ERROR("server.battlepay", "RetailSystems: circular BattlePay bundle detected at product {}", productID);
        return false;
    }

    state = 1;
    CatalogProduct& product = products.at(productID);
    for (uint32 bundledProductID : product.Packet.BundledProductIDs)
    {
        auto bundled = products.find(bundledProductID);
        if (bundled == products.end() || !ResolveFulfillment(bundledProductID, products, states))
        {
            product.Fulfillment.clear();
            state = 2;
            return false;
        }

        product.Fulfillment.insert(product.Fulfillment.end(), bundled->second.Fulfillment.begin(), bundled->second.Fulfillment.end());
    }

    state = 2;
    return !product.Fulfillment.empty();
}

uint64 GetBalance(uint32 accountID)
{
    if (QueryResult result = LoginDatabase.PQuery(
        "SELECT `balance` FROM `retail_battlepay_balance` WHERE `account_id` = {}", accountID))
    {
        return result->Fetch()[0].GetUInt64();
    }

    return 0;
}

uint64 GeneratePurchaseID()
{
    uint64 milliseconds = uint64(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    return (milliseconds << 16) | (PurchaseSequence.fetch_add(1, std::memory_order_relaxed) & 0xFFFF);
}

WorldPackets::RetailBattlePay::Purchase ToPacketPurchase(PendingPurchase const& pending, PurchaseStatus status, Error result)
{
    WorldPackets::RetailBattlePay::Purchase purchase;
    purchase.PurchaseID = pending.PurchaseID;
    purchase.Status = uint32(status);
    purchase.ResultCode = uint32(result);
    purchase.ProductID = pending.ProductID;
    purchase.BasePrice = pending.BasePrice;
    purchase.UserPrice = pending.CurrentPrice;
    purchase.TimeCreated = pending.CreatedAt;
    purchase.WalletName = WalletName;
    return purchase;
}

void SendStartResult(WorldSession* session, uint32 clientToken, uint64 purchaseID, Error result)
{
    WorldPackets::RetailBattlePay::StartPurchaseResponse response;
    response.PurchaseResult = uint32(result);
    response.ClientToken = clientToken;
    response.PurchaseID = purchaseID;
    session->SendPacket(response.Write());
}

void SendUpdate(WorldSession* session, PendingPurchase const& pending, PurchaseStatus status, Error result)
{
    WorldPackets::RetailBattlePay::PurchaseUpdate update;
    update.Purchases.push_back(ToPacketPurchase(pending, status, result));
    session->SendPacket(update.Write());
}

bool GetProduct(uint32 productID, CatalogProduct& product)
{
    std::shared_lock lock(CatalogLock);
    auto itr = Products.find(productID);
    if (itr == Products.end())
        return false;
    product = itr->second;
    return true;
}

void RefundFailedDelivery(PendingPurchase const& pending)
{
    LoginDatabaseTransaction transaction = LoginDatabase.BeginTransaction();
    transaction->PAppend(
        "UPDATE `retail_battlepay_balance` SET `balance` = `balance` + {} WHERE `account_id` = {}",
        pending.CurrentPrice, pending.AccountID);
    transaction->PAppend(
        "UPDATE `retail_battlepay_purchase` SET `status` = {}, `result_code` = {} WHERE `purchase_id` = {}",
        uint32(PurchaseStatus::Finish), uint32(Error::PaymentFailed), pending.PurchaseID);
    LoginDatabase.DirectCommitTransaction(transaction);
}
}

void Initialize()
{
    bool enabled = sConfigMgr->GetBoolDefault("RetailSystems.BattlePay.Enabled", false, true);
    uint32 currencyID = uint32(std::max(1, sConfigMgr->GetIntDefault("RetailSystems.BattlePay.CurrencyID", 1, true)));
    uint32 catalogShopLicenseID = uint32(std::max(1,
        sConfigMgr->GetIntDefault("RetailSystems.BattlePay.CatalogShopLicenseID", 0x0011A691, true)));
    std::string walletName = sConfigMgr->GetStringDefault("RetailSystems.BattlePay.WalletName", "Battle Coins", true);
    std::string captureFile = sConfigMgr->GetStringDefault("RetailSystems.BattlePay.CaptureFile", "", true);
    if (walletName.empty())
        walletName = "Battle Coins";

    CapturedCatalogData capturedCatalog;
    if (enabled && !captureFile.empty())
    {
        if (LoadCapturedCatalog(captureFile, capturedCatalog))
        {
            catalogShopLicenseID = capturedCatalog.LicenseID;
            TC_LOG_INFO("server.battlepay", "RetailSystems: loaded BattlePay capture {} for client build {} "
                "(catalog version {}, {} product bytes, {} bootstrap packets, {} store gates, {} SSO bytes, "
                "{} DB query routes, {} available-hotfix bytes, {} hotfix-connect bytes/{} pushes, {} RPC timings, "
                "{} login-bootstrap packets, "
                "{} CatalogShop routes)", captureFile,
                capturedCatalog.ClientBuild, capturedCatalog.CatalogVersion,
                capturedCatalog.ProductListPayload.size(), capturedCatalog.PreProductPackets.size(),
                capturedCatalog.StoreGateRoutes.size(), capturedCatalog.OpenCheckoutResponsePayload.size(),
                capturedCatalog.DBQueryRoutes.size(), capturedCatalog.AvailableHotfixesPayload.size(),
                capturedCatalog.HotfixConnectPayload.size(), capturedCatalog.HotfixPushIDs.size(),
                capturedCatalog.RpcResponseDelays.size(),
                capturedCatalog.LoginBootstrapPackets.size(),
                capturedCatalog.CatalogShopRoutes.size());
        }
        else
        {
            TC_LOG_ERROR("server.battlepay", "RetailSystems: failed to load BattlePay capture {}", captureFile);
        }
    }

    std::vector<WorldPackets::RetailBattlePay::Group> groups;
    std::unordered_map<uint32, CatalogProduct> products;
    std::vector<std::pair<uint32, uint32>> orderedProducts;

    if (enabled)
    {
        if (QueryResult result = WorldDatabase.Query(
            "SELECT `group_id`, `icon_file_data_id`, `display_type`, `ordering`, `flags`, `parent_group_id`, "
            "`name`, `disabled_description` FROM `retail_battlepay_group` WHERE `active` = 1 ORDER BY `ordering`, `group_id`"))
        {
            do
            {
                Field* fields = result->Fetch();
                WorldPackets::RetailBattlePay::Group& group = groups.emplace_back();
                group.GroupID = fields[0].GetUInt32();
                group.IconFileDataID = fields[1].GetUInt32();
                group.DisplayType = fields[2].GetUInt8();
                group.Ordering = fields[3].GetUInt32();
                group.Flags = fields[4].GetUInt32();
                group.ParentGroupID = fields[5].GetUInt32();
                group.Name = fields[6].GetString();
                group.DisabledDescription = fields[7].GetString();
            } while (result->NextRow());
        }

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `product_id`, `group_id`, `normal_price`, `current_price`, `product_type`, `flags`, "
            "`required_deliverable_id`, `eligibility`, `pmt_product_id`, `ordering`, `shop_flags`, `banner_type`, "
            "`display_file_data_id`, `display_model_scene_id`, COALESCE(`name_1`, ''), COALESCE(`name_2`, ''), "
            "COALESCE(`name_3`, ''), COALESCE(`tooltip`, ''), COALESCE(`instructions`, ''), `display_flags`, "
            "`override_text_color`, `override_texture`, `override_background`, COALESCE(`disclaimer`, ''), "
            "COALESCE(`nydus_link`, ''), `battlepay_card_type`, `item_quantity` "
            "FROM `retail_battlepay_product` WHERE `active` = 1 ORDER BY `ordering`, `product_id`"))
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 productID = fields[0].GetUInt32();
                CatalogProduct& product = products[productID];
                product.Packet.ProductID = productID;
                product.Packet.NormalPriceFixedPoint = fields[2].GetUInt64();
                product.Packet.CurrentPriceFixedPoint = fields[3].GetUInt64();
                product.Packet.Type = fields[4].GetUInt32();
                product.Packet.Flags = fields[5].GetUInt32();
                product.Packet.RequiredDeliverableID = fields[6].GetUInt32();
                product.Packet.Eligibility = fields[7].GetUInt32();
                product.Packet.PmtProductID = fields[8].GetUInt64() ? fields[8].GetUInt64() : productID;

                WorldPackets::RetailBattlePay::DisplayInfo display;
                display.FileDataID = ReadOptionalUInt32(fields[12]);
                display.ModelSceneID = ReadOptionalUInt32(fields[13]);
                display.Name1 = fields[14].GetString();
                display.Name2 = fields[15].GetString();
                display.Name3 = fields[16].GetString();
                display.Tooltip = fields[17].GetString();
                display.Instructions = fields[18].GetString();
                display.Flags = ReadOptionalUInt32(fields[19]);
                display.OverrideTextColor = ReadOptionalUInt32(fields[20]);
                display.OverrideTexture = ReadOptionalUInt32(fields[21]);
                display.OverrideBackground = ReadOptionalUInt32(fields[22]);
                display.Disclaimer = fields[23].GetString();
                display.NydusLink = fields[24].GetString();
                display.BattlepayCardType = fields[25].GetUInt32();
                display.BannerType = fields[11].GetUInt8();
                display.ItemQuantity = fields[26].GetUInt32();
                if (HasDisplayContent(display))
                {
                    // Retail sends display data on both the catalog product and its shop entry.
                    // The 12.x client does not build a storefront card from Product::Display alone.
                    product.Packet.Display = display;
                    product.Shop.Display = std::move(display);
                }

                product.Shop.EntryID = productID;
                product.Shop.GroupID = fields[1].GetUInt32();
                product.Shop.ProductID = productID;
                product.Shop.Ordering = fields[9].GetUInt32();
                product.Shop.Flags = fields[10].GetUInt32();
                product.Shop.BannerType = fields[11].GetUInt8();
                orderedProducts.emplace_back(product.Shop.Ordering, productID);
            } while (result->NextRow());
        }

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `deliverable_id`, `product_id`, `client_type`, `delivery_type`, `entry`, `quantity`, `flags`, "
            "`name`, `item_id`, `mount_spell_id`, `battle_pet_creature_id`, `boost_id`, "
            "`trans_item_modified_appearance_id`, `transmog_set_id`, `char_title_id`, `spell_item_enchantment_id`, "
            "`warband_scene_id`, `display_file_data_id`, `display_model_scene_id`, `display_name`, `display_tooltip` "
            "FROM `retail_battlepay_deliverable` WHERE `active` = 1 ORDER BY `product_id`, `ordering`, `deliverable_id`"))
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 productID = fields[1].GetUInt32();
                auto productItr = products.find(productID);
                if (productItr == products.end())
                    continue;

                uint8 rawDeliveryType = fields[3].GetUInt8();
                if (rawDeliveryType > uint8(DeliveryType::Toy))
                {
                    TC_LOG_ERROR("server.battlepay", "RetailSystems: BattlePay deliverable {} has invalid delivery type {}",
                        fields[0].GetUInt32(), rawDeliveryType);
                    continue;
                }

                CatalogDeliverable deliverable;
                deliverable.Action.DeliverableID = fields[0].GetUInt32();
                deliverable.Action.Type = DeliveryType(rawDeliveryType);
                deliverable.Action.Entry = fields[4].GetUInt32();
                deliverable.Action.Quantity = fields[5].GetUInt32();
                if (!ValidateDelivery(deliverable.Action))
                {
                    TC_LOG_ERROR("server.battlepay", "RetailSystems: BattlePay deliverable {} references invalid entry {}",
                        deliverable.Action.DeliverableID, deliverable.Action.Entry);
                    continue;
                }

                WorldPackets::RetailBattlePay::Deliverable& packet = deliverable.Packet;
                packet.DeliverableID = deliverable.Action.DeliverableID;
                packet.Type = fields[2].GetUInt32();
                packet.Quantity = deliverable.Action.Quantity;
                packet.Flags = fields[6].GetUInt32();
                packet.Name = fields[7].GetString();
                packet.ItemID = fields[8].GetUInt32();
                packet.MountSpellID = fields[9].GetUInt32();
                packet.BattlePetCreatureID = fields[10].GetUInt32();
                packet.BoostID = fields[11].GetUInt32();
                packet.TransItemModifiedAppearanceID = fields[12].GetUInt32();
                packet.TransmogSetID = fields[13].GetUInt32();
                packet.CharTitleID = fields[14].GetUInt32();
                packet.SpellItemEnchantmentID = fields[15].GetUInt32();
                packet.WarbandSceneID = fields[16].GetUInt32();

                if (!packet.ItemID && (deliverable.Action.Type == DeliveryType::Item || deliverable.Action.Type == DeliveryType::Toy))
                    packet.ItemID = deliverable.Action.Entry;
                if (!packet.MountSpellID && deliverable.Action.Type == DeliveryType::Mount)
                    packet.MountSpellID = deliverable.Action.Entry;

                WorldPackets::RetailBattlePay::DisplayInfo display;
                display.FileDataID = ReadOptionalUInt32(fields[17]);
                display.ModelSceneID = ReadOptionalUInt32(fields[18]);
                display.Name1 = fields[19].GetString();
                display.Tooltip = fields[20].GetString();
                display.ItemQuantity = deliverable.Action.Quantity;
                if (HasDisplayContent(display))
                    packet.Display = std::move(display);

                productItr->second.Packet.DeliverableIDs.push_back(packet.DeliverableID);
                productItr->second.Fulfillment.push_back(deliverable.Action);
                productItr->second.Deliverables.push_back(std::move(deliverable));
            } while (result->NextRow());
        }

        if (QueryResult result = WorldDatabase.Query(
            "SELECT `product_id`, `bundled_product_id` FROM `retail_battlepay_product_bundle` ORDER BY `product_id`, `ordering`"))
        {
            do
            {
                Field* fields = result->Fetch();
                auto productItr = products.find(fields[0].GetUInt32());
                if (productItr != products.end() && products.contains(fields[1].GetUInt32()))
                    productItr->second.Packet.BundledProductIDs.push_back(fields[1].GetUInt32());
            } while (result->NextRow());
        }

        std::unordered_map<uint32, uint8> fulfillmentStates;
        fulfillmentStates.reserve(products.size());
        for (auto const& [productID, product] : products)
        {
            (void)product;
            ResolveFulfillment(productID, products, fulfillmentStates);
        }

        std::sort(orderedProducts.begin(), orderedProducts.end());
    }

    std::vector<uint32> productOrder;
    productOrder.reserve(orderedProducts.size());
    for (auto const& [ordering, productID] : orderedProducts)
    {
        (void)ordering;
        auto itr = products.find(productID);
        if (itr == products.end() || itr->second.Fulfillment.empty())
        {
            products.erase(productID);
            continue;
        }
        productOrder.push_back(productID);
    }

    {
        std::unique_lock lock(CatalogLock);
        CurrencyID = currencyID;
        WalletName = std::move(walletName);
        Groups = std::move(groups);
        Products = std::move(products);
        ProductOrder = std::move(productOrder);
        CapturedCatalog = std::move(capturedCatalog);
    }

    CatalogShopLicenseID.store(catalogShopLicenseID, std::memory_order_release);
    Enabled.store(enabled, std::memory_order_release);
    TC_LOG_INFO("server.battlepay", "RetailSystems: BattlePay {} with {} groups and {} products",
        enabled ? "enabled" : "disabled", Groups.size(), ProductOrder.size());
}

bool IsEnabled()
{
    return Enabled.load(std::memory_order_acquire);
}

bool HasCapturedCatalog()
{
    std::shared_lock lock(CatalogLock);
    return !CapturedCatalog.ProductListPayload.empty();
}

void SendCapturedPacket(WorldSession* session, WorldPacket const* packet, uint32 delayMs)
{
    if (!session || !packet)
        return;

    if (!delayMs)
    {
        session->SendPacket(packet);
        return;
    }

    DelayedCapturedPacket delayed;
    delayed.DueAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
    delayed.Sequence = DelayedPacketSequence.fetch_add(1, std::memory_order_relaxed);
    delayed.Opcode = packet->GetOpcode();
    delayed.Connection = packet->GetConnection();
    if (packet->size())
        delayed.Payload.assign(packet->data(), packet->data() + packet->size());

    std::lock_guard lock(DelayedCapturedPacketsLock);
    DelayedCapturedPackets[session].push_back(std::move(delayed));
}

void ProcessDelayedPackets(WorldSession* session)
{
    if (!session)
        return;

    std::vector<DelayedCapturedPacket> ready;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    {
        std::lock_guard lock(DelayedCapturedPacketsLock);
        auto sessionPackets = DelayedCapturedPackets.find(session);
        if (sessionPackets == DelayedCapturedPackets.end())
            return;

        for (auto itr = sessionPackets->second.begin(); itr != sessionPackets->second.end();)
        {
            if (itr->DueAt <= now)
            {
                ready.push_back(std::move(*itr));
                itr = sessionPackets->second.erase(itr);
            }
            else
                ++itr;
        }

        if (sessionPackets->second.empty())
            DelayedCapturedPackets.erase(sessionPackets);
    }

    std::ranges::sort(ready, [](DelayedCapturedPacket const& left, DelayedCapturedPacket const& right)
    {
        if (left.DueAt != right.DueAt)
            return left.DueAt < right.DueAt;
        return left.Sequence < right.Sequence;
    });

    for (DelayedCapturedPacket const& delayed : ready)
    {
        WorldPacket packet(delayed.Opcode, delayed.Payload.size(), delayed.Connection);
        if (!delayed.Payload.empty())
            packet.append(delayed.Payload.data(), delayed.Payload.size());
        session->SendPacket(&packet);
    }
}

void ClearDelayedPackets(WorldSession* session)
{
    std::lock_guard lock(DelayedCapturedPacketsLock);
    DelayedCapturedPackets.erase(session);
}

bool GetCapturedDBReply(uint32 tableHash, uint32 recordID, uint8& status, std::vector<uint8>& data, uint32& delayMs)
{
    std::shared_lock lock(CatalogLock);
    auto route = std::ranges::find_if(CapturedCatalog.DBQueryRoutes,
        [tableHash](CapturedDBQueryRoute const& candidate)
        {
            return candidate.TableHash == tableHash;
        });
    if (route == CapturedCatalog.DBQueryRoutes.end())
        return false;

    if (auto reply = route->Replies.find(recordID); reply != route->Replies.end())
    {
        status = reply->second.Status;
        data = reply->second.Data;
        delayMs = route->ResponseDelayMs;
        return true;
    }

    if (route->DefaultStatus == std::numeric_limits<uint32>::max())
        return false;

    status = uint8(route->DefaultStatus);
    data.clear();
    delayMs = route->ResponseDelayMs;
    return true;
}

bool SendCapturedAvailableHotfixes(WorldSession* session)
{
    if (!session)
        return false;

    std::vector<uint8> responsePayload;
    {
        std::shared_lock lock(CatalogLock);
        if (CapturedCatalog.AvailableHotfixesPayload.size() < 8)
            return false;
        responsePayload = CapturedCatalog.AvailableHotfixesPayload;
    }

    WorldPacket capturedResponse(SMSG_AVAILABLE_HOTFIXES, responsePayload.size());
    capturedResponse.append(responsePayload.data(), responsePayload.size());
    session->SendPacket(&capturedResponse);
    return true;
}

bool SendCapturedHotfixConnect(WorldSession* session, std::vector<int32> const& requestedPushIDs)
{
    if (!session)
        return false;

    std::vector<uint8> responsePayload;
    std::vector<int32> expectedPushIDs;
    uint32 delayMs = 0;
    {
        std::shared_lock lock(CatalogLock);
        if (CapturedCatalog.HotfixConnectPayload.size() < 8)
            return false;
        responsePayload = CapturedCatalog.HotfixConnectPayload;
        expectedPushIDs = CapturedCatalog.HotfixPushIDs;
        delayMs = CapturedCatalog.HotfixConnectDelayMs;
    }

    std::vector<int32> sortedRequestedPushIDs = requestedPushIDs;
    std::ranges::sort(sortedRequestedPushIDs);
    if (sortedRequestedPushIDs != expectedPushIDs)
        TC_LOG_ERROR("server.battlepay", "RetailSystems: captured BattlePay hotfix route expects {} push IDs, "
            "but client requested {} (hotfix cache namespace/state mismatch)",
            expectedPushIDs.size(), sortedRequestedPushIDs.size());
    else
        TC_LOG_INFO("server.battlepay", "RetailSystems: matched captured BattlePay hotfix route ({} push IDs)",
            expectedPushIDs.size());

    WorldPacket capturedResponse(SMSG_HOTFIX_CONNECT, responsePayload.size());
    capturedResponse.append(responsePayload.data(), responsePayload.size());
    SendCapturedPacket(session, &capturedResponse, delayMs);
    return true;
}

uint32 GetCapturedRpcDelay(uint32 serviceHash, uint32 methodId)
{
    std::shared_lock lock(CatalogLock);
    auto itr = CapturedCatalog.RpcResponseDelays.find((uint64(serviceHash) << 32) | methodId);
    return itr != CapturedCatalog.RpcResponseDelays.end() ? itr->second : 0;
}

bool SendCapturedOpenCheckout(WorldSession* session, uint32 clientToken)
{
    if (!session)
        return false;

    std::vector<uint8> responsePayload;
    uint32 delayMs = 0;
    {
        std::shared_lock lock(CatalogLock);
        if (CapturedCatalog.OpenCheckoutResponsePayload.size() < 25)
            return false;
        responsePayload = CapturedCatalog.OpenCheckoutResponsePayload;
        delayMs = CapturedCatalog.OpenCheckoutResponseDelayMs;
    }

    auto readUInt64 = [&responsePayload](std::size_t offset)
    {
        uint64 value = 0;
        for (uint8 byteIndex = 0; byteIndex < 8; ++byteIndex)
            value |= uint64(responsePayload[offset + byteIndex]) << (byteIndex * 8);
        return value;
    };
    auto writeUInt32 = [&responsePayload](std::size_t offset, uint32 value)
    {
        for (uint8 byteIndex = 0; byteIndex < 4; ++byteIndex)
            responsePayload[offset + byteIndex] = uint8(value >> (byteIndex * 8));
    };
    auto writeUInt64 = [&responsePayload](std::size_t offset, uint64 value)
    {
        for (uint8 byteIndex = 0; byteIndex < 8; ++byteIndex)
            responsePayload[offset + byteIndex] = uint8(value >> (byteIndex * 8));
    };

    uint64 capturedIssuedAt = readUInt64(8);
    uint64 capturedExpiresAt = readUInt64(16);
    uint64 lifetime = capturedExpiresAt > capturedIssuedAt ? capturedExpiresAt - capturedIssuedAt : 4 * 60 * 60;
    if (lifetime > 24 * 60 * 60)
        lifetime = 4 * 60 * 60;

    uint64 now = uint64(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    writeUInt32(0, clientToken);
    writeUInt64(8, now);
    writeUInt64(16, now + lifetime);

    WorldPacket capturedResponse(SMSG_GENERATE_SSO_TOKEN_RESPONSE, responsePayload.size());
    capturedResponse.append(responsePayload.data(), responsePayload.size());
    SendCapturedPacket(session, &capturedResponse, delayMs);
    return true;
}

void SendLastCatalogFetch(WorldSession* session)
{
    if (!session)
        return;

    WorldPackets::RetailBattlePay::LastCatalogFetchResponse response;
    uint32 delayMs = 0;
    {
        std::shared_lock lock(CatalogLock);
        if (CapturedCatalog.CatalogVersion)
        {
            response.LastCatalogFetch = CapturedCatalog.CatalogVersion;
            if (!CapturedCatalog.StoreGateRoutes.empty())
                delayMs = CapturedCatalog.StoreGateRoutes.front().ResponseDelayMs;
            SendCapturedPacket(session, response.Write(), delayMs);
            return;
        }
    }

    if (QueryResult result = LoginDatabase.PQuery(
        "SELECT `last_fetch` FROM `retail_battlepay_catalog_fetch` WHERE `account_id` = {}", session->GetAccountId()))
    {
        response.LastCatalogFetch = result->Fetch()[0].GetUInt64();
    }

    session->SendPacket(response.Write());
}

void UpdateLastCatalogFetch(uint32 accountID)
{
    {
        std::shared_lock lock(CatalogLock);
        if (CapturedCatalog.CatalogVersion)
            return;
    }

    uint64 now = uint64(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    LoginDatabase.DirectPExecute(
        "INSERT INTO `retail_battlepay_catalog_fetch` (`account_id`, `last_fetch`) VALUES ({}, {}) "
        "ON DUPLICATE KEY UPDATE `last_fetch` = VALUES(`last_fetch`)", accountID, now);
}

void SendProductList(WorldSession* session)
{
    if (!session)
        return;

    WorldPackets::RetailBattlePay::ProductListResponse response;
    if (!IsEnabled())
    {
        response.Result = 1; // ProductListResult::Unavailable
        session->SendPacket(response.Write());
        return;
    }

    std::shared_lock lock(CatalogLock);
    if (!CapturedCatalog.ProductListPayload.empty())
    {
        for (CapturedBootstrapPacket const& bootstrap : CapturedCatalog.PreProductPackets)
        {
            WorldPacket capturedBootstrap(bootstrap.Opcode, bootstrap.Payload.size());
            capturedBootstrap.append(bootstrap.Payload.data(), bootstrap.Payload.size());
            SendCapturedPacket(session, &capturedBootstrap, bootstrap.DelayMs);
        }

        WorldPacket capturedResponse(SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE,
            CapturedCatalog.ProductListPayload.size());
        capturedResponse.append(CapturedCatalog.ProductListPayload.data(), CapturedCatalog.ProductListPayload.size());
        SendCapturedPacket(session, &capturedResponse, CapturedCatalog.ProductResponseDelayMs);

        WorldPackets::RetailBattlePay::LastCatalogFetchResponse lastCatalogFetch;
        lastCatalogFetch.LastCatalogFetch = CapturedCatalog.CatalogVersion;
        SendCapturedPacket(session, lastCatalogFetch.Write(), CapturedCatalog.LastCatalogResponseDelayMs);
        return;
    }

    response.CurrencyID = CurrencyID;
    response.Groups = Groups;
    for (uint32 productID : ProductOrder)
    {
        CatalogProduct const& catalogProduct = Products.at(productID);
        response.Products.push_back(catalogProduct.Packet);
        response.Shops.push_back(catalogProduct.Shop);
        for (CatalogDeliverable const& catalogDeliverable : catalogProduct.Deliverables)
        {
            WorldPackets::RetailBattlePay::Deliverable deliverable = catalogDeliverable.Packet;
            deliverable.AlreadyOwns = IsAlreadyOwned(session, catalogDeliverable.Action);
            response.Deliverables.push_back(std::move(deliverable));
        }
    }

    session->SendPacket(response.Write());
}

void SendPurchaseList(WorldSession* session)
{
    if (!session)
        return;

    WorldPackets::RetailBattlePay::PurchaseListResponse response;
    if (!IsEnabled())
    {
        response.Result = 1; // ProductListResult::Unavailable
        session->SendPacket(response.Write());
        return;
    }

    if (QueryResult result = LoginDatabase.PQuery(
        "SELECT `purchase_id`, `status`, `result_code`, `product_id`, `base_price`, `user_price`, `created_at`, "
        "`wallet_name` FROM `retail_battlepay_purchase` WHERE `account_id` = {} ORDER BY `purchase_id` DESC LIMIT 100",
        session->GetAccountId()))
    {
        do
        {
            Field* fields = result->Fetch();
            WorldPackets::RetailBattlePay::Purchase& purchase = response.Purchases.emplace_back();
            purchase.PurchaseID = fields[0].GetUInt64();
            purchase.Status = fields[1].GetUInt32();
            purchase.ResultCode = fields[2].GetUInt32();
            purchase.ProductID = fields[3].GetUInt32();
            purchase.BasePrice = fields[4].GetUInt64();
            purchase.UserPrice = fields[5].GetUInt64();
            purchase.TimeCreated = fields[6].GetUInt64();
            purchase.WalletName = fields[7].GetString();
        } while (result->NextRow());
    }

    session->SendPacket(response.Write());
}

void SendDistributionList(WorldSession* session)
{
    if (!session)
        return;

    WorldPackets::RetailBattlePay::DistributionListResponse response;
    response.Result = IsEnabled() ? uint32(Error::Ok) : uint32(Error::BattlePayDisabled);
    session->SendPacket(response.Write());
}

void SendCatalogShopLicense(WorldSession* session)
{
    if (!session || !IsEnabled())
        return;

    WorldPackets::RetailBattlePay::CatalogShopObtainLicense response;
    response.LicenseID = CatalogShopLicenseID.load(std::memory_order_acquire);
    session->SendPacket(response.Write());
}

void SendLoginBootstrap(WorldSession* session)
{
    if (!session)
        return;

    std::shared_lock lock(CatalogLock);
    if (!CapturedCatalog.LoginBootstrapPackets.empty())
    {
        for (CapturedBootstrapPacket const& bootstrap : CapturedCatalog.LoginBootstrapPackets)
        {
            WorldPacket capturedPacket(bootstrap.Opcode, bootstrap.Payload.size());
            capturedPacket.append(bootstrap.Payload.data(), bootstrap.Payload.size());
            SendCapturedPacket(session, &capturedPacket, bootstrap.DelayMs);
        }
        return;
    }

    lock.unlock();
    SendDistributionList(session);
    SendCatalogShopLicense(session);
}

bool SendCapturedCatalogShopLicenseData(WorldSession* session, std::vector<uint32> const& gameDataIDs)
{
    if (!session || gameDataIDs.empty())
        return false;

    std::vector<uint32> sortedRequestIDs = gameDataIDs;
    std::ranges::sort(sortedRequestIDs);

    std::shared_lock lock(CatalogLock);
    auto route = std::ranges::find_if(CapturedCatalog.CatalogShopRoutes,
        [&sortedRequestIDs](CapturedCatalogShopRoute const& candidate)
        {
            return candidate.SortedRequestIDs == sortedRequestIDs;
        });
    if (route == CapturedCatalog.CatalogShopRoutes.end())
        return false;

    WorldPacket capturedResponse(SMSG_CATALOG_SHOP_LICENSE_DATA, route->ResponsePayload.size());
    capturedResponse.append(route->ResponsePayload.data(), route->ResponsePayload.size());
    SendCapturedPacket(session, &capturedResponse, route->ResponseDelayMs);
    return true;
}

bool SendCapturedStoreGateResponse(WorldSession* session, uint32 requestOpcode, uint32 clientToken)
{
    if (!session)
        return false;

    std::vector<uint8> responsePayload;
    uint32 responseOpcode = 0;
    uint32 delayMs = 0;
    {
        std::shared_lock lock(CatalogLock);
        auto route = std::ranges::find_if(CapturedCatalog.StoreGateRoutes,
            [requestOpcode](CapturedStoreGateRoute const& candidate)
            {
                return candidate.RequestOpcode == requestOpcode;
            });
        if (route == CapturedCatalog.StoreGateRoutes.end())
            return false;

        responseOpcode = route->ResponseOpcode;
        responsePayload = route->ResponsePayload;
        delayMs = route->ResponseDelayMs;
    }

    if (requestOpcode == CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE ||
        requestOpcode == CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY)
    {
        responsePayload[0] = uint8(clientToken);
        responsePayload[1] = uint8(clientToken >> 8);
        responsePayload[2] = uint8(clientToken >> 16);
        responsePayload[3] = uint8(clientToken >> 24);
    }

    WorldPacket capturedResponse(responseOpcode, responsePayload.size());
    capturedResponse.append(responsePayload.data(), responsePayload.size());
    SendCapturedPacket(session, &capturedResponse, delayMs);
    return true;
}

void StartPurchase(WorldSession* session, uint32 clientToken, uint32 productID, ObjectGuid targetCharacter, uint32 /*unknown*/)
{
    if (!session || !IsEnabled())
    {
        if (session)
            SendStartResult(session, clientToken, 0, Error::BattlePayDisabled);
        return;
    }

    Player* player = session->GetPlayer();
    if (!player)
    {
        SendStartResult(session, clientToken, 0, Error::PurchaseDenied);
        return;
    }

    if (targetCharacter.IsEmpty())
        targetCharacter = player->GetGUID();
    if (targetCharacter != player->GetGUID())
    {
        SendStartResult(session, clientToken, 0, Error::PurchaseDenied);
        return;
    }

    CatalogProduct product;
    if (!GetProduct(productID, product) || !CanDeliver(session, product))
    {
        SendStartResult(session, clientToken, 0, Error::PurchaseDenied);
        return;
    }

    uint64 balance = GetBalance(session->GetAccountId());
    if (balance < product.Packet.CurrentPriceFixedPoint)
    {
        SendStartResult(session, clientToken, 0, Error::InsufficientBalance);
        return;
    }

    PendingPurchase pending;
    pending.PurchaseID = GeneratePurchaseID();
    pending.AccountID = session->GetAccountId();
    pending.ClientToken = clientToken;
    pending.ServerToken = urand(1, 0x0FFFFFFF);
    pending.ProductID = productID;
    pending.TargetCharacter = targetCharacter;
    pending.BasePrice = product.Packet.NormalPriceFixedPoint;
    pending.CurrentPrice = product.Packet.CurrentPriceFixedPoint;
    pending.CreatedAt = uint64(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    pending.ExpiresAt = std::chrono::steady_clock::now() + std::chrono::minutes(2);

    {
        std::lock_guard lock(PendingLock);
        PendingPurchases[pending.AccountID] = pending;
    }

    SendStartResult(session, clientToken, pending.PurchaseID, Error::Ok);
    SendUpdate(session, pending, PurchaseStatus::Loading, Error::Ok);

    WorldPackets::RetailBattlePay::ConfirmPurchase confirm;
    confirm.PurchaseID = pending.PurchaseID;
    confirm.ServerToken = pending.ServerToken;
    session->SendPacket(confirm.Write());
}

void ConfirmPurchase(WorldSession* session, uint32 serverToken, uint64 clientCurrentPriceFixedPoint, bool confirmed)
{
    if (!session || !IsEnabled())
        return;

    PendingPurchase pending;
    {
        std::lock_guard lock(PendingLock);
        auto itr = PendingPurchases.find(session->GetAccountId());
        if (itr == PendingPurchases.end())
            return;

        pending = itr->second;
        if (pending.ServerToken != serverToken)
            return;

        PendingPurchases.erase(itr);
    }

    if (!confirmed || clientCurrentPriceFixedPoint != pending.CurrentPrice ||
        std::chrono::steady_clock::now() > pending.ExpiresAt)
    {
        SendUpdate(session, pending, PurchaseStatus::Finish, Error::PurchaseDenied);
        return;
    }

    Player* player = session->GetPlayer();
    CatalogProduct product;
    if (!player || player->GetGUID() != pending.TargetCharacter || !GetProduct(pending.ProductID, product) ||
        product.Packet.CurrentPriceFixedPoint != pending.CurrentPrice || !CanDeliver(session, product))
    {
        SendUpdate(session, pending, PurchaseStatus::Finish, Error::PurchaseDenied);
        return;
    }

    LoginDatabase.DirectPExecute(
        "INSERT IGNORE INTO `retail_battlepay_balance` (`account_id`, `balance`) VALUES ({}, 0)", pending.AccountID);
    if (GetBalance(pending.AccountID) < pending.CurrentPrice)
    {
        SendUpdate(session, pending, PurchaseStatus::Finish, Error::InsufficientBalance);
        return;
    }

    std::string escapedWalletName = WalletName;
    LoginDatabase.EscapeString(escapedWalletName);
    LoginDatabaseTransaction transaction = LoginDatabase.BeginTransaction();
    transaction->PAppend(
        "INSERT INTO `retail_battlepay_purchase` (`purchase_id`, `account_id`, `character_guid`, `product_id`, "
        "`base_price`, `user_price`, `status`, `result_code`, `created_at`, `wallet_name`) "
        "SELECT {}, {}, {}, {}, {}, {}, {}, {}, {}, '{}' FROM `retail_battlepay_balance` "
        "WHERE `account_id` = {} AND `balance` >= {}",
        pending.PurchaseID, pending.AccountID, pending.TargetCharacter.GetCounter(), pending.ProductID,
        pending.BasePrice, pending.CurrentPrice, uint32(PurchaseStatus::Loading), uint32(Error::Ok),
        pending.CreatedAt, escapedWalletName, pending.AccountID, pending.CurrentPrice);
    transaction->PAppend(
        "UPDATE `retail_battlepay_balance` SET `balance` = `balance` - {} "
        "WHERE `account_id` = {} AND EXISTS (SELECT 1 FROM `retail_battlepay_purchase` WHERE `purchase_id` = {})",
        pending.CurrentPrice, pending.AccountID, pending.PurchaseID);
    LoginDatabase.DirectCommitTransaction(transaction);

    if (!LoginDatabase.PQuery(
        "SELECT 1 FROM `retail_battlepay_purchase` WHERE `purchase_id` = {} AND `account_id` = {}",
        pending.PurchaseID, pending.AccountID))
    {
        SendUpdate(session, pending, PurchaseStatus::Finish, Error::PaymentFailed);
        return;
    }

    if (!Deliver(session, product))
    {
        RefundFailedDelivery(pending);
        SendUpdate(session, pending, PurchaseStatus::Finish, Error::PaymentFailed);
        TC_LOG_ERROR("server.battlepay", "RetailSystems: BattlePay delivery failed for purchase {} (account {}, product {})",
            pending.PurchaseID, pending.AccountID, pending.ProductID);
        return;
    }

    uint64 deliveredAt = uint64(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    LoginDatabase.PExecute(
        "UPDATE `retail_battlepay_purchase` SET `status` = {}, `result_code` = {}, `delivered_at` = {} "
        "WHERE `purchase_id` = {}",
        uint32(PurchaseStatus::Finish), uint32(Error::Ok), deliveredAt, pending.PurchaseID);

    SendUpdate(session, pending, PurchaseStatus::Finish, Error::Ok);
    SendProductList(session);
    TC_LOG_INFO("server.battlepay", "RetailSystems: completed BattlePay purchase {} for account {}, product {}",
        pending.PurchaseID, pending.AccountID, pending.ProductID);
}

void AcknowledgeFailure(WorldSession* session, uint32 serverToken)
{
    if (!session)
        return;

    std::lock_guard lock(PendingLock);
    auto itr = PendingPurchases.find(session->GetAccountId());
    if (itr != PendingPurchases.end() && itr->second.ServerToken == serverToken)
        PendingPurchases.erase(itr);
}

void ClearPending(uint32 accountID)
{
    std::lock_guard lock(PendingLock);
    PendingPurchases.erase(accountID);
}
}
