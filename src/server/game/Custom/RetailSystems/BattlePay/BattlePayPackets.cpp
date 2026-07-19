/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlePayPackets.h"
#include "PacketOperators.h"
#include <algorithm>
#include <string_view>

namespace WorldPackets::RetailBattlePay
{
namespace
{
std::string_view Limit(std::string const& value, std::size_t maximum)
{
    std::size_t length = std::min(value.size(), maximum);
    if (length != value.size())
        while (length && (static_cast<unsigned char>(value[length]) & 0xC0) == 0x80)
            --length;
    return std::string_view(value.data(), length);
}
}

void StartPurchase::Read()
{
    _worldPacket >> ClientToken;
    _worldPacket >> ProductID;
    _worldPacket >> TargetCharacter;
    _worldPacket >> Unknown;

    // 12.0.7 appends telemetry/payment strings which are not trusted or used by the realm.
    _worldPacket.rfinish();
}

void ConfirmPurchaseResponse::Read()
{
    _worldPacket >> ServerToken;
    _worldPacket >> ClientCurrentPriceFixedPoint;
    ConfirmPurchase = _worldPacket.ReadBit();
    _worldPacket.rfinish();
}

void AckFailedResponse::Read()
{
    _worldPacket >> ServerToken;
}

void RequestStoreFrontInfoUpdate::Read()
{
    _worldPacket >> StoreFrontID;
    _worldPacket.rfinish(); // Requested currency IDs are not needed for an empty realm-owned storefront.
}

void CatalogShopLicenseGameDataRequest::Read()
{
    uint32 gameDataCount = _worldPacket.read<uint32>();
    if (gameDataCount > 4096)
        OnInvalidArraySize(gameDataCount, 4096);

    GameDataIDs.resize(gameDataCount);
    for (uint32& gameDataID : GameDataIDs)
        _worldPacket >> gameDataID;
}

void CommerceTokenGetMarketPrice::Read()
{
    _worldPacket >> ClientToken;
}

void ConsumableTokenCanVeteranBuy::Read()
{
    _worldPacket >> ClientToken;
}

ByteBuffer& operator<<(ByteBuffer& data, DisplayCard const& displayCard)
{
    std::string_view title = Limit(displayCard.Title, 0x3FF);
    data << SizedString::BitsSize<10>(title);
    data.FlushBits();
    data << uint32(displayCard.CreatureDisplayInfoID);
    data << uint32(displayCard.ModelSceneID);
    data << uint32(displayCard.TransmogSetID);
    data << SizedString::Data(title);
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, DisplayInfo const& displayInfo)
{
    std::string_view name1 = Limit(displayInfo.Name1, 0x3FF);
    std::string_view name2 = Limit(displayInfo.Name2, 0x3FF);
    std::string_view name3 = Limit(displayInfo.Name3, 0x1FFF);
    std::string_view tooltip = Limit(displayInfo.Tooltip, 0x1FFF);
    std::string_view instructions = Limit(displayInfo.Instructions, 0x1FFF);
    std::string_view disclaimer = Limit(displayInfo.Disclaimer, 0x1FFF);
    std::string_view nydusLink = Limit(displayInfo.NydusLink, 0xFFF);

    data << Bits<1>(displayInfo.FileDataID.has_value());
    data << Bits<1>(displayInfo.ModelSceneID.has_value());
    data << SizedString::BitsSize<10>(name1);
    data << SizedString::BitsSize<10>(name2);
    data << SizedString::BitsSize<13>(name3);
    data << SizedString::BitsSize<13>(tooltip);
    data << SizedString::BitsSize<13>(instructions);
    data << Bits<1>(displayInfo.Flags.has_value());
    data << Bits<1>(displayInfo.OverrideTextColor.has_value());
    data << Bits<1>(displayInfo.OverrideTexture.has_value());
    data << Bits<1>(displayInfo.OverrideBackground.has_value());
    data << SizedString::BitsSize<13>(disclaimer);
    data << SizedString::BitsSize<12>(nydusLink);
    data.FlushBits();

    data << Size<uint32>(displayInfo.DisplayCards);
    data << uint32(displayInfo.BattlepayCardType);
    data << uint32(displayInfo.BannerType);
    data << uint32(displayInfo.ItemQuantity);

    if (displayInfo.FileDataID)
        data << uint32(*displayInfo.FileDataID);
    if (displayInfo.ModelSceneID)
        data << uint32(*displayInfo.ModelSceneID);

    data << SizedString::Data(name1);
    data << SizedString::Data(name2);
    data << SizedString::Data(name3);
    data << SizedString::Data(tooltip);
    data << SizedString::Data(instructions);

    if (displayInfo.Flags)
        data << uint32(*displayInfo.Flags);
    if (displayInfo.OverrideTextColor)
        data << uint32(*displayInfo.OverrideTextColor);
    if (displayInfo.OverrideTexture)
        data << uint32(*displayInfo.OverrideTexture);
    if (displayInfo.OverrideBackground)
        data << uint32(*displayInfo.OverrideBackground);

    data << SizedString::Data(disclaimer);
    data << SizedString::Data(nydusLink);
    for (DisplayCard const& displayCard : displayInfo.DisplayCards)
        data << displayCard;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, DeliverableChoice const& choice)
{
    data << uint32(choice.ID);
    data << uint32(choice.Type);
    data << uint32(choice.ItemID);
    data << uint32(choice.Quantity);
    data << uint32(choice.MountSpellID);
    data << uint32(choice.BattlePetCreatureID);
    data << Bits<1>(choice.AlreadyOwns);
    data << Bits<1>(choice.PetResult.has_value());
    data << Bits<1>(choice.Display.has_value());
    if (choice.PetResult)
        data << Bits<4>(*choice.PetResult);
    data.FlushBits();
    if (choice.Display)
        data << *choice.Display;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, Deliverable const& deliverable)
{
    std::string_view name = Limit(deliverable.Name, 0xFF);
    std::size_t choiceCount = std::min<std::size_t>(deliverable.Choices.size(), 0x7F);

    data << uint32(deliverable.DeliverableID);
    data << uint32(deliverable.Type);
    data << uint32(deliverable.ItemID);
    data << uint32(deliverable.Quantity);
    data << uint32(deliverable.MountSpellID);
    data << uint32(deliverable.BattlePetCreatureID);
    data << uint32(deliverable.BoostID);
    data << uint32(deliverable.Flags);
    data << uint32(deliverable.TransItemModifiedAppearanceID);
    data << uint32(deliverable.TransmogSetID);
    data << uint32(deliverable.CharTitleID);
    data << uint32(deliverable.SpellItemEnchantmentID);
    data << uint32(deliverable.WarbandSceneID);
    data << SizedString::BitsSize<8>(name);
    data << Bits<1>(deliverable.AlreadyOwns);
    data << Bits<1>(deliverable.PetResult.has_value());
    data << Bits<7>(choiceCount);
    data << Bits<1>(deliverable.Display.has_value());
    if (deliverable.PetResult)
        data << Bits<4>(*deliverable.PetResult);
    data.FlushBits();

    for (std::size_t i = 0; i < choiceCount; ++i)
        data << deliverable.Choices[i];
    data << SizedString::Data(name);
    if (deliverable.Display)
        data << *deliverable.Display;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, Product const& product)
{
    data << uint32(product.ProductID);
    data << uint64(product.NormalPriceFixedPoint);
    data << uint64(product.CurrentPriceFixedPoint);
    data << Size<uint32>(product.DeliverableIDs);
    data << uint32(product.Type);
    data << uint32(product.Flags);
    data << uint32(product.RequiredDeliverableID);
    data << Size<uint32>(product.BundledProductIDs);
    data << uint32(product.Eligibility);
    data << uint64(product.PmtProductID);

    for (uint32 deliverableID : product.DeliverableIDs)
        data << uint32(deliverableID);
    for (uint32 bundledProductID : product.BundledProductIDs)
        data << uint32(bundledProductID);

    data << Bits<1>(product.Display.has_value());
    data.FlushBits();
    if (product.Display)
        data << *product.Display;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, Group const& group)
{
    std::string_view name = Limit(group.Name, 0xFF);
    std::string_view disabledDescription = Limit(group.DisabledDescription, 0xFFFFFE);

    data << uint32(group.GroupID);
    data << uint32(group.IconFileDataID);
    data << uint8(group.DisplayType);
    data << uint32(group.Ordering);
    data << uint32(group.Flags);
    data << uint32(group.ParentGroupID);
    data << SizedString::BitsSize<8>(name);
    data.WriteBits(disabledDescription.size(), 24);
    data.FlushBits();
    data << SizedString::Data(name);
    data << SizedString::Data(disabledDescription);
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, ShopEntry const& shop)
{
    data << uint32(shop.EntryID);
    data << uint32(shop.GroupID);
    data << uint32(shop.ProductID);
    data << uint32(shop.Ordering);
    data << uint32(shop.Flags);
    data << uint8(shop.BannerType);
    data << Bits<1>(shop.Display.has_value());
    data.FlushBits();
    if (shop.Display)
        data << *shop.Display;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, Purchase const& purchase)
{
    std::string_view walletName = Limit(purchase.WalletName, 0xFF);
    data << uint64(purchase.PurchaseID);
    data << uint32(purchase.Status);
    data << uint32(purchase.ResultCode);
    data << uint32(purchase.ProductID);
    data << uint64(purchase.BasePrice);
    data << uint64(purchase.UserPrice);
    data << uint64(purchase.TimeCreated);
    data << SizedString::BitsSize<8>(walletName);
    data.FlushBits();
    data << SizedString::Data(walletName);
    return data;
}

WorldPacket const* ProductListResponse::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << uint32(CurrencyID);
    _worldPacket << Size<uint32>(Products);
    _worldPacket << Size<uint32>(Deliverables);
    _worldPacket << Size<uint32>(Groups);
    _worldPacket << Size<uint32>(Shops);

