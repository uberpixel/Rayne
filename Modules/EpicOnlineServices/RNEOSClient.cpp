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
		_status = Status::Disconnected;
	}
		
	EOSClient::~EOSClient()
	{
		
	}

	void EOSClient::Connect(EOS_ProductUserId serverProductID)
	{
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
		
		EOS_P2P_SendPacketOptions connectionOptions = {0};
		connectionOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = serverProductID;
		connectionOptions.Channel = 0;
		connectionOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		connectionOptions.bAllowDelayedDelivery = true;
		connectionOptions.DataLengthBytes = 7;
		connectionOptions.Data = "CONNECT";
		
		EOS_P2P_SendPacket(world->GetP2PHandle(), &connectionOptions);
		
/*		EOS_P2P_AcceptConnectionOptions connectionOptions = {0};
		connectionOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = serverProductID;
		EOS_P2P_AcceptConnection(world->GetP2PHandle(), &connectionOptions);*/

		Peer peer;
		peer.id = 0;
		peer.peer = serverProductID;
		if(peer.peer == NULL)
		{
			RNDebug("Couldn't connect to server!");
			_status = Status::Disconnected;
			return;
		}

		_peers.insert(std::pair<uint16, Peer>(peer.id, peer));
	}

	void EOSClient::Disconnect()
	{
		if(_status == Status::Disconnected || _status == Status::Disconnecting)
			return;
		
		if(_status == Status::Connecting)
		{
			ForceDisconnect();
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
	}

	void EOSClient::ForceDisconnect()
	{
		_status = Status::Disconnected;
		//EOS_peer_reset(_peers[0].peer);
		_peers.clear();

		RNDebug("Disconnected!");

		HandleDidDisconnect(0, 0);
	}

	void EOSClient::Update(float delta)
	{
		if(_status == Status::Disconnected)
			return;
		
		EOSWorld *world = EOSWorld::GetInstance();
		
		uint32 nextPacketSize = 0;
		EOS_P2P_GetNextReceivedPacketSizeOptions nextPacketSizeOptions = {0};
		nextPacketSizeOptions.ApiVersion = EOS_P2P_GETNEXTRECEIVEDPACKETSIZE_API_LATEST;
		nextPacketSizeOptions.LocalUserId = world->GetUserID();
		while(EOS_P2P_GetNextReceivedPacketSize(world->GetP2PHandle(), &nextPacketSizeOptions, &nextPacketSize) == EOS_EResult::EOS_Success)
		{
			EOS_P2P_ReceivePacketOptions receiveOptions = {0};
			receiveOptions.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
			receiveOptions.LocalUserId = world->GetUserID();
			receiveOptions.MaxDataSizeBytes = nextPacketSize;
			
			EOS_ProductUserId senderUserID;
			EOS_P2P_SocketId socketID;
			uint8 channel = 0;
			uint32 bytesWritten = 0;
			
			Data *data = Data::WithBytes(nullptr, nextPacketSize);
			if(EOS_P2P_ReceivePacket(world->GetP2PHandle(), &receiveOptions, &senderUserID, &socketID, &channel, data->GetBytes(), &bytesWritten) != EOS_EResult::EOS_Success)
			{
				RNDebug("Failed receiving Data");
				break;
			}
			
			if(data->GetLength() == 9 && String::WithBytes(data->GetBytes(), data->GetLength(), Encoding::UTF8)->IsEqual(RNCSTR("CONNECTED")))
			{
				RNDebug("Connected!");
				_status = Status::Connected;
				HandleDidConnect(0);
				continue;
			}
			
			ReceivedPacket(data, 0, channel);
		}

/*		EOSEvent event;
		while(EOS_host_service(_EOSHost, &event, 0) > 0)
		{
			switch(event.type)
			{
				case EOS_EVENT_TYPE_RECEIVE:
				{
					Data *data = Data::WithBytes(event.packet->data, event.packet->dataLength);
					EOS_packet_destroy(event.packet);

					ReceivedPacket(data, 0, event.channelID);
					break;
				}

				case EOS_EVENT_TYPE_DISCONNECT:
				{
					ForceDisconnect();
					break;
				}
					
				case EOS_EVENT_TYPE_NONE:
				{
					break;
				}
			}
		}*/
	}
}
