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
		
	}
		
	ENetHost::~ENetHost()
	{
		
	}

	void ENetHost::SendPacket(Data *data, uint16 receiverID, uint32 channel, bool reliable)
	{
		if(_peers.size() == 0)
			return;

		ENetPacket * packet = enet_packet_create(data->GetBytes(), data->GetLength(), reliable? ENET_PACKET_FLAG_RELIABLE : 0);

		if(_peers.find(receiverID) != _peers.end())
		{
			enet_peer_send(_peers[receiverID].peer, channel, packet);
		}
	}

	bool ENetHost::HasReliableDataInTransit()
	{
		for(auto iter : _peers)
		{
			if(iter.second.peer->reliableDataInTransit > 0)
			{
				return true;
			}
		}
		
		return false;
	}

	double ENetHost::GetLastRoundtripTime(uint16 peerID)
	{
		return _peers[peerID].peer->lastRoundTripTime * 0.001;
	}

	void ENetHost::SetTimeout(uint16 peerID, size_t limit, size_t minimum, size_t maximum)
	{
		//Times are in milliseconds!
		enet_peer_timeout(_peers[peerID].peer, limit, minimum, maximum);
		
/*		ENET_PEER_TIMEOUT_LIMIT                = 32,
		ENET_PEER_TIMEOUT_MINIMUM              = 5000,
		ENET_PEER_TIMEOUT_MAXIMUM              = 30000,*/
	}

	void ENetHost::SetPingInterval(uint16 peerID, size_t interval)
	{
		//Times are in milliseconds!
		enet_peer_ping_interval(_peers[peerID].peer, interval);
	}
}
