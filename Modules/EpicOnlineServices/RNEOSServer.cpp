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
		if(IsReservedChannel(channel)) return; //Host control traffic

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
		uint64 senderStream = peer._reliableSendStream;
		DecodeResult result = DecodePackets(peer, rawData, bytesWritten, channel);
		Unlock();

		Retain();
		auto isSenderCurrent = [&]()
		{
			Lock();
			auto currentPeer = _peers.find(senderUserID);
			bool isCurrent = _status != Disconnected && _status != Disconnecting && currentPeer != _peers.end() &&
				currentPeer->second.clientID == senderID && currentPeer->second._reliableSendStream == senderStream && !currentPeer->second._wantsDisconnect;
			Unlock();
			return isCurrent;
		};

		if(result.multipartProgress && isSenderCurrent()) HandleReliableMultipartProgress(senderID, channel);
		if(result.lostReliableData && isSenderCurrent()) HandleReliablePacketLoss(senderID, channel);
		for(Data *packet : result.packets)
		{
			if(!result.lostReliableData && isSenderCurrent()) ReceivedPacket(packet, senderID, channel);
			packet->Release();
		}
		Release();
	}

	size_t EOSServer::GetNumberOfConnectedUsers() const
	{
		return _activeUserIDs.size();
	}

	void EOSServer::DisconnectUserDelayed(uint8 userID, uint16 data, float delay)
	{
		Lock();
		auto mappedPeer = _idMap.find(userID);
		if(mappedPeer != _idMap.end())
		{
			auto peer = _peers.find(mappedPeer->second);
			if(peer != _peers.end())
			{
				peer->second._disconnectDelay = delay;
				peer->second._wantsDisconnect = true;
			}
		}
		Unlock();
	}

	void EOSServer::DisconnectUser(uint8 userID)
	{
		Lock();
		auto mappedPeer = _idMap.find(userID);
		if(mappedPeer == _idMap.end())
		{
			Unlock();
			return;
		}
		EOS_ProductUserId productUserID = mappedPeer->second;
		auto peer = _peers.find(productUserID);
		if(peer == _peers.end())
		{
			Unlock();
			return;
		}
		ClearPeerData(peer->second);
		_peers.erase(peer);
		_idMap.erase(mappedPeer);
		ReleaseUserID(userID);
		Retain();
		Unlock();

		ClosePeerTransport(productUserID);
		HandleDidDisconnect(userID, 0);
		Release();
	}

	void EOSServer::Disconnect()
	{
		Retain();
		Lock();
		std::vector<std::pair<EOS_ProductUserId, uint64>> peersToDisconnect;
		for(auto &peer : _peers)
		{
			peer.second._wantsDisconnect = true;
			peersToDisconnect.emplace_back(peer.first, peer.second._reliableSendStream);
		}
		Unlock();
		for(const auto &closingPeer : peersToDisconnect)
		{
			Lock();
			auto peer = _peers.find(closingPeer.first);
			bool isCurrentPeer = peer != _peers.end() && peer->second._reliableSendStream == closingPeer.second;
			Unlock();
			if(isCurrentPeer) DisconnectClient(closingPeer.first);
		}
		Release();
	}

	void EOSServer::DisconnectClient(EOS_ProductUserId productUserId)
	{
		if(productUserId == EOSWorld::GetInstance()->GetUserID())
		{
			Disconnect();
			return;
		}
		Lock();
		auto peer = _peers.find(productUserId);
		uint8 clientID = peer != _peers.end() ? static_cast<uint8>(peer->second.clientID) : EOSServerClientIDNone;
		Unlock();
		if(clientID != EOSServerClientIDNone) DisconnectUser(clientID);
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
		if(EOS_P2P_AcceptConnection(world->GetP2PHandle(), &connectionOptions) != EOS_EResult::EOS_Success) return;

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
			existingPeer->second._didNotifyConnection = true;
		}
		existingPeer->second._isConnectionActive = true;
		uint8 clientID = static_cast<uint8>(existingPeer->second.clientID);
		server->Unlock();

		ProtocolPacketHeader packetHeader {ProtocolPacketTypeConnectResponse, 0, 0};
		server->SendRawPacket(Data->RemoteUserId, 0, &packetHeader, sizeof(packetHeader), true);

		if(isNewPeer)
		{
			RNDebug("A new client connected");
			server->Retain();
			server->HandleDidConnect(clientID);
			server->Release();
		}
	}

	void EOSServer::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer *>(Data->ClientData);
		if(!server || Data->Reason == EOS_EConnectionClosedReason::EOS_CCR_ClosedByLocalUser) return;
		server->Retain();
		server->Lock();
		auto peer = server->_peers.find(Data->RemoteUserId);
		if(peer == server->_peers.end())
		{
			server->Unlock();
			server->Release();
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
		server->Release();
	}
} // namespace RN
