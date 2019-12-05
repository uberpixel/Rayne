//
//  RNENetServer.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetServer.h"
#include "RNENetWorld.h"
#include "enet/enet.h"

namespace RN
{
	RNDefineMeta(ENetServer, ENetHost)

	ENetServer::ENetServer(uint32 port, uint16 maxConnections, uint32 channelCount) : _maxConnections(maxConnections)
	{
		_status = Status::Server;
		_ip = nullptr;
		_port = port;
		_channelCount = channelCount;

		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = _port;

		_enetHost = enet_host_create(&address,
			_maxConnections,
			_channelCount,
			0      /* assume any amount of incoming bandwidth */,
			0      /* assume any amount of outgoing bandwidth */);

		if(!_enetHost)
		{
			RNDebug("Error while trying to create a server!");
			return;
		}
	}
		
	ENetServer::~ENetServer()
	{
		enet_host_destroy(_enetHost);
	}

	uint16 ENetServer::GetUserID()
	{
		for(uint16 freeID = 1; freeID < _maxConnections; freeID++)
		{
			if(!_activeUserIDs.count(freeID))
			{
				_activeUserIDs.insert(freeID);
				return freeID;
			}
		}
		
		return -1;
	}

	void ENetServer::ReleaseUserID(uint16 userID)
	{
		_activeUserIDs.erase(userID);
	}

	void ENetServer::Update(float delta)
	{
		ENetEvent event;
		while(enet_host_service(_enetHost, &event, 0) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
				{
					//enet_address_get_host_ip()
					RNDebug("A new client connected from " << event.peer->address.host << ":" << event.peer->address.port);
					Peer peer;
					peer.id = GetUserID();
					peer.peer = event.peer;
					enet_peer_timeout(peer.peer, 0, 0, 5000);
					_peers.insert(std::pair<uint16, Peer>(peer.id, peer));
					event.peer->data = malloc(sizeof(uint16));
					*static_cast<uint16*>(event.peer->data) = peer.id;

					HandleDidConnect(peer.id);
					break;
				}

				case ENET_EVENT_TYPE_RECEIVE:
				{
					Data *data = Data::WithBytes(event.packet->data, event.packet->dataLength);
					enet_packet_destroy(event.packet);

					uint16 id = *static_cast<uint16*>(event.peer->data);
					ReceivedPacket(data, id, event.channelID);
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					RNDebug("Client disconnected: " << event.peer->data);
					uint16 id = *static_cast<uint16*>(event.peer->data);
					_peers.erase(id);
					ReleaseUserID(id);
					free(event.peer->data);
					event.peer->data = nullptr;

					HandleDidDisconnect(id);
					break;
				}
			}
		}
	}

	size_t ENetServer::GetNumberOfConnectedUsers() const
	{
		return _activeUserIDs.size();
	}
}
