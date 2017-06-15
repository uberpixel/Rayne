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

	ENetServer::ENetServer(uint32 port, uint16 maxConnections) : _maxConnections(maxConnections)
	{
		_status = Status::Server;
		_ip = nullptr;
		_port = port;

		ENetAddress address;
		address.host = ENET_HOST_ANY;
		address.port = _port;

		_enetHost = enet_host_create(&address /* the address to bind the server host to */,
			_maxConnections,
			2      /* allow up to 2 channels to be used, 0 and 1 */,
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

	void ENetServer::Update(float delta)
	{
		ENetEvent event;
		while(enet_host_service(_enetHost, &event, 0) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					RNDebug("A new client connected from " << event.peer->address.host << ":" << event.peer->address.port);
					/* Store any relevant client information here. */
					event.peer->data = "Client information";
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					RNDebug("Received package: \"" << event.packet->data << "\" with length " << event.packet->dataLength << " from peer \"" << event.peer->data << "\" at channel " << event.channelID);
					/* Clean up the packet now that we're done using it. */
					enet_packet_destroy(event.packet);
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					RNDebug("Client disconnected: " << event.peer->data);
					/* Reset the peer's client information. */
					event.peer->data = NULL;
					break;
			}
		}
	}
}
