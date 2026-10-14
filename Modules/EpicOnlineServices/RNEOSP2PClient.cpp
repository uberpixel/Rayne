//
//  RNEOSP2PClient.cpp
//  Rayne-EOS
//
//  Copyright 2025 by Guacam. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSP2PClient.h"
#include "RNEOSWorld.h"

#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"
#include "eos_platform_prereqs.h"

constexpr float RN_EOS_CONNECTION_TIMEOUT = 12.0f;
constexpr float RN_EOS_CONNECTION_RETRY_INTERVAL = 3.0f;

namespace RN
{
	RNDefineMeta(EOSP2PClient, EOSHost)

	EOSP2PClient::EOSP2PClient(bool isHost, String *socketID_, EOS_ProductUserId hostProductUserID, bool recoverableMesh) :
		EOSHost(socketID_), _hostClientID(CLIENT_ID_NONE), _hostProductUserID(hostProductUserID), _isHostMigrationPending(false), _recoverableMesh(recoverableMesh), _connectionTimeout(0.0f)
	{
		Lock();
		RN_ASSERT(isHost || _hostProductUserID, "A non-host P2P client requires the lobby owner's product user ID.");
		_limitAggregateSending = _recoverableMesh;
		_status = (isHost || _recoverableMesh) ? Connected : Disconnected;

		EOSWorld *world = EOSWorld::GetInstance();
		if(isHost) _hostProductUserID = world->GetUserID();
		_clientID = GetClientIDForProductUserID(world->GetUserID());
		_hostClientID = GetClientIDForProductUserID(_hostProductUserID);

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

	EOSP2PClient::~EOSP2PClient()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(world && world->GetP2PHandle())
		{
			if(_connectionRequestNotificationID != EOS_INVALID_NOTIFICATIONID)
			{
				EOS_P2P_RemoveNotifyPeerConnectionRequest(world->GetP2PHandle(), _connectionRequestNotificationID);
			}
			if(_connectionClosedNotificationID != EOS_INVALID_NOTIFICATIONID)
			{
				EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
			}
		}
	}

	void EOSP2PClient::Connect(EOS_ProductUserId remoteProductUserID)
	{
		if(!remoteProductUserID || remoteProductUserID == EOSWorld::GetInstance()->GetUserID() || !ShouldAcceptPeer(remoteProductUserID))
		{
			return;
		}
		RNDebug("Connecting to " << remoteProductUserID);
		Lock();
		RN_ASSERT(_status != Disconnecting, "Cannot connect while disconnecting.");
		bool didReconnectLocalHost = false;
		if(_blockedPeers.count(remoteProductUserID) > 0)
		{
			Unlock();
			return;
		}

		if(_status == Disconnected)
		{
			_hostClientID = GetClientIDForProductUserID(_hostProductUserID);
			didReconnectLocalHost = _hostProductUserID == EOSWorld::GetInstance()->GetUserID();
			_status = (_recoverableMesh || didReconnectLocalHost) ? Connected : Connecting;
			_connectionTimeout = 0.0f;
		}
		Peer *peer = BindPeerLocked(remoteProductUserID);
		if(!peer)
		{
			Unlock();
			RNWarning("EOS product user ID collision while connecting to " << remoteProductUserID);
			Disconnect();
			return;
		}
		peer->_reconnectTimer = 0.0f;
		peer->_isReconnectScheduled = true;
		Unlock();

		if(!SendConnectRequest(remoteProductUserID))
		{
			RNWarning("Failed to send connection request to " << remoteProductUserID << "; retrying without disconnecting the mesh");
		}
		if(didReconnectLocalHost) HandleDidConnect(_clientID);
	}

