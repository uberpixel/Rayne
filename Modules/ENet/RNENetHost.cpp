//
//  RNENetHost.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetHost.h"
#include "RNENetWorld.h"
#include "enet/enet.h"

namespace RN
{
	RNDefineMeta(ENetHost, Object)

	ENetHost::ENetHost() : _ip(nullptr), _port(0)
	{
		RN_ASSERT(ENetWorld::GetInstance(), "You need an ENetWorld before creating a host!");

		ENetWorld::GetInstance()->AddHost(this);
	}
		
	ENetHost::~ENetHost()
	{
		ENetWorld::GetInstance()->RemoveHost(this);
	}

	void ENetHost::SendPackage(Data *data, uint32 receiverID)
	{
		/* Create a reliable packet of size 7 containing "packet\0" */
//		ENetPacket * packet = enet_packet_create("packet", strlen("packet") + 1, ENET_PACKET_FLAG_RELIABLE);
		/* Extend the packet so and append the string "foo", so it now */
		/* contains "packetfoo\0"                                      */
//		enet_packet_resize(packet, strlen("packetfoo") + 1);
//		strcpy(&packet->data[strlen("packet")], "foo");
		/* Send the packet to the peer over channel id 0. */
		/* One could also broadcast the packet by         */
		/* enet_host_broadcast (host, 0, packet);         */
//		enet_peer_send(peer, 0, packet);
	}
}
