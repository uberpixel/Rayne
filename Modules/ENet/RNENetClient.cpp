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

	ENetClient::ENetClient() : _connectionTimeOut(5.0f), _enetPeer(nullptr)
	{
		_status = Status::Disconnected;

		_enetHost = enet_host_create(nullptr /* the address to bind the server host to */,
			1,
			2      /* allow up to 2 channels to be used, 0 and 1 */,
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

		/* Initiate the connection, allocating the two channels 0 and 1. */
		_enetPeer = enet_host_connect(_enetHost, &address, 2, 0);
		if(_enetPeer == NULL)
		{
			RNDebug("Couldn't connect to server!");
			return;
		}
	}

	void ENetClient::Disconnect()
	{
		if(_status == Status::Disconnected || _status == Status::Disconnecting)
			return;

		_status = Status::Disconnecting;
		enet_peer_disconnect(_enetPeer, 0);
	}

	void ENetClient::HandleDisconnect()
	{
		enet_peer_reset(_enetPeer);
		_enetPeer = nullptr;
	}

	void ENetClient::Update(float delta)
	{
		_connectionTimeOut -= delta;

		if(_enetPeer && _connectionTimeOut <= 0.0f)
		{
			HandleDisconnect();
		}

		ENetEvent event;
		while(enet_host_service(_enetHost, &event, 0) > 0)
		{
			_connectionTimeOut = 5.0f;

			switch(event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					_status = Status::Connected;
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					RNDebug("Received package: " << event.packet->data);
					/* Clean up the packet now that we're done using it. */
					enet_packet_destroy(event.packet);
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					HandleDisconnect();

					/* Reset the peer's client information. */
					event.peer->data = NULL;
					break;
			}
		}
	}
}
