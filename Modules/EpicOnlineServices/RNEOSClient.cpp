//
//  RNEOSHost.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSClient.h"
#include "RNEOSWorld.h"

#include "eos_platform_prereqs.h"
#include "eos_sdk.h"
#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"

namespace RN
{
	RNDefineMeta(EOSClient, EOSHost)

	EOSClient::EOSClient()
	{
		Lock();
		_status = Status::Disconnected;
		
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
		
		EOS_P2P_AddNotifyPeerConnectionClosedOptions disconnectListenerOptions = {0};
		disconnectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
		disconnectListenerOptions.LocalUserId = world->GetUserID();
		disconnectListenerOptions.SocketId = &socketID;
		_connectionClosedNotificationID = EOS_P2P_AddNotifyPeerConnectionClosed(world->GetP2PHandle(), &disconnectListenerOptions, this, OnConnectionClosedCallback);
		
		Unlock();
	}
		
	EOSClient::~EOSClient()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
	}

	void EOSClient::Connect(EOS_ProductUserId serverProductID)
	{
		Lock();
		RN_ASSERT(_status == Status::Disconnected, "Already connected to a server.");

		_status = Status::Connecting;
		
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
		packetHeader.packetType = ProtocolPacketTypeConnectRequest;
		packetHeader.packetID = 0;
		
		EOS_P2P_SendPacketOptions connectionOptions = {0};
		connectionOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = serverProductID;
		connectionOptions.Channel = 0;
		connectionOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		connectionOptions.bAllowDelayedDelivery = true;
		connectionOptions.DataLengthBytes = sizeof(packetHeader);
		connectionOptions.Data = &packetHeader;
		
		EOS_P2P_SendPacket(world->GetP2PHandle(), &connectionOptions);

		const Peer &peer = CreatePeer(0, serverProductID);
		if(peer.internalID == NULL)
		{
			RNDebug("Couldn't connect to server!");
			_status = Status::Disconnected;
			Unlock();
			return;
		}

		_peers.insert(std::pair<uint16, Peer>(peer.userID, peer));
		
		Unlock();
	}

	void EOSClient::Disconnect()
	{
		Lock();
		if(_status == Status::Disconnected || _status == Status::Disconnecting)
		{
			Unlock();
			return;
		}
		
		if(_status == Status::Connecting)
		{
			Unlock();
			ForceDisconnect(0);
			return;
		}

		_status = Status::Disconnecting;
		
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
		
		EOS_P2P_CloseConnectionsOptions options = {0};
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;
		
		EOS_P2P_CloseConnections(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSClient::ForceDisconnect(RN::uint16 reason)
	{
		Lock();
		_status = Status::Disconnected;
		_peers.clear();
		Unlock();

		RNDebug("Disconnected!");
		HandleDidDisconnect(0, reason);
	}

	void EOSClient::Update(float delta)
	{
		EOSHost::Update(delta); //Needs to go first as it picks out some packets!
		
		Lock();
		if(_status == Status::Disconnected)
		{
			Unlock();
			return;
		}
		
		EOSWorld *world = EOSWorld::GetInstance();
		
		uint32 nextPacketSize = 0;
		EOS_P2P_GetNextReceivedPacketSizeOptions nextPacketSizeOptions = {0};
		nextPacketSizeOptions.ApiVersion = EOS_P2P_GETNEXTRECEIVEDPACKETSIZE_API_LATEST;
		nextPacketSizeOptions.LocalUserId = world->GetUserID();
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
			
			if(packetHeader.packetType == ProtocolPacketTypeConnectResponse)
			{
				if(packetHeader.packetID == 0)
				{
					RNDebug("Connected!");
					_status = Status::Connected;
					Unlock();
					HandleDidConnect(0);
					Lock();
				}
				else
				{
					RNDebug("Malformed connect response");
				}
				delete[] rawData;
				continue;
			}
			
			if(!IsPacketInOrder(packetHeader.packetType, 0, packetHeader.packetID, channel))
			{
				delete[] rawData;
				continue;
			}
			
			//Get data object from the packet without protocol header
			Data *data = Data::WithBytes(&rawData[2], nextPacketSize-2);
			delete[] rawData;
			
			Unlock();
			ReceivedPacket(data, 0, channel);
			Lock();
		}
		
		Unlock();
	}

	void EOSClient::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSClient *client = static_cast<EOSClient*>(Data->ClientData);
		
		RNDebug("Disconnected from Server");
		client->ForceDisconnect(static_cast<uint16>(Data->Reason));
	}
}
