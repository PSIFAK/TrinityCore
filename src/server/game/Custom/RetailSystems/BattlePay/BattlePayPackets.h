/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_BATTLE_PAY_PACKETS_H
#define TRINITYCORE_RETAIL_BATTLE_PAY_PACKETS_H

#include "Packet.h"

namespace WorldPackets::RetailBattlePay
{
struct DisplayCard
{
    std::string Title;
    uint32 CreatureDisplayInfoID = 0;
    uint32 ModelSceneID = 0;
    uint32 TransmogSetID = 0;
};

struct DisplayInfo
{
    Optional<uint32> FileDataID;
    Optional<uint32> ModelSceneID;
    std::string Name1;
    std::string Name2;
    std::string Name3;
    std::string Tooltip;
    std::string Instructions;
    Optional<uint32> Flags;
    Optional<uint32> OverrideTextColor;
    Optional<uint32> OverrideTexture;
    Optional<uint32> OverrideBackground;
    std::string Disclaimer;
    std::string NydusLink;
    std::vector<DisplayCard> DisplayCards;
    uint32 BattlepayCardType = 0;
    uint32 BannerType = 0;
    uint32 ItemQuantity = 0;
};

struct DeliverableChoice
{
    uint32 ID = 0;
    uint32 Type = 0;
    uint32 ItemID = 0;
    uint32 Quantity = 0;
    uint32 MountSpellID = 0;
    uint32 BattlePetCreatureID = 0;
    bool AlreadyOwns = false;
    Optional<uint8> PetResult;
    Optional<DisplayInfo> Display;
};

struct Deliverable
{
    uint32 DeliverableID = 0;
    uint32 Type = 0;
    uint32 ItemID = 0;
    uint32 Quantity = 0;
    uint32 MountSpellID = 0;
    uint32 BattlePetCreatureID = 0;
    uint32 BoostID = 0;
    uint32 Flags = 0;
    uint32 TransItemModifiedAppearanceID = 0;
    uint32 TransmogSetID = 0;
    uint32 CharTitleID = 0;
    uint32 SpellItemEnchantmentID = 0;
    uint32 WarbandSceneID = 0;
    std::string Name;
    bool AlreadyOwns = false;
    Optional<uint8> PetResult;
    std::vector<DeliverableChoice> Choices;
    Optional<DisplayInfo> Display;
};

struct Product
{
    uint32 ProductID = 0;
    uint64 NormalPriceFixedPoint = 0;
    uint64 CurrentPriceFixedPoint = 0;
    std::vector<uint32> DeliverableIDs;
    uint32 Type = 0;
    uint32 Flags = 0;
    uint32 RequiredDeliverableID = 0;
    std::vector<uint32> BundledProductIDs;
    uint32 Eligibility = 0;
    uint64 PmtProductID = 0;
    Optional<DisplayInfo> Display;
};

struct Group
{
    uint32 GroupID = 0;
    uint32 IconFileDataID = 0;
    uint8 DisplayType = 0;
    uint32 Ordering = 0;
    uint32 Flags = 0;
    uint32 ParentGroupID = 0;
    std::string Name;
    std::string DisabledDescription;
};

struct ShopEntry
{
    uint32 EntryID = 0;
    uint32 GroupID = 0;
    uint32 ProductID = 0;
    uint32 Ordering = 0;
    uint32 Flags = 0;
    uint8 BannerType = 0;
    Optional<DisplayInfo> Display;
};

struct Purchase
{
    uint64 PurchaseID = 0;
    uint32 Status = 0;
    uint32 ResultCode = 0;
    uint32 ProductID = 0;
    uint64 BasePrice = 0;
    uint64 UserPrice = 0;
    uint64 TimeCreated = 0;
    std::string WalletName;
};

class GetProductList final : public ClientPacket
{
public:
    explicit GetProductList(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_GET_PRODUCT_LIST, std::move(packet)) { }
    void Read() override { }
};

class GetPurchaseList final : public ClientPacket
{
public:
    explicit GetPurchaseList(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_GET_PURCHASE_LIST, std::move(packet)) { }
    void Read() override { }
};

class OpenCheckout final : public ClientPacket
{
public:
    explicit OpenCheckout(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_OPEN_CHECKOUT, std::move(packet)) { }
    void Read() override { _worldPacket >> ClientToken; }