    for (Product const& product : Products)
        _worldPacket << product;
    for (Deliverable const& deliverable : Deliverables)
        _worldPacket << deliverable;
    for (Group const& group : Groups)
        _worldPacket << group;
    for (ShopEntry const& shop : Shops)
        _worldPacket << shop;
    return &_worldPacket;
}

WorldPacket const* PurchaseListResponse::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket << Size<uint32>(Purchases);
    for (Purchase const& purchase : Purchases)
        _worldPacket << purchase;
    return &_worldPacket;
}

WorldPacket const* LastCatalogFetchResponse::Write()
{
    _worldPacket << uint64(LastCatalogFetch);
    return &_worldPacket;
}

WorldPacket const* CatalogShopObtainLicense::Write()
{
    _worldPacket << uint32(LicenseID);
    return &_worldPacket;
}

WorldPacket const* CatalogShopLicenseData::Write()
{
    // 12.0.7 CatalogShop reply header: final result, data count, missing count, format version.
    _worldPacket << uint32(1);
    _worldPacket << uint32(0);
    _worldPacket << Size<uint32>(MissingGameDataIDs);
    _worldPacket << uint32(2);

    // Each missing entry is byte-aligned in the client protocol.
    for (uint32 gameDataID : MissingGameDataIDs)
    {
        _worldPacket << uint32(gameDataID);
        _worldPacket.WriteBit(false);
        _worldPacket.FlushBits();
    }

    return &_worldPacket;
}

