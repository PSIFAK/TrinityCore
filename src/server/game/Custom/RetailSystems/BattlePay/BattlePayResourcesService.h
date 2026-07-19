/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_RETAIL_BATTLE_PAY_RESOURCES_SERVICE_H
#define TRINITYCORE_RETAIL_BATTLE_PAY_RESOURCES_SERVICE_H

#include "Client/api/client/v2/friends_service.pb.h"
#include "Client/resource_service.pb.h"
#include "WorldserverService.h"

namespace Battlenet::Services
{
class RetailBattlePayResourcesService final : public WorldserverService<resources::v1::ResourcesService>
{
    using BaseService = WorldserverService<resources::v1::ResourcesService>;

public:
    explicit RetailBattlePayResourcesService(WorldSession* session);

    uint32 HandleGetContentHandle(resources::v1::ContentHandleRequest const* request, ContentHandle* response,
        std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& continuation) override;
    uint32 HandleGetTitleIcons(resources::v1::GetTitleIconsRequest const* request, resources::v1::GetTitleIconsResponse* response,
        std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& continuation) override;

protected:
    // Retail world-service replies echo the client's zero object ID. TrinityCore's
    // generic world wrapper still uses object ID 1, which 12.x rejects for ResourcesService.
    void SendResponse(uint32 serviceHash, uint32 methodId, uint32 token, uint32 status) override;
    void SendResponse(uint32 serviceHash, uint32 methodId, uint32 token,
        google::protobuf::Message const* response) override;
};

class RetailBattlePayFriendsService final : public WorldserverService<friends::v2::client::FriendsService>
{
    using FriendsBaseService = WorldserverService<friends::v2::client::FriendsService>;

public:
    explicit RetailBattlePayFriendsService(WorldSession* session);

    uint32 HandleGetSentInvitations(friends::v2::client::GetSentInvitationsRequest const* request,
        friends::v2::client::GetSentInvitationsResponse* response,
        std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& continuation) override;
    uint32 HandleGetReceivedInvitations(friends::v2::client::GetReceivedInvitationsRequest const* request,
        friends::v2::client::GetReceivedInvitationsResponse* response,
        std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& continuation) override;
    uint32 HandleGetFriends(friends::v2::client::GetFriendsRequest const* request,
        friends::v2::client::GetFriendsResponse* response,
        std::function<void(ServiceBase*, uint32, google::protobuf::Message const*)>& continuation) override;

protected:
    void SendResponse(uint32 serviceHash, uint32 methodId, uint32 token, uint32 status) override;
    void SendResponse(uint32 serviceHash, uint32 methodId, uint32 token,
        google::protobuf::Message const* response) override;
};
}

#endif // TRINITYCORE_RETAIL_BATTLE_PAY_RESOURCES_SERVICE_H
