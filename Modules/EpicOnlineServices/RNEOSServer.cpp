//
//  RNEOSServer.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSServer.h"
#include "RNEOSWorld.h"

#include "eos_platform_prereqs.h"
#include "eos_sdk.h"
#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"

namespace RN
{
	RNDefineMeta(EOSServer, EOSHost)

	EOSServer::EOSServer(uint16 maxConnections) : _maxConnections(maxConnections)
	{
		Lock();
		_status = Status::Server;
		
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
		
		EOS_P2P_AddNotifyPeerConnectionRequestOptions connectListenerOptions = {0};
		connectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
		connectListenerOptions.LocalUserId = world->GetUserID();
		connectListenerOptions.SocketId = &socketID;
		EOS_P2P_AddNotifyPeerConnectionRequest(world->GetP2PHandle(), &connectListenerOptions, this, OnConnectionRequestCallback);
		
		EOS_P2P_AddNotifyPeerConnectionClosedOptions disconnectListenerOptions = {0};
		disconnectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
		disconnectListenerOptions.LocalUserId = world->GetUserID();
		disconnectListenerOptions.SocketId = &socketID;
		EOS_P2P_AddNotifyPeerConnectionClosed(world->GetP2PHandle(), &disconnectListenerOptions, this, OnConnectionClosedCallback);
		
		Unlock();
	}
		
	EOSServer::~EOSServer()
	{
		
	}

	uint16 EOSServer::GetUserID()
	{
		for(uint16 freeID = 1; freeID < _maxConnections; freeID++)
		{
			if(!_activeUserIDs.count(freeID))
			{
				_activeUserIDs.insert(freeID);
				return freeID;
			}
		}
		
		return -1;
	}

	void EOSServer::ReleaseUserID(uint16 userID)
	{
		_activeUserIDs.erase(userID);
	}

	void EOSServer::Update(float delta)
	{
		EOSHost::Update(delta);
		
		Lock();
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
			
			if(data->GetLength() == 7 && String::WithBytes(data->GetBytes(), data->GetLength(), Encoding::UTF8)->IsEqual(RNCSTR("CONNECT")))
			{
				continue;
			}
			
			//TODO: Make getting peer index from senderID nicer
			uint16 id = 0;
			for(int i = 0; i < _peers.size(); i++)
			{
				if(_peers[i].peer == senderUserID)
				{
					id = i;
				}
			}
			Unlock();
			ReceivedPacket(data, id, channel);
			Lock();
		}
		
		Unlock();
	}

	size_t EOSServer::GetNumberOfConnectedUsers() const
	{
		return _activeUserIDs.size();
	}

	void EOSServer::DisconnectUser(uint16 userID, uint16 data)
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
		
		EOS_P2P_CloseConnectionOptions options = {0};
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = _peers[userID].peer;
		options.SocketId = &socketID;
		
		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSServer::OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer*>(Data->ClientData);
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
		
		EOS_P2P_AcceptConnectionOptions connectionOptions = {0};
		connectionOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = Data->RemoteUserId;
		EOS_P2P_AcceptConnection(world->GetP2PHandle(), &connectionOptions);
		
		RNDebug("A new client connected");
		Peer peer;
		peer.id = server->GetUserID();
		peer.peer = Data->RemoteUserId;
		//EOS_peer_timeout(peer.peer, 0, 0, 0);
		server->_peers.insert(std::pair<uint16, Peer>(peer.id, peer));
		
		
		EOS_P2P_SendPacketOptions connectConfirmOptions = {0};
		connectConfirmOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		connectConfirmOptions.SocketId = &socketID;
		connectConfirmOptions.LocalUserId = world->GetUserID();
		connectConfirmOptions.RemoteUserId = Data->RemoteUserId;
		connectConfirmOptions.Channel = 0;
		connectConfirmOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		connectConfirmOptions.bAllowDelayedDelivery = true;
		connectConfirmOptions.DataLengthBytes = 9;
		connectConfirmOptions.Data = "CONNECTED";
		
		EOS_P2P_SendPacket(world->GetP2PHandle(), &connectConfirmOptions);

		server->HandleDidConnect(peer.id);
	}

	void EOSServer::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer*>(Data->ClientData);
		
		//TODO: Make getting peer index from senderID nicer
		uint16 id = 0;
		for(int i = 0; i < server->_peers.size(); i++)
		{
			if(server->_peers[i].peer == Data->RemoteUserId)
			{
				id = i;
			}
		}
		
		RNDebug("Client disconnected: " << id);
		server->_peers.erase(id);
		server->ReleaseUserID(id);
		server->HandleDidDisconnect(id, 0);
	}
}