WorldPacket const* DecorRefundListResponse::Write()
{
    _worldPacket << uint32(0); // Refund entry count
    return &_worldPacket;
}

WorldPacket const* AllLicensedDecorQuantitiesResponse::Write()
{
    _worldPacket << uint32(0); // Licensed decor quantity count
    return &_worldPacket;
}

WorldPacket const* AccountStoreFrontUpdate::Write()
{
    _worldPacket << uint8(2); // Complete storefront snapshot
    _worldPacket << uint32(StoreFrontID);
    _worldPacket << uint32(0); // Currency state count
    _worldPacket << uint32(0); // Item state count
    _worldPacket.WriteBit(true); // Currency snapshot complete
    _worldPacket.WriteBit(true); // Item snapshot complete
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* DistributionListResponse::Write()
{
    _worldPacket << uint32(Result);
    _worldPacket.WriteBits(0, 11); // No pending character-service distributions.
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* StartPurchaseResponse::Write()
{
    _worldPacket << uint32(PurchaseResult);
    _worldPacket << uint32(ClientToken);
    _worldPacket << uint64(PurchaseID);
    return &_worldPacket;
}

WorldPacket const* PurchaseUpdate::Write()
{
    _worldPacket << Size<uint32>(Purchases);
    for (Purchase const& purchase : Purchases)
        _worldPacket << purchase;
    return &_worldPacket;
}

WorldPacket const* ConfirmPurchase::Write()
{
    _worldPacket << uint64(PurchaseID);
    _worldPacket << uint32(ServerToken);
    return &_worldPacket;
}
}