    uint32 ClientToken = 0;
};

class GetLastCatalogFetch final : public ClientPacket
{
public:
    explicit GetLastCatalogFetch(WorldPacket&& packet) : ClientPacket(CMSG_GET_LAST_CATALOG_FETCH, std::move(packet)) { }
    void Read() override { }
};

class UpdateLastCatalogFetch final : public ClientPacket
{
public:
    explicit UpdateLastCatalogFetch(WorldPacket&& packet) : ClientPacket(CMSG_UPDATE_LAST_CATALOG_FETCH, std::move(packet)) { }
    void Read() override { }
};

class CatalogShopLicenseGameDataRequest final : public ClientPacket
{
public:
    explicit CatalogShopLicenseGameDataRequest(WorldPacket&& packet)
        : ClientPacket(CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST, std::move(packet)) { }
    void Read() override;

    std::vector<uint32> GameDataIDs;
};

class CommerceTokenGetMarketPrice final : public ClientPacket
{
public:
    explicit CommerceTokenGetMarketPrice(WorldPacket&& packet)
        : ClientPacket(CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE, std::move(packet)) { }
    void Read() override;

    uint32 ClientToken = 0;
};

class ConsumableTokenCanVeteranBuy final : public ClientPacket
{
public:
    explicit ConsumableTokenCanVeteranBuy(WorldPacket&& packet)
        : ClientPacket(CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY, std::move(packet)) { }
    void Read() override;

    uint32 ClientToken = 0;
};

class GetDecorRefundList final : public ClientPacket
{
public:
    explicit GetDecorRefundList(WorldPacket&& packet) : ClientPacket(CMSG_GET_DECOR_REFUND_LIST, std::move(packet)) { }
    void Read() override { }
};

class GetAllLicensedDecorQuantities final : public ClientPacket
{
public:
    explicit GetAllLicensedDecorQuantities(WorldPacket&& packet)
        : ClientPacket(CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES, std::move(packet)) { }
    void Read() override { }
};

class RequestStoreFrontInfoUpdate final : public ClientPacket
{
public:
    explicit RequestStoreFrontInfoUpdate(WorldPacket&& packet)
        : ClientPacket(CMSG_REQUEST_STORE_FRONT_INFO_UPDATE, std::move(packet)) { }
    void Read() override;

    uint32 StoreFrontID = 0;
};

class StartPurchase final : public ClientPacket
{
public:
    explicit StartPurchase(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_START_PURCHASE, std::move(packet)) { }
    void Read() override;

    uint32 ClientToken = 0;
    uint32 ProductID = 0;
    ObjectGuid TargetCharacter;
    uint32 Unknown = 0;
};

class ConfirmPurchaseResponse final : public ClientPacket
{
public:
    explicit ConfirmPurchaseResponse(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE, std::move(packet)) { }
    void Read() override;

    uint32 ServerToken = 0;
    uint64 ClientCurrentPriceFixedPoint = 0;
    bool ConfirmPurchase = false;
};

class AckFailedResponse final : public ClientPacket
{
public:
    explicit AckFailedResponse(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_ACK_FAILED_RESPONSE, std::move(packet)) { }
    void Read() override;

    uint32 ServerToken = 0;
};

class RequestPriceInfo final : public ClientPacket
{
public:
    explicit RequestPriceInfo(WorldPacket&& packet) : ClientPacket(CMSG_BATTLE_PAY_REQUEST_PRICE_INFO, std::move(packet)) { }
    void Read() override { _worldPacket.rfinish(); }
};

class ProductListResponse final : public ServerPacket
{
public:
    ProductListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE) { }
    WorldPacket const* Write() override;

