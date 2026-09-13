//
//  RNEOSServer.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSServer.h"
#include "RNEOSWorld.h"

#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"
#include "eos_platform_prereqs.h"
#include "eos_sdk.h"

namespace RN
{
	constexpr uint8 EOSServerClientIDNone = std::numeric_limits<uint8>::max();
	constexpr uint8 EOSServerClientIDReserved = std::numeric_limits<uint8>::max() - 1;

	RNDefineMeta(EOSServer, EOSHost)

	EOSServer::EOSServer(uint8 maxConnections) :
		EOSHost(RNCSTR("FuckYeah")), _maxConnections(maxConnections)
	{
		Lock();
		_status = Server;

		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_AddNotifyPeerConnectionRequestOptions connectListenerOptions;
		connectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
		connectListenerOptions.LocalUserId = world->GetUserID();
		connectListenerOptions.SocketId = &socketID;
		_connectionRequestNotificationID = EOS_P2P_AddNotifyPeerConnectionRequest(world->GetP2PHandle(), &connectListenerOptions, this, OnConnectionRequestCallback);

		EOS_P2P_AddNotifyPeerConnectionClosedOptions disconnectListenerOptions;
		disconnectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
		disconnectListenerOptions.LocalUserId = world->GetUserID();
		disconnectListenerOptions.SocketId = &socketID;
		_connectionClosedNotificationID = EOS_P2P_AddNotifyPeerConnectionClosed(world->GetP2PHandle(), &disconnectListenerOptions, this, OnConnectionClosedCallback);

		Unlock();
	}

	EOSServer::~EOSServer()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(world && world->GetP2PHandle())
		{
			if(_connectionClosedNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
			if(_connectionRequestNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionRequest(world->GetP2PHandle(), _connectionRequestNotificationID);
		}
	}

	uint8 EOSServer::GetUserID()
	{
		for(uint8 freeID = 1; freeID < std::min(_maxConnections, EOSServerClientIDReserved); freeID++)
		{
			if(!_activeUserIDs.count(freeID))
			{
				_activeUserIDs.insert(freeID);
				return freeID;
			}
		}

		return EOSServerClientIDNone;
	}

	void EOSServer::ReleaseUserID(uint8 userID)
	{
		_activeUserIDs.erase(userID);
	}

	void EOSServer::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader)) return;
		EOSHost::ReceivedPacketInternal(rawData, bytesWritten, senderUserID, channel);
		if(channel == 255) return; //This is a ping, handled by the Host class

		Lock();
		auto sender = _peers.find(senderUserID);
		if(sender == _peers.end())
		{
			Unlock();
			return;
		}
		Peer &peer = sender->second;
		uint8 senderID = static_cast<uint8>(peer.clientID);
		peer._isConnectionActive = true;
		DecodeResult result = DecodePackets(peer, rawData, bytesWritten, channel);
		bool acceptsData = !peer._wantsDisconnect;
		for(const DecodedPacket &packet : result.packets)
		{
			if(packet.type == ProtocolPacketTypeConnectRequest && (packet.packetID != 0 || packet.data->GetLength() != 0))
			{
				RNDebug("Malformed connect request");
			}
		}
		Unlock();

		if(result.multipartProgress) HandleReliableMultipartProgress(senderID, channel);
		if(result.lostReliableData)
		{
			HandleReliablePacketLoss(senderID, channel);
			for(const DecodedPacket &packet : result.packets) packet.data->Release();
			return;
		}
		for(const DecodedPacket &packet : result.packets)
		{
			if(acceptsData && (packet.type == ProtocolPacketTypeData || packet.type == ProtocolPacketTypeReliableData)) ReceivedPacket(packet.data, senderID, channel);
			packet.data->Release();
		}
	}

	void EOSServer::Update(float delta)
	{
		EOSHost::Update(delta); //This sends regular pings and handles sending of scheduled packets

		Lock();
		
		std::vector<uint8> peersToDisconnect;
		for(auto &pair : _peers)
		{
			if(pair.second._wantsDisconnect)
			{
				pair.second._disconnectDelay -= delta;
				if(pair.second._disconnectDelay < 0.0f)
				{
					peersToDisconnect.push_back(static_cast<uint8>(pair.second.clientID));
				}
			}
		}

		for(uint8 clientID : peersToDisconnect)
		{
			DisconnectUser(clientID);
		}

		Unlock();
	}

	size_t EOSServer::GetNumberOfConnectedUsers() const
	{
		return _activeUserIDs.size();
	}

	void EOSServer::DisconnectUserDelayed(uint8 userID, uint16 data, float delay)
	{
		Lock();
		EOS_ProductUserId internalID = _idMap[userID];
		_peers[internalID]._disconnectDelay = delay;
		_peers[internalID]._wantsDisconnect = true;
		Unlock();
	}

	void EOSServer::DisconnectUser(uint8 userID)
	{
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = _idMap[userID];
		options.SocketId = &socketID;

		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSServer::Disconnect()
	{
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionsOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;

		EOS_P2P_CloseConnections(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSServer::DisconnectClient(EOS_ProductUserId productUserId)
	{
		if(productUserId == EOSWorld::GetInstance()->GetUserID())
		{
			Disconnect();
		}
		else if(_peers.find(productUserId) != _peers.end())
		{
			DisconnectUser(static_cast<uint8>(_peers[productUserId].clientID));
		}
		else
		{
			RNWarning("Trying to disconnect unknown peer " << productUserId);
		}
	}

	void EOSServer::OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer *>(Data->ClientData);
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, server->GetSocketID()->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_AcceptConnectionOptions connectionOptions;
		connectionOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = Data->RemoteUserId;
		EOS_P2P_AcceptConnection(world->GetP2PHandle(), &connectionOptions);

		server->Lock();
		auto existingPeer = server->_peers.find(Data->RemoteUserId);
		bool isNewPeer = existingPeer == server->_peers.end();
		if(isNewPeer)
		{
			uint8 clientID = server->GetUserID();
			if(clientID == EOSServerClientIDNone)
			{
				server->Unlock();
				return;
			}
			existingPeer = server->_peers.insert(std::pair(Data->RemoteUserId, server->CreatePeer(clientID, Data->RemoteUserId))).first;
			server->_idMap[clientID] = Data->RemoteUserId;
		}
		existingPeer->second._isConnectionActive = true;
		uint8 clientID = static_cast<uint8>(existingPeer->second.clientID);
		server->Unlock();

		ProtocolPacketHeader packetHeader {ProtocolPacketTypeConnectResponse, 0, 0};
		server->SendRawPacket(Data->RemoteUserId, 0, &packetHeader, sizeof(packetHeader), true);

		if(isNewPeer)
		{
			RNDebug("A new client connected");
			server->HandleDidConnect(clientID);
		}
	}

	void EOSServer::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer *>(Data->ClientData);
		server->Lock();
		auto peer = server->_peers.find(Data->RemoteUserId);
		if(peer == server->_peers.end())
		{
			server->Unlock();
			return;
		}
		uint8 id = static_cast<uint8>(peer->second.clientID);
		server->ClearPeerData(peer->second);
		server->_idMap.erase(id);
		server->_peers.erase(peer);
		server->ReleaseUserID(id);
		server->Unlock();

		RNDebug("Client disconnected: " << id);
		server->HandleDidDisconnect(id, static_cast<uint16>(Data->Reason));
	}
} // namespace RN
