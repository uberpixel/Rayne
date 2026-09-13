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

	EOSP2PClient::EOSP2PClient(bool isHost, String *socketID_, EOS_ProductUserId hostProductUserID) :
		EOSHost(socketID_), _hostClientID(CLIENT_ID_NONE), _hostProductUserID(hostProductUserID), _isHostMigrationPending(false), _connectionTimeout(0.0f)
	{
		Lock();
		RN_ASSERT(isHost || _hostProductUserID, "A non-host P2P client requires the lobby owner's product user ID.");
		_status = isHost ? Connected : Disconnected;

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
			if(_connectionRequestNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionRequest(world->GetP2PHandle(), _connectionRequestNotificationID);
			if(_connectionClosedNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
		}
	}

	void EOSP2PClient::Connect(EOS_ProductUserId remoteProductUserID)
	{
		if(!remoteProductUserID || remoteProductUserID == EOSWorld::GetInstance()->GetUserID() || !ShouldAcceptPeer(remoteProductUserID)) return;
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
			_status = didReconnectLocalHost ? Connected : Connecting;
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

		EOS_P2P_CloseConnectionsOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
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
		for(auto &peer : _peers) ClearPeerData(peer.second);
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
		Unlock();
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = productUserId;
		options.SocketId = &socketID;

		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
		FinalizePeerDisconnect(productUserId, 0);
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
		bool shouldConnectHost = !didCompleteHostMigration && hostProductUserId != EOSWorld::GetInstance()->GetUserID();
		Unlock();

		if(shouldConnectHost) Connect(hostProductUserId);
		if(didCompleteHostMigration) HandleHostMigration();
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

	bool EOSP2PClient::SendConnectRequest(EOS_ProductUserId receiverID)
	{
		ProtocolPacketHeader header {ProtocolPacketTypeConnectRequest, 0, 0};
		return SendRawPacket(receiverID, 0, &header, sizeof(header), true);
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
		_idMap[clientID] = productUserID;
		return &peer->second;
	}

	void EOSP2PClient::HandleReliablePacketLoss(EOSClientID clientID)
	{
		RNWarning("Unrecoverable reliable EOS data loss from peer " << clientID << "; disconnecting the P2P session");
		Disconnect();
	}

	void EOSP2PClient::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader) || !ShouldAcceptPeer(senderUserID)) return;
		Lock();
		bool senderIsBlocked = _blockedPeers.count(senderUserID) > 0;
		Unlock();
		if(senderIsBlocked) return;
		EOSHost::ReceivedPacketInternal(rawData, bytesWritten, senderUserID, channel);
		if(channel == 255) return; //This is a ping, handled by the Host class
		
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
		peer->_isConnectionActive = true;
		DecodeResult result = DecodePackets(*peer, rawData, bytesWritten, channel);
		std::vector<Data *> receivedPackets;
		bool receivedConnectRequest = false;
		for(const DecodedPacket &packet : result.packets)
		{
			if(packet.type == ProtocolPacketTypeConnectRequest)
			{
				receivedConnectRequest = packet.packetID == 0 && packet.data->GetLength() == 0;
				continue;
			}

			if((packet.type == ProtocolPacketTypeData || packet.type == ProtocolPacketTypeReliableData) && peer->_didNotifyConnection)
				receivedPackets.push_back(packet.data->Retain());
		}

		bool didConnectLocalClient = false;
		std::vector<EOSClientID> connectedPeers;
		if(receivedConnectRequest)
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
					connectedPeers.push_back(mappedPeer.second.clientID);
				}
			}
		}
		bool didCompleteHostMigration = TryCompleteHostMigration();
		Unlock();

		if(result.multipartProgress && senderID != CLIENT_ID_NONE) HandleReliableMultipartProgress(senderID, channel);
		if(result.lostReliableData && senderID != CLIENT_ID_NONE)
		{
			Retain();
			HandleReliablePacketLoss(senderID, channel);
			for(Data *packet : receivedPackets) packet->Release();
			for(const DecodedPacket &packet : result.packets) packet.data->Release();
			Release();
			return;
		}
		if(didConnectLocalClient) HandleDidConnect(_clientID);
		for(EOSClientID clientID : connectedPeers) HandleDidConnect(clientID);
		if(didCompleteHostMigration) HandleHostMigration();
		for(Data *packet : receivedPackets)
		{
			ReceivedPacket(packet, senderID, channel);
			packet->Release();
		}
		for(const DecodedPacket &packet : result.packets) packet.data->Release();
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

		std::vector<EOS_ProductUserId> peersToReconnect;
		std::vector<EOSClientID> peersToDisconnect;
		for(auto &pair : _peers)
		{
			if(pair.second._isReconnectScheduled)
			{
				pair.second._reconnectTimer += std::max(delta, 0.0f);
				if(pair.second._reconnectTimer >= RN_EOS_CONNECTION_RETRY_INTERVAL && ShouldAcceptPeer(pair.first))
				{
					peersToReconnect.push_back(pair.first);
					pair.second._reconnectTimer = 0.0f;
				}
			}
			if(pair.second._wantsDisconnect)
			{
				pair.second._disconnectDelay -= delta;
				if(pair.second._disconnectDelay < 0.0f)
				{
					peersToDisconnect.push_back(pair.second.clientID);
				}
			}
		}

		Unlock();
		for(EOSClientID clientID : peersToDisconnect) DisconnectClient(clientID);
		for(EOS_ProductUserId peer : peersToReconnect) Connect(peer);
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
		peer->_isConnectionActive = true;
		peer->_isReconnectScheduled = false;
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
		if(!client) return;

		client->Lock();
		auto peer = client->_peers.find(Data->RemoteUserId);
		if(peer == client->_peers.end())
		{
			client->Unlock();
			return;
		}

		EOSClientID id = peer->second.clientID;
		peer->second._isConnectionActive = false;
		bool shouldReconnect = client->_status != Disconnected && client->_status != Disconnecting && client->_blockedPeers.count(Data->RemoteUserId) == 0 && client->ShouldAcceptPeer(Data->RemoteUserId);
		if(shouldReconnect)
		{
			for(auto &assembly : peer->second._multipartAssemblies) assembly.second.age = 0.0f;
			client->Unlock();
			RNWarning("Connection to peer " << id << " closed while the peer is still present; reconnecting");
			client->Connect(Data->RemoteUserId);
			client->HandleConnectionInterrupted(id);
			return;
		}

		client->Unlock();
		client->FinalizePeerDisconnect(Data->RemoteUserId, static_cast<uint16>(Data->Reason));
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
		if(didCompleteHostMigration) HandleHostMigration();
	}
} // namespace RN
