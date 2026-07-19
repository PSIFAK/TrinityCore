/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BattlePayResourcesService.h"
#include "BattlePayService.h"
#include "Base64.h"
#include "BattlenetPackets.h"
#include "BattlenetRpcErrorCodes.h"
#include "Log.h"
#include "WorldSession.h"
#include <algorithm>
#include <array>

namespace Battlenet::Services
{
namespace
{
constexpr uint32 ContentVersion = 1920291413;
constexpr uint32 DepotRegion = 17749;
constexpr std::string_view DepotUrl = "https://prod.depot.battle.net/${hash}.${usage}";

struct ContentHandleData
{
    uint32 Program;
    uint32 Stream;
    uint32 Version;
    uint32 Usage;
    std::string_view Hash;
};

// Captured from the 12.0.7.68453 retail resource service used by the catalog.
// The content version stayed stable across builds while this depot hash changed,
// so it must be kept in sync with the packet capture rather than inferred from version.
constexpr std::array ContentHandles =
{
    ContentHandleData{ 16974,   1634756212, ContentVersion, 1885762681, "NpylGlVtJhWP74LoAm4bjD5ZftgFWN4itZdCXdKamMY=" },
    ContentHandleData{ 4288624, 1919971955, ContentVersion, 1634758771, "bp2PBmOWop9bAV3g5uthqQikN4l+8vvhxeiLGIJSxnU=" },
    ContentHandleData{ 5730135, 1380995667, ContentVersion, 2003793779, "MoOZh0SViWCJJz9VzSNKCQbILEPWiRvoaJ+LrULEVMg=" },
    ContentHandleData{ 5730135, 1098016097, ContentVersion, 2003793779, "2fhgEmcPWV59aH/RryxO279XftmPUFi3SqRQ+ml16Jc=" },
    ContentHandleData{ 5730135, 1382378605, ContentVersion, 2003793779, "TY8saIxYy7Ri4sRVQhPyciCqVM5w0NuU1fnrtQSAb94=" }
};

struct TitleIconData
{
    uint32 TitleID;
    uint32 Version;
    std::string_view Hash;
};

constexpr std::array TitleIcons =
{
    TitleIconData{ 4288624, 115, "CdvyvU+Db5USPgI5JleCilbrINYljOeQcO6/tNvdUQs=" },
    TitleIconData{ 4288624, 109, "JoRl+YUjP1BMgQH5RuznnI0+UYkTqr3An/lPWspCm3E=" },
    TitleIconData{ 5730135, 115, "pyED5sgPhx1hTXkjfGeW/HWALT/4yyvGh+0yhVcLkK8=" },
    TitleIconData{ 5730135, 109, "xY8tmUpu5TeQHFtuaPV1Jz1O+C9zGEB/fnuNGWTbv18=" }
};

bool FillContentHandle(ContentHandle* response, uint32 usage, std::string_view encodedHash)
{
    Optional<std::vector<uint8>> hash = Trinity::Encoding::Base64::Decode(encodedHash);
    if (!hash || hash->size() != 32)
        return false;

    response->set_region(DepotRegion);
    response->set_usage(usage);
    response->set_hash(hash->data(), hash->size());
    response->set_proto_url(DepotUrl.data(), DepotUrl.size());
    return true;
}

void SendRetailRpcResponse(WorldSession* session, uint32 serviceHash, uint32 methodId, uint32 token,
    BattlenetRpcErrorCode status, google::protobuf::Message const* payload)
{
    WorldPackets::Battlenet::Response response;
    response.BnetStatus = status;
    response.Method.Type = MAKE_PAIR64(methodId, serviceHash);
    response.Method.ObjectId = 0;
    response.Method.Token = token;

    if (payload)
    {
        int32 size = payload->ByteSize();
        if (size > 0)
        {
            response.Data.resize(size);
            payload->SerializePartialToArray(response.Data.data(), size);
        }
    }

    RetailSystems::BattlePay::SendCapturedPacket(session, response.Write(),
        RetailSystems::BattlePay::GetCapturedRpcDelay(serviceHash, methodId));
}
}

RetailBattlePayResourcesService::RetailBattlePayResourcesService(WorldSession* session) : BaseService(session) { }

void RetailBattlePayResourcesService::SendResponse(uint32 serviceHash, uint32 methodId, uint32 token, uint32 status)
{
    SendRetailRpcResponse(_session, serviceHash, methodId, token, BattlenetRpcErrorCode(status), nullptr);
}

void RetailBattlePayResourcesService::SendResponse(uint32 serviceHash, uint32 methodId, uint32 token,
    google::protobuf::Message const* response)
{
    SendRetailRpcResponse(_session, serviceHash, methodId, token, ERROR_OK, response);
}

uint32 RetailBattlePayResourcesService::HandleGetContentHandle(resources::v1::ContentHandleRequest const* request,
    ContentHandle* response, std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& /*continuation*/)
{
    auto itr = std::ranges::find_if(ContentHandles, [request](ContentHandleData const& handle)
    {
        return handle.Program == request->program() && handle.Stream == request->stream() && handle.Version == request->version();
    });

    if (itr == ContentHandles.end())
    {
        TC_LOG_DEBUG("server.battlepay", "RetailSystems: no resource content handle for program {}, stream {}, version {}",
            request->program(), request->stream(), request->version());
        return ERROR_RPC_NOT_IMPLEMENTED;
    }

    return FillContentHandle(response, itr->Usage, itr->Hash) ? ERROR_OK : ERROR_INTERNAL;
}

uint32 RetailBattlePayResourcesService::HandleGetTitleIcons(resources::v1::GetTitleIconsRequest const* request,
    resources::v1::GetTitleIconsResponse* response,
    std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& /*continuation*/)
{
    for (uint32 titleID : request->title_ids())
    {
        auto itr = std::ranges::find_if(TitleIcons, [titleID, request](TitleIconData const& icon)
        {
            return icon.TitleID == titleID && icon.Version == request->version();
        });

        if (itr == TitleIcons.end())
            continue;

        TitleIconContentHandle* titleIcon = response->add_title_icon_content_handles();
        titleIcon->set_title_id(titleID);
        if (!FillContentHandle(titleIcon->mutable_content_handle(), 7368295, itr->Hash))
            return ERROR_INTERNAL;
    }

    return response->title_icon_content_handles_size() ? ERROR_OK : ERROR_RPC_NOT_IMPLEMENTED;
}

RetailBattlePayFriendsService::RetailBattlePayFriendsService(WorldSession* session) : FriendsBaseService(session) { }

uint32 RetailBattlePayFriendsService::HandleGetSentInvitations(
    friends::v2::client::GetSentInvitationsRequest const* /*request*/,
    friends::v2::client::GetSentInvitationsResponse* response,
    std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& /*continuation*/)
{
    response->set_continuation(0);
    return ERROR_OK;
}

uint32 RetailBattlePayFriendsService::HandleGetReceivedInvitations(
    friends::v2::client::GetReceivedInvitationsRequest const* /*request*/,
    friends::v2::client::GetReceivedInvitationsResponse* response,
    std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& /*continuation*/)
{
    response->set_continuation(0);
    return ERROR_OK;
}

uint32 RetailBattlePayFriendsService::HandleGetFriends(friends::v2::client::GetFriendsRequest const* /*request*/,
    friends::v2::client::GetFriendsResponse* response,
    std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& /*continuation*/)
{
    response->set_continuation(0);
    return ERROR_OK;
}

void RetailBattlePayFriendsService::SendResponse(uint32 serviceHash, uint32 methodId, uint32 token, uint32 status)
{
    SendRetailRpcResponse(_session, serviceHash, methodId, token, BattlenetRpcErrorCode(status), nullptr);
}

void RetailBattlePayFriendsService::SendResponse(uint32 serviceHash, uint32 methodId, uint32 token,
    google::protobuf::Message const* response)
{
    SendRetailRpcResponse(_session, serviceHash, methodId, token, ERROR_OK, response);
}
}
