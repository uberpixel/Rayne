//
//  RNENetHost.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetClient.h"
#include "RNENetWorld.h"
#include "enet/enet.h"

namespace RN
{
	RNDefineMeta(ENetClient, ENetHost)

	ENetClient::ENetClient(uint32 channelCount) : _connectionTimeOut(5.0f)
	{
		_status = Status::Disconnected;
		_channelCount = channelCount;

		_enetHost = enet_host_create(nullptr, 1, _channelCount,
			0      /* assume any amount of incoming bandwidth */,
			0      /* assume any amount of outgoing bandwidth */);

		if(!_enetHost)
		{
			RNDebug("Error while trying to create a host!");
			return;
		}
	}
		
	ENetClient::~ENetClient()
	{
		enet_host_destroy(_enetHost);
	}

	void ENetClient::Connect(String *ip, uint32 port)
	{
		RN_ASSERT(_status == Status::Disconnected, "Already connected to a server.");

		_ip = ip->Retain();
		_port = port;
		_status = Status::Connecting;

		ENetAddress address;
		enet_address_set_host(&address, _ip->GetUTF8String());
		address.port = _port;

		Peer peer;
		peer.id = 0;
		peer.peer = enet_host_connect(_enetHost, &address, _channelCount, 0);
		if(peer.peer == NULL)
		{
			RNDebug("Couldn't connect to server!");
			return;
		}

		_peers.insert(std::pair<uint16, Peer>(peer.id, peer));
	}

	void ENetClient::Disconnect()
	{
		if(_status == Status::Disconnected || _status == Status::Disconnecting)
			return;

		_status = Status::Disconnecting;
		enet_peer_disconnect(_peers[0].peer, 0);
	}

	void ENetClient::ForceDisconnect()
	{
		_status = Status::Disconnected;
		enet_peer_reset(_peers[0].peer);
		_peers.clear();

		RNDebug("Disconnected!");

		HandleDidDisconnect(0);
	}

	void ENetClient::Update(float delta)
	{
		if(_status == Status::Disconnected)
			return;

		if(delta > 0.5f)
			delta = 0.5f;

		_connectionTimeOut -= delta;

		if(_peers.size() > 0 && _connectionTimeOut <= 0.0f)
		{
			ForceDisconnect();
		}

		ENetEvent event;
		while(enet_host_service(_enetHost, &event, 0) > 0)
		{
			_connectionTimeOut = 5.0f;

			switch(event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					RNDebug("Connected!");
					_status = Status::Connected;
					HandleDidConnect(0);
					break;

				case ENET_EVENT_TYPE_RECEIVE:
				{
					Data *data = Data::WithBytes(event.packet->data, event.packet->dataLength);
					enet_packet_destroy(event.packet);

					ReceivedPacket(data, 0, event.channelID);
					break;
				}

				case ENET_EVENT_TYPE_DISCONNECT:
					ForceDisconnect();
					break;
			}
		}
	}
}
