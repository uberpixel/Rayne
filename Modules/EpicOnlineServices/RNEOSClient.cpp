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
		
		EOSWorld *world = EOSWorld::GetInstance();
		
		EOS_P2P_SocketId socketID = {0};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		socketID.SocketName[0] = 'F';
		socketID.SocketName[1] = 'u';
		socketID.SocketName[2] = 'c';
		socketID.SocketName[3] = 'k';
		socketID.SocketName[4] = ' ';
		socketID.SocketName[5] = 'Y';
		socketID.SocketName[6] = 'e';
		socketID.SocketName[7] = 'a';
		socketID.SocketName[8] = 'h';
		
		EOS_P2P_AddNotifyPeerConnectionRequestOptions options = {0};
		options.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;
		
		EOS_P2P_AddNotifyPeerConnectionRequest(world->GetP2PHandle(), &options, this, OnConnectionRequestCallback);
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
		socketID.SocketName[4] = ' ';
		socketID.SocketName[5] = 'Y';
		socketID.SocketName[6] = 'e';
		socketID.SocketName[7] = 'a';
		socketID.SocketName[8] = 'h';
		
		EOS_P2P_AcceptConnectionOptions connectionOptions = {0};
		connectionOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = serverProductID;
		EOS_P2P_AcceptConnection(world->GetP2PHandle(), &connectionOptions);

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
		socketID.SocketName[4] = ' ';
		socketID.SocketName[5] = 'Y';
		socketID.SocketName[6] = 'e';
		socketID.SocketName[7] = 'a';
		socketID.SocketName[8] = 'h';
		
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

	void EOSClient::OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data)
	{
		EOSClient *client = static_cast<EOSClient*>(Data->ClientData);
		
		RNDebug("Connected!");
		client->_status = Status::Connected;
		client->HandleDidConnect(0);
	}
}