	void EOSP2PClient::Reconnect(EOS_ProductUserId remoteProductUserID)
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world || !remoteProductUserID || remoteProductUserID == world->GetUserID() || !ShouldAcceptPeer(remoteProductUserID)) return;
		Lock();
		if(_status == Disconnecting || _peers.count(remoteProductUserID) > 0)
		{
			Unlock();
			return;
		}
		_blockedPeers.erase(remoteProductUserID);
		Unlock();
		Connect(remoteProductUserID);
	}

	void EOSP2PClient::Disconnect()
	{
		RNDebug("Disconnecting");
		Lock();
		if(_status == Disconnected || _status == Disconnecting)
		{
			Unlock();
			return;
		}
		_status = Disconnecting;
		Retain();
		Unlock();

		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionsOptions options = {};
		options.ApiVersion = EOS_P2P_CLOSECONNECTIONS_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;

		if(EOS_P2P_CloseConnections(world->GetP2PHandle(), &options) == EOS_EResult::EOS_Success)
		{
			RNDebug("Closed all connections");
		}
		else
		{
			RNWarning("Failed closing all connections");
		}

		Lock();
		EOSClientID localClientID = _clientID;
		for(auto &peer : _peers)
		{
			ClearPeerData(peer.second);
		}
		_peers.clear();
		_idMap.clear();
		_blockedPeers.clear();
		_hostClientID = CLIENT_ID_NONE;
		_isHostMigrationPending = false;
		_status = Disconnected;
		Unlock();

		HandleDidDisconnect(localClientID, 0);

		Release();
	}
	
	void EOSP2PClient::DisconnectClient(EOS_ProductUserId productUserId)
	{
		if(productUserId == EOSWorld::GetInstance()->GetUserID())
		{
			Disconnect();
			return;
		}
		bool shouldBlockReconnect = ShouldAcceptPeer(productUserId);
		Lock();
		if(shouldBlockReconnect) _blockedPeers.insert(productUserId);
		else _blockedPeers.erase(productUserId);
		auto peer = _peers.find(productUserId);
		if(peer == _peers.end())
		{
			Unlock();
			RNWarning("Trying to disconnect unknown peer " << productUserId);
			return;
		}
		peer->second._isReconnectScheduled = false;
		peer->second._wantsDisconnect = true;
		Unlock();
		Retain();
		ClosePeerTransport(productUserId);
		FinalizePeerDisconnect(productUserId, 0);
		Release();
	}

	void EOSP2PClient::DisconnectClient(EOSClientID clientID)
	{
		if(clientID == _clientID)
		{
			Disconnect();
			return;
		}
		Lock();
		auto mappedPeer = _idMap.find(clientID);
		EOS_ProductUserId productUserID = mappedPeer != _idMap.end() ? mappedPeer->second : nullptr;
		Unlock();
		if(!productUserID)
		{
			RNWarning("Trying to disconnect unknown client " << clientID);
			return;
		}
		DisconnectClient(productUserID);
	}

	void EOSP2PClient::DisconnectClientDelayed(EOSClientID clientID, float delay)
	{
		RNDebug("Disconnecting client " << clientID << " in " << delay << " seconds");
		Lock();
		auto mappedPeer = _idMap.find(clientID);
		if(mappedPeer != _idMap.end())
		{
			_blockedPeers.insert(mappedPeer->second);
			auto peer = _peers.find(mappedPeer->second);
			if(peer != _peers.end())
			{
				peer->second._disconnectDelay = delay;
				peer->second._wantsDisconnect = true;
				peer->second._isReconnectScheduled = false;
			}
		}
		Unlock();
	}

	void EOSP2PClient::MigrateHost(EOS_ProductUserId hostProductUserId)
	{
		if(!hostProductUserId) return;
		Lock();
		if(_status == Disconnecting)
		{
			Unlock();
			return;
		}

		_hostProductUserID = hostProductUserId;
		if(_recoverableMesh)
		{
			_hostClientID = GetClientIDForProductUserID(hostProductUserId);
			_isHostMigrationPending = false;
			Unlock();
			Retain();
			HandleHostMigration();
			Release();
			return;
		}
		if(_status == Disconnected)
		{
			_hostClientID = CLIENT_ID_NONE;
			_isHostMigrationPending = false;
			Unlock();
			return;
		}
		_hostClientID = GetClientIDForProductUserID(hostProductUserId);
		_isHostMigrationPending = true;

		bool didCompleteHostMigration = TryCompleteHostMigration();
		bool didConnectLocalClient = didCompleteHostMigration && IsHost() && _status == Connecting;
		std::vector<std::pair<EOS_ProductUserId, uint64>> connectedPeers;
		if(didConnectLocalClient)
		{
			_status = Connected;
			_connectionTimeout = 0.0f;
			for(auto &mappedPeer : _peers)
			{
				if(!mappedPeer.second._isConnectionActive || mappedPeer.second._didNotifyConnection) continue;
				mappedPeer.second._didNotifyConnection = true;
				connectedPeers.emplace_back(mappedPeer.first, mappedPeer.second._reliableSendStream);
			}
		}
		bool shouldConnectHost = !didCompleteHostMigration && hostProductUserId != EOSWorld::GetInstance()->GetUserID();
		Unlock();

		if(shouldConnectHost) Connect(hostProductUserId);
		if(didCompleteHostMigration)
		{
			Retain();
			HandleHostMigration();
			if(didConnectLocalClient && _status == Connected) HandleDidConnect(_clientID);
			for(const auto &connectedPeer : connectedPeers)
			{
				Lock();
				auto currentPeer = _peers.find(connectedPeer.first);
				EOSClientID clientID = _status == Connected && currentPeer != _peers.end() &&
					!currentPeer->second._wantsDisconnect && currentPeer->second._reliableSendStream == connectedPeer.second ? currentPeer->second.clientID : CLIENT_ID_NONE;
				Unlock();
				if(clientID != CLIENT_ID_NONE) HandleDidConnect(clientID);
			}
			Release();
		}
	}

	bool EOSP2PClient::TryCompleteHostMigration()
	{
		if(!_isHostMigrationPending) return false;

		if(_hostProductUserID == EOSWorld::GetInstance()->GetUserID())
		{
			_hostClientID = _clientID;
		}
		else
		{
			auto host = _peers.find(_hostProductUserID);
			if(host == _peers.end() || !host->second._didNotifyConnection) return false;
			_hostClientID = host->second.clientID;
		}

		_isHostMigrationPending = false;
		if(_hostProductUserID == EOSWorld::GetInstance()->GetUserID())
		{
			RNDebug("Took over host role.");
		}
		else
		{
			RNDebug("Lobby host role was transferred to client " << _hostClientID);
		}
		return true;
	}

	bool EOSP2PClient::SendConnectRequest(EOS_ProductUserId receiverID, bool response, bool pending)
	{
		ProtocolPacketHeader header {response ? ProtocolPacketTypeConnectResponse : ProtocolPacketTypeConnectRequest, 0, 0};
		if(!_recoverableMesh) return SendRawPacket(receiverID, 0, &header, sizeof(header), true);
		Lock();
		auto peer = _peers.find(receiverID);
		if(peer == _peers.end() || peer->second._wantsDisconnect)
		{
			Unlock();
			return false;
		}
		ConnectionHandshake handshake {pending ? peer->second._pendingLocalStream : peer->second._reliableSendStream,
			pending ? peer->second._pendingRemoteStream : peer->second._remoteStream};
		Unlock();
		header.dataLength = sizeof(handshake);
		uint8 bytes[sizeof(header) + sizeof(handshake)];
		memcpy(bytes, &header, sizeof(header));
		memcpy(bytes + sizeof(header), &handshake, sizeof(handshake));
		return SendRawPacket(receiverID, 0, bytes, sizeof(bytes), true);
	}

	void EOSP2PClient::ReceiveMeshHandshake(EOS_ProductUserId senderUserID, const ProtocolPacketHeader &header, const ConnectionHandshake &handshake)
	{
		if(!handshake.streamID) return;
		Lock();
		if(_status != Connected || _blockedPeers.count(senderUserID))
		{
			Unlock();
			return;
		}
		Peer *peer = BindPeerLocked(senderUserID);
		if(!peer || peer->_wantsDisconnect)
		{
			Unlock();
			return;
		}
		bool isResponse = header.packetType == ProtocolPacketTypeConnectResponse;
		bool didDisconnect = false;
		bool didConnect = false;
		bool shouldRespond = !isResponse;
		bool usePending = false;
		if(peer->_didNotifyConnection && handshake.streamID != peer->_remoteStream)
		{
			if(!isResponse)
			{
				usePending = true;
				// Keep one candidate challenge. An unconfirmed request cannot replace a healthy stream.
				if(peer->_pendingRemoteStream != handshake.streamID)
				{
					peer->_pendingLocalStream = CreatePeer(peer->clientID, senderUserID)._reliableSendStream;
					peer->_pendingRemoteStream = handshake.streamID;
				}
			}
			else if(peer->_pendingLocalStream && handshake.echoedStreamID == peer->_pendingLocalStream && handshake.streamID == peer->_pendingRemoteStream)
			{
				EOSClientID clientID = peer->clientID;
				uint64 localStream = peer->_pendingLocalStream;
				ClearPeerData(*peer);
				*peer = CreatePeer(clientID, senderUserID);
				peer->_reliableSendStream = localStream;
				peer->_requiresBoundStream = true;
				peer->_remoteStream = handshake.streamID;
				didDisconnect = true;
				didConnect = true;
				shouldRespond = true;
			}
			else
			{
				Unlock();
				return;
			}
		}
		else if(!peer->_didNotifyConnection)
		{
			if(isResponse && handshake.echoedStreamID != peer->_reliableSendStream)
			{
				Unlock();
				return;
			}
			peer->_remoteStream = handshake.streamID;
			didConnect = isResponse;
			shouldRespond = true;
		}
		// A response proving both current stream IDs also proves transport recovery.
		// Unconfirmed requests and stale responses cannot resume an interrupted stream.
		if(isResponse && handshake.echoedStreamID == peer->_reliableSendStream && handshake.streamID == peer->_remoteStream)
		{
			peer->_isConnectionInterrupted = false;
			peer->_connectionInterruptedAt = 0.0;
			peer->_isConnectionActive = true;
		}
		if(didConnect)
		{
			peer->_isConnectionActive = true;
			peer->_didNotifyConnection = true;
			peer->_isReconnectScheduled = false;
			peer->_reconnectInterval = RN_EOS_CONNECTION_RETRY_INTERVAL;
		}
		else if(!peer->_didNotifyConnection) peer->_isReconnectScheduled = true;
		EOSClientID clientID = peer->clientID;
		uint64 stream = peer->_reliableSendStream;
		Unlock();
		Retain();
		if(didDisconnect) HandleDidDisconnect(clientID, 0);
		Lock();
		auto current = _peers.find(senderUserID);
		bool isCurrent = _status == Connected && current != _peers.end() && !current->second._wantsDisconnect && current->second._reliableSendStream == stream;
		Unlock();
		if(isCurrent && shouldRespond) SendConnectRequest(senderUserID, true, usePending);
		if(isCurrent && didConnect) HandleDidConnect(clientID);
		Release();
	}

	EOSHost::Peer *EOSP2PClient::BindPeerLocked(EOS_ProductUserId productUserID)
	{
		if(!productUserID || _blockedPeers.count(productUserID) > 0) return nullptr;
		EOSClientID clientID = GetClientIDForProductUserID(productUserID);
		if(clientID == CLIENT_ID_NONE || clientID == _clientID) return nullptr;
		auto peer = _peers.find(productUserID);
		if(peer == _peers.end()) peer = _peers.insert(std::pair(productUserID, CreatePeer(clientID, productUserID))).first;
		auto mappedPeer = _idMap.find(clientID);
		if(peer->second.clientID != clientID || (mappedPeer != _idMap.end() && mappedPeer->second != productUserID)) return nullptr;
		peer->second._requiresBoundStream = _recoverableMesh;
		_idMap[clientID] = productUserID;
		return &peer->second;
	}

	void EOSP2PClient::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader))
		{
			return;
		}
		Lock();
		bool senderIsBlocked = _blockedPeers.count(senderUserID) > 0;
		auto closingPeer = _peers.find(senderUserID);
		bool acceptsClosingAcknowledgement = channel == 254 && closingPeer != _peers.end() && closingPeer->second._wantsDisconnect;
		Unlock();
		if(!acceptsClosingAcknowledgement && (senderIsBlocked || !ShouldAcceptPeer(senderUserID)))
		{
			return;
		}
		if(_recoverableMesh && !IsReservedChannel(channel))
		{
			ProtocolPacketHeader header;
			memcpy(&header, rawData, sizeof(header));
			if(header.packetType == ProtocolPacketTypeConnectRequest || header.packetType == ProtocolPacketTypeConnectResponse)
			{
				if(channel != 0 || header.packetID != 0 || header.dataLength != sizeof(ConnectionHandshake) || bytesWritten != sizeof(header) + sizeof(ConnectionHandshake)) return;
				ConnectionHandshake handshake;
				memcpy(&handshake, rawData + sizeof(header), sizeof(handshake));
				ReceiveMeshHandshake(senderUserID, header, handshake);
				return;
			}
			Lock();
			auto peer = _peers.find(senderUserID);
			bool isReady = peer != _peers.end() && peer->second._didNotifyConnection && !peer->second._wantsDisconnect;
			Unlock();
			if(!isReady) return;
		}
		EOSHost::ReceivedPacketInternal(rawData, bytesWritten, senderUserID, channel);
		if(IsReservedChannel(channel)) return; //Host control traffic
		
		Lock();
		if(_status == Disconnected || _status == Disconnecting)
		{
			Unlock();
			return;
		}

		Peer *peer = BindPeerLocked(senderUserID);
		if(!peer)
		{
			Unlock();
			RNWarning("EOS product user ID collision from " << senderUserID << "; disconnecting the P2P session");
			Disconnect();
			return;
		}

		EOSClientID senderID = peer->clientID;
		uint64 senderStream = peer->_reliableSendStream;
		if(!_recoverableMesh) peer->_isConnectionActive = true;
		DecodeResult result = DecodePackets(*peer, rawData, bytesWritten, channel);
		bool senderWasAdmitted = peer->_didNotifyConnection;

		bool didConnectLocalClient = false;
		std::vector<std::pair<EOS_ProductUserId, uint64>> connectedPeers;
		if(result.connectRequest)
		{
			peer->_isReconnectScheduled = false;
			if(senderUserID == _hostProductUserID && _status == Connecting)
			{
				_status = Connected;
				_connectionTimeout = 0.0f;
				didConnectLocalClient = true;
			}
			if(_status == Connected)
			{
				for(auto &mappedPeer : _peers)
				{
					if(!mappedPeer.second._isConnectionActive || mappedPeer.second._didNotifyConnection) continue;
					mappedPeer.second._didNotifyConnection = true;
					connectedPeers.emplace_back(mappedPeer.first, mappedPeer.second._reliableSendStream);
				}
			}
		}
		bool didCompleteHostMigration = TryCompleteHostMigration();
		Unlock();

		Retain();
		auto isSenderCurrent = [&]()
		{
			Lock();
			auto currentPeer = _peers.find(senderUserID);
			bool isCurrent = _status != Disconnected && _status != Disconnecting && currentPeer != _peers.end() &&
				currentPeer->second.clientID == senderID && currentPeer->second._reliableSendStream == senderStream;
			Unlock();
			return isCurrent;
		};
		if(result.multipartProgress && senderID != CLIENT_ID_NONE) HandleReliableMultipartProgress(senderID, channel);
		if(result.lostReliableData && senderID != CLIENT_ID_NONE && isSenderCurrent()) HandleReliablePacketLoss(senderID, channel);
		if(didCompleteHostMigration && isSenderCurrent()) HandleHostMigration();
		if(isSenderCurrent())
		{
			if(didConnectLocalClient) HandleDidConnect(_clientID);
			for(const auto &connectedPeer : connectedPeers)
			{
				Lock();
				auto currentPeer = _peers.find(connectedPeer.first);
				EOSClientID clientID = _status == Connected && currentPeer != _peers.end() &&
					!currentPeer->second._wantsDisconnect && currentPeer->second._reliableSendStream == connectedPeer.second ? currentPeer->second.clientID : CLIENT_ID_NONE;
				Unlock();
				if(clientID != CLIENT_ID_NONE) HandleDidConnect(clientID);
			}
		}
		for(Data *packet : result.packets)
		{
			if(senderWasAdmitted && isSenderCurrent())
			{
				ReceivedPacket(packet, senderID, channel);
			}
			packet->Release();
		}
		Release();
	}

	void EOSP2PClient::Update(float delta)
	{
		Retain();
		EOSHost::Update(delta); //This sends regular pings and handles sending of scheduled packets

		Lock();
		if(_status == Connecting)
		{
			_connectionTimeout += delta;
			if(_connectionTimeout > RN_EOS_CONNECTION_TIMEOUT) //Give up after some time and shut down the connection
			{
				Unlock();
				Disconnect();
				Lock();
			}
		}
		
		if(_status == Disconnected || _status == Disconnecting)
		{
			Unlock();
			Release();
			return;
		}

		std::vector<std::pair<EOS_ProductUserId, uint64>> peersToReconnect;
		for(auto &pair : _peers)
		{
			if(pair.second._isReconnectScheduled && !pair.second._wantsDisconnect)
			{
				pair.second._reconnectTimer += std::max(delta, 0.0f);
				if(pair.second._reconnectTimer >= pair.second._reconnectInterval && ShouldAcceptPeer(pair.first))
				{
					peersToReconnect.emplace_back(pair.first, pair.second._reliableSendStream);
					pair.second._reconnectTimer = 0.0f;
					if(_recoverableMesh) pair.second._reconnectInterval = std::min(pair.second._reconnectInterval * 2.0f, 12.0f);
				}
			}
		}

		Unlock();
		for(const auto &reconnectingPeer : peersToReconnect)
		{
			Lock();
			auto peer = _peers.find(reconnectingPeer.first);
			bool shouldReconnect = _status != Disconnected && _status != Disconnecting && peer != _peers.end() &&
				peer->second._reliableSendStream == reconnectingPeer.second && peer->second._isReconnectScheduled && !peer->second._wantsDisconnect;
			Unlock();
			if(shouldReconnect)
			{
				Connect(reconnectingPeer.first);
			}
		}
		Release();
	}

	void EOSP2PClient::OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data)
	{
		EOSP2PClient *client = static_cast<EOSP2PClient *>(Data->ClientData);
		if(!client || !client->ShouldAcceptPeer(Data->RemoteUserId)) return;
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world) return;

		client->Lock();
		bool isBlocked = client->_blockedPeers.count(Data->RemoteUserId) > 0;
		client->Unlock();
		if(isBlocked) return;

		EOS_P2P_AcceptConnectionOptions acceptOptions;
		acceptOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		acceptOptions.LocalUserId = Data->LocalUserId;
		acceptOptions.RemoteUserId = Data->RemoteUserId;
		acceptOptions.SocketId = Data->SocketId;

		if(EOS_P2P_AcceptConnection(world->GetP2PHandle(), &acceptOptions) != EOS_EResult::EOS_Success)
		{
			RNWarning("Failed to accept connection request from " << Data->RemoteUserId);
			return;
		}

		client->Lock();
		Peer *peer = client->BindPeerLocked(Data->RemoteUserId);
		if(!peer)
		{
			client->Unlock();
			RNWarning("EOS product user ID collision from " << Data->RemoteUserId << "; disconnecting the P2P session");
			client->Disconnect();
			return;
		}
		if(!peer->_didNotifyConnection)
		{
			peer->_isConnectionActive = !client->_recoverableMesh;
			peer->_isReconnectScheduled = client->_recoverableMesh;
		}
		client->Unlock();

		RNDebug("Accepted connection request from " << Data->RemoteUserId);

		if(!client->SendConnectRequest(Data->RemoteUserId))
		{
			RNWarning("Failed to send connection response to " << Data->RemoteUserId << "; scheduling a connection retry");
			client->Lock();
			auto retryPeer = client->_peers.find(Data->RemoteUserId);
			if(retryPeer != client->_peers.end())
			{
				retryPeer->second._reconnectTimer = 0.0f;
				retryPeer->second._isReconnectScheduled = true;
			}
			client->Unlock();
		}
	}

	void EOSP2PClient::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSP2PClient *client = static_cast<EOSP2PClient *>(Data->ClientData);
		// Local close paths own finalization, even if EOS delivers their notification after an explicit reconnect.
		if(!client || Data->Reason == EOS_EConnectionClosedReason::EOS_CCR_ClosedByLocalUser) return;
		client->Lock();
		bool isKnownPeer = client->_peers.count(Data->RemoteUserId) > 0;
		client->Unlock();
		if(!isKnownPeer) return;
		if(client->_recoverableMesh)
		{
			client->Retain();
			client->FailPeer(Data->RemoteUserId, false);
			client->Release();
			return;
		}
		client->Retain();
		bool shouldBlockReconnect = client->ShouldAcceptPeer(Data->RemoteUserId);
		client->Lock();
		if(shouldBlockReconnect) client->_blockedPeers.insert(Data->RemoteUserId);
		else client->_blockedPeers.erase(Data->RemoteUserId);
		client->Unlock();
		// EOS has already closed and flushed this stream; do not close it again from the notification.
		client->FinalizePeerDisconnect(Data->RemoteUserId, static_cast<uint16>(Data->Reason));
		client->Release();
	}

	void EOSP2PClient::HandlePeerFailure(EOS_ProductUserId productUserID)
	{
		FailPeer(productUserID, true);
	}

	void EOSP2PClient::FailPeer(EOS_ProductUserId productUserID, bool closeTransport)
	{
		if(!_recoverableMesh || !ShouldAcceptPeer(productUserID))
		{
			DisconnectClient(productUserID);
			return;
		}
		Lock();
		auto peer = _peers.find(productUserID);
		if(_status != Connected || _blockedPeers.count(productUserID) || peer == _peers.end() || peer->second._wantsDisconnect)
		{
			Unlock();
			return;
		}
		// Repeated close hints for an already retired attempt must not keep postponing its retry.
		if(!peer->second._didNotifyConnection && peer->second._isReconnectScheduled && !peer->second._isConnectionInterrupted)
		{
			Unlock();
			return;
		}
		EOSClientID clientID = peer->second.clientID;
		bool didNotify = peer->second._didNotifyConnection;
		ClearPeerData(peer->second);
		peer->second = CreatePeer(clientID, productUserID);
		peer->second._requiresBoundStream = true;
		peer->second._isReconnectScheduled = true;
		Unlock();
		if(closeTransport) ClosePeerTransport(productUserID);
		if(didNotify) HandleDidDisconnect(clientID, 0);
	}

	void EOSP2PClient::FinalizePeerDisconnect(EOS_ProductUserId productUserID, uint16 reason)
	{
		Lock();
		auto peer = _peers.find(productUserID);
		if(peer == _peers.end())
		{
			Unlock();
			return;
		}

		EOSClientID clientID = peer->second.clientID;
		bool didNotifyConnection = peer->second._didNotifyConnection;
		ClearPeerData(peer->second);
		auto idIterator = _idMap.find(clientID);
		if(idIterator != _idMap.end() && idIterator->second == productUserID) _idMap.erase(idIterator);
		_peers.erase(peer);

		bool didCompleteHostMigration = TryCompleteHostMigration();
		Unlock();

		RNDebug("Peer disconnected. peer id: " << clientID << " | product ID: " << productUserID);
		if(didNotifyConnection) HandleDidDisconnect(clientID, reason);
		Lock();
		bool migrationIsCurrent = _status == Connected && !_isHostMigrationPending && !_peers.count(productUserID);
		Unlock();
		if(didCompleteHostMigration && migrationIsCurrent) HandleHostMigration();
	}
} // namespace RN
