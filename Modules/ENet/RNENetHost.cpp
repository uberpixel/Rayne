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

	void ENetHost::SendPackage(Data *data, uint16 receiverID, uint32 channel, bool reliable)
	{
		if(_peers.size() == 0)
			return;

		ENetPacket * packet = enet_packet_create(data->GetBytes(), data->GetLength(), reliable? ENET_PACKET_FLAG_RELIABLE : 0);

		if(_peers.find(receiverID) != _peers.end())
		{
			enet_peer_send(_peers[receiverID].peer, 0, packet);
		}
	}
}
