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

	void ENetHost::SendPackage(Data *data, uint32 receiverID, uint32 channel)
	{
		if(_peers.size() == 0)
			return;

		ENetPacket * packet = enet_packet_create(data->GetBytes(), data->GetLength(), 0);

		Peer &peer = _peers[0];
		uint32 counter = 1;
		while(peer.id != receiverID && counter < _peers.size())
		{
			peer = _peers[counter++];
		}

		if(peer.id == receiverID)
			enet_peer_send(peer.peer, 0, packet);
	}
}