    uint32 Result = 0;
    uint32 CurrencyID = 1;
    std::vector<Product> Products;
    std::vector<Deliverable> Deliverables;
    std::vector<Group> Groups;
    std::vector<ShopEntry> Shops;
};

class PurchaseListResponse final : public ServerPacket
{
public:
    PurchaseListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_PURCHASE_LIST_RESPONSE) { }
    WorldPacket const* Write() override;

    uint32 Result = 0;
    std::vector<Purchase> Purchases;
};

class LastCatalogFetchResponse final : public ServerPacket
{
public:
    LastCatalogFetchResponse() : ServerPacket(SMSG_LAST_CATALOG_FETCH_RESPONSE, 8) { }
    WorldPacket const* Write() override;

    uint64 LastCatalogFetch = 0;
};

class CatalogShopObtainLicense final : public ServerPacket
{
public:
    CatalogShopObtainLicense() : ServerPacket(SMSG_CATALOG_SHOP_OBTAIN_LICENSE, 4) { }
    WorldPacket const* Write() override;

    uint32 LicenseID = 0;
};

class CatalogShopLicenseData final : public ServerPacket
{
public:
    CatalogShopLicenseData() : ServerPacket(SMSG_CATALOG_SHOP_LICENSE_DATA) { }
    WorldPacket const* Write() override;

    std::vector<uint32> MissingGameDataIDs;
};

class DecorRefundListResponse final : public ServerPacket
{
public:
    DecorRefundListResponse() : ServerPacket(SMSG_GET_DECOR_REFUND_LIST_RESPONSE, 4) { }
    WorldPacket const* Write() override;
};

class AllLicensedDecorQuantitiesResponse final : public ServerPacket
{
public:
    AllLicensedDecorQuantitiesResponse() : ServerPacket(SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE, 4) { }
    WorldPacket const* Write() override;
};

class AccountStoreFrontUpdate final : public ServerPacket
{
public:
    AccountStoreFrontUpdate() : ServerPacket(SMSG_ACCOUNT_STORE_FRONT_UPDATE, 14) { }
    WorldPacket const* Write() override;

    uint32 StoreFrontID = 0;
};

class DistributionListResponse final : public ServerPacket
{
public:
    DistributionListResponse() : ServerPacket(SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE, 6) { }
    WorldPacket const* Write() override;

    uint32 Result = 0;
};

class StartPurchaseResponse final : public ServerPacket
{
public:
    StartPurchaseResponse() : ServerPacket(SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE, 16) { }
    WorldPacket const* Write() override;

    uint32 PurchaseResult = 0;
    uint32 ClientToken = 0;
    uint64 PurchaseID = 0;
};

class PurchaseUpdate final : public ServerPacket
{
public:
    PurchaseUpdate() : ServerPacket(SMSG_BATTLE_PAY_PURCHASE_UPDATE) { }
    WorldPacket const* Write() override;

    std::vector<Purchase> Purchases;
};

class ConfirmPurchase final : public ServerPacket
{
public:
    ConfirmPurchase() : ServerPacket(SMSG_BATTLE_PAY_CONFIRM_PURCHASE, 12) { }
    WorldPacket const* Write() override;

    uint64 PurchaseID = 0;
    uint32 ServerToken = 0;
};

ByteBuffer& operator<<(ByteBuffer& data, DisplayCard const& displayCard);
ByteBuffer& operator<<(ByteBuffer& data, DisplayInfo const& displayInfo);
ByteBuffer& operator<<(ByteBuffer& data, DeliverableChoice const& choice);
ByteBuffer& operator<<(ByteBuffer& data, Deliverable const& deliverable);
ByteBuffer& operator<<(ByteBuffer& data, Product const& product);
ByteBuffer& operator<<(ByteBuffer& data, Group const& group);
ByteBuffer& operator<<(ByteBuffer& data, ShopEntry const& shop);
ByteBuffer& operator<<(ByteBuffer& data, Purchase const& purchase);
}

#endif // TRINITYCORE_RETAIL_BATTLE_PAY_PACKETS_H
