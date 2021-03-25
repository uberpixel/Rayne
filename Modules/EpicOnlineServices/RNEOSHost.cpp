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

	EOSHost::EOSHost() : _pingTimer(10.0)
	{
		
	}
		
	EOSHost::~EOSHost()
	{
		
	}

	bool EOSHost::IsPacketInOrder(EOSHost::ProtocolPacketType packetType, uint16 senderID, uint8 packetID, uint8 channel)
	{
		EOSWorld *world = EOSWorld::GetInstance();
		Peer &peer = _peers[senderID];
		
		//Send ack for reliable data. EOS has something like this internally, but does not expose any of it :(
		if(packetType == ProtocolPacketTypeReliableData)
		{
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
			
			ProtocolPacketHeader packetHeader;
			packetHeader.packetType = ProtocolPacketTypeReliableDataAck;
			packetHeader.packetID = packetID;
			
			EOS_P2P_SendPacketOptions sendPacketOptions = {0};
			sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
			sendPacketOptions.Channel = channel;
			sendPacketOptions.LocalUserId = world->GetUserID();
			sendPacketOptions.RemoteUserId = peer.internalID;
			sendPacketOptions.SocketId = &socketID;
			sendPacketOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
			sendPacketOptions.bAllowDelayedDelivery = false;
			sendPacketOptions.Data = &packetHeader;
			sendPacketOptions.DataLengthBytes = sizeof(packetHeader);
			EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
		}
		
		//Handle reliable data ack
		if(packetType == ProtocolPacketTypeReliableDataAck)
		{
			//TODO: need to somehow flag relaible data in transit per channel instead of global for peer!?
			if(peer._hasReliableInTransit && packetID == peer._lastReliableIDForChannel[channel])
			{
				peer._hasReliableInTransit = false;
			}
		}
		
		//This assumes that less than 127 packets are ever lost at once...
		if(packetType == ProtocolPacketTypeReliableData || peer._receivedIDForChannel[channel] < packetID || (peer._receivedIDForChannel[channel] > 127 && packetID < 127))
		{
			peer._receivedIDForChannel[channel] = packetID;
			return true;
		}
		
		return false;
	}

	void EOSHost::SendPing(uint16 receiverID, bool isResponse, uint8 responseID)
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
		
		ProtocolPacketHeader packetHeader;
		if(isResponse)
		{
			packetHeader.packetType = ProtocolPacketTypePingResponse;
			packetHeader.packetID = responseID;
		}
		else
		{
			packetHeader.packetType = ProtocolPacketTypePingRequest;
			packetHeader.packetID = _peers[receiverID]._lastPingID++;
			
			clock_gettime(CLOCK_MONOTONIC, &_peers[receiverID]._sentPingTime);
		}
		
		EOS_P2P_SendPacketOptions sendPacketOptions = {0};
		sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		sendPacketOptions.Channel = 255;
		sendPacketOptions.LocalUserId = world->GetUserID();
		sendPacketOptions.RemoteUserId = _peers[receiverID].internalID;
		sendPacketOptions.SocketId = &socketID;
		sendPacketOptions.Reliability = EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
		sendPacketOptions.bAllowDelayedDelivery = false;
		sendPacketOptions.Data = &packetHeader;
		sendPacketOptions.DataLengthBytes = sizeof(packetHeader);
		EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
		Unlock();
	}

	void EOSHost::SendPacket(Data *data, uint16 receiverID, uint32 channel, bool reliable)
	{
		//TODO: Split up packet if too big (whatever too big is for EOS...)
		
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
		
		_pingTimer += delta;
		if(_pingTimer > 5.0)
		{
			_pingTimer = 0.0f;
			for(auto pair : _peers)
			{
				SendPing(pair.first, false, 0);
			}
		}
		
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
		
		uint32 nextPacketSize = 0;
		uint8 pingChannel = 255;
		EOS_P2P_GetNextReceivedPacketSizeOptions nextPacketSizeOptions = {0};
		nextPacketSizeOptions.ApiVersion = EOS_P2P_GETNEXTRECEIVEDPACKETSIZE_API_LATEST;
		nextPacketSizeOptions.LocalUserId = world->GetUserID();
		nextPacketSizeOptions.RequestedChannel = &pingChannel;
		while(EOS_P2P_GetNextReceivedPacketSize(world->GetP2PHandle(), &nextPacketSizeOptions, &nextPacketSize) == EOS_EResult::EOS_Success)
		{
			if(nextPacketSize < sizeof(ProtocolPacketHeader))
			{
				RNDebug("Packet too small, this is not supposed to ever happen...");
				continue;
			}
			
			EOS_P2P_ReceivePacketOptions receiveOptions = {0};
			receiveOptions.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
			receiveOptions.LocalUserId = world->GetUserID();
			receiveOptions.MaxDataSizeBytes = nextPacketSize;
			receiveOptions.RequestedChannel = &pingChannel;
			
			EOS_ProductUserId senderUserID;
			EOS_P2P_SocketId socketID;
			uint8 channel = 0;
			uint32 bytesWritten = 0;
			
			uint8 *rawData = new uint8[nextPacketSize];
			if(EOS_P2P_ReceivePacket(world->GetP2PHandle(), &receiveOptions, &senderUserID, &socketID, &channel, rawData, &bytesWritten) != EOS_EResult::EOS_Success)
			{
				RNDebug("Failed receiving Data");
				break;
			}
			
			ProtocolPacketHeader packetHeader;
			packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[0]);
			packetHeader.packetID = rawData[1];
			
			uint16 id = GetUserIDForInternalID(senderUserID);
			if(packetHeader.packetType == ProtocolPacketTypePingRequest)
			{
				SendPing(id, true, packetHeader.packetID);
			}
			else if(packetHeader.packetType == ProtocolPacketTypePingResponse)
			{
				if(_peers[id]._lastPingID-1 == packetHeader.packetID)
				{
					struct timespec receivedPingTime;
					clock_gettime(CLOCK_MONOTONIC, &receivedPingTime);
					double timeElapsed = (receivedPingTime.tv_sec - _peers[id]._sentPingTime.tv_sec) * 1000.0 + ((double)(receivedPingTime.tv_nsec - _peers[id]._sentPingTime.tv_nsec))/1000000.0;
					
					RNDebug("Ping time for " << id << ": " << timeElapsed);
					
					_peers[id].smoothedPing = _peers[id].smoothedPing * 0.5 + timeElapsed * 0.5;
				}
				else
				{
					RNDebug("Missed a ping!");
				}
			}
			delete[] rawData;
		}
		
		while(_scheduledPackets.size() > 0)
		{
			if(_peers.size() > 0 && _peers.find(_scheduledPackets.front().receiverID) != _peers.end())
			{
				uint8 headerData[2];
				headerData[1] = _peers[_scheduledPackets.front().receiverID]._packetIDForChannel[_scheduledPackets.front().channel]++;
				
				ProtocolPacketType packetType = ProtocolPacketTypeData;
				if(_scheduledPackets.front().isReliable)
				{
					packetType = ProtocolPacketTypeReliableData;
					_peers[_scheduledPackets.front().receiverID]._hasReliableInTransit = true;
					_peers[_scheduledPackets.front().receiverID]._lastReliableIDForChannel[_scheduledPackets.front().channel] = headerData[1];
				}
				headerData[0] = static_cast<ProtocolPacketType>(packetType);

				Data *data = Data::WithBytes(headerData, 2);
				data->Append(_scheduledPackets.front().data);
				
				EOS_P2P_SendPacketOptions sendPacketOptions = {0};
				sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
				sendPacketOptions.Channel = _scheduledPackets.front().channel;
				sendPacketOptions.LocalUserId = world->GetUserID();
				sendPacketOptions.RemoteUserId = _peers[_scheduledPackets.front().receiverID].internalID;
				sendPacketOptions.SocketId = &socketID;
				sendPacketOptions.Reliability = _scheduledPackets.front().isReliable?EOS_EPacketReliability::EOS_PR_ReliableOrdered:EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
				sendPacketOptions.bAllowDelayedDelivery = false;
				sendPacketOptions.Data = data->GetBytes();
				sendPacketOptions.DataLengthBytes = data->GetLength();
				EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
			}
			
			_scheduledPackets.front().data->Release();
			_scheduledPackets.pop();
		}
		Unlock();
	}

	EOSHost::Peer EOSHost::CreatePeer(uint16 userID, EOS_ProductUserId internalID)
	{
		Peer peer;
		peer.userID = userID;
		peer.internalID = internalID;
		peer.smoothedPing = 50.0;
		peer._lastPingID = 0;
		peer._hasReliableInTransit = false;
		
		for(int i = 0; i < 254; i++)
		{
			peer._packetIDForChannel[i] = 0;
			peer._receivedIDForChannel[i] = 255;
			peer._lastReliableIDForChannel[i] = 0;
		}
		
		return peer;
	}

	uint16 EOSHost::GetUserIDForInternalID(EOS_ProductUserId internalID)
	{
		for(auto pair : _peers)
		{
			if(pair.second.internalID == internalID)
			{
				return pair.first;
			}
		}
	}

	bool EOSHost::HasReliableDataInTransit()
	{
		for(auto iter : _peers)
		{
			if(iter.second._hasReliableInTransit)
			{
				return true;
			}
		}
		
		return false;
	}

	double EOSHost::GetLastRoundtripTime(uint16 peerID)
	{
		return _peers[peerID].smoothedPing * 0.001;
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
