//
//  RNEOSHost.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSHost.h"
#include "RNEOSWorld.h"

#include "eos_platform_prereqs.h"
#include "eos_sdk.h"
#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"

namespace RN
{
	RNDefineMeta(EOSHost, Object)

	EOSHost::EOSHost()
	{
		
	}
		
	EOSHost::~EOSHost()
	{
		
	}

	void EOSHost::SendPacket(Data *data, uint16 receiverID, uint32 channel, bool reliable)
	{
		Lock();
		if(_peers.size() == 0 || _peers.find(receiverID) == _peers.end())
		{
			Unlock();
			return;
		}
		
		_scheduledPackets.push({receiverID, channel, reliable, data->Retain()});
		Unlock();
	}

	void EOSHost::Update(float delta)
	{
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();
		
		EOS_P2P_SocketId socketID = {0};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		socketID.SocketName[0] = 'F';
		socketID.SocketName[1] = 'u';
		socketID.SocketName[2] = 'c';
		socketID.SocketName[3] = 'k';
		socketID.SocketName[4] = 'Y';
		socketID.SocketName[5] = 'e';
		socketID.SocketName[6] = 'a';
		socketID.SocketName[7] = 'h';
		
		while(_scheduledPackets.size() > 0)
		{
			if(_peers.size() > 0 && _peers.find(_scheduledPackets.front().receiverID) != _peers.end())
			{
				EOS_P2P_SendPacketOptions sendPacketOptions = {0};
				sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
				sendPacketOptions.Channel = _scheduledPackets.front().channel;
				sendPacketOptions.LocalUserId = world->GetUserID();
				sendPacketOptions.RemoteUserId = _peers[_scheduledPackets.front().receiverID].peer;
				sendPacketOptions.SocketId = &socketID;
				sendPacketOptions.Reliability = _scheduledPackets.front().isReliable?EOS_EPacketReliability::EOS_PR_ReliableOrdered:EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
				sendPacketOptions.bAllowDelayedDelivery = false;
				sendPacketOptions.Data = _scheduledPackets.front().data->GetBytes();
				sendPacketOptions.DataLengthBytes = _scheduledPackets.front().data->GetLength();
				EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
			}
			
			_scheduledPackets.front().data->Release();
			_scheduledPackets.pop();
		}
		Unlock();
	}

	bool EOSHost::HasReliableDataInTransit()
	{
/*		for(auto iter : _peers)
		{
			if(iter.second.peer->reliableDataInTransit > 0)
			{
				return true;
			}
		}*/
		
		return false;
	}

	double EOSHost::GetLastRoundtripTime(uint16 peerID)
	{
//		return _peers[peerID].peer->lastRoundTripTime * 0.001;
		return 0.0;
	}

	void EOSHost::SetTimeout(uint16 peerID, size_t limit, size_t minimum, size_t maximum)
	{
		//Times are in milliseconds!
//		EOS_peer_timeout(_peers[peerID].peer, limit, minimum, maximum);
		
/*		EOS_PEER_TIMEOUT_LIMIT                = 32,
		EOS_PEER_TIMEOUT_MINIMUM              = 5000,
		EOS_PEER_TIMEOUT_MAXIMUM              = 30000,*/
	}

	void EOSHost::SetPingInterval(uint16 peerID, size_t interval)
	{
		//Times are in milliseconds!
//		EOS_peer_ping_interval(_peers[peerID].peer, interval);
	}
}
