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

namespace RN
{
	RNDefineMeta(EOSP2PClient, EOSHost)

	EOSP2PClient::EOSP2PClient(bool isHost, String *socketID_) :
		EOSHost(socketID_), _connectionTimeout(0.0f), _lastUsedClientID(0)
	{
		Lock();
		_status = isHost ? Connected : Disconnected;
		_clientID = isHost ? 0 : CLIENT_ID_NONE;
		_hostClientID = isHost ? 0 : CLIENT_ID_NONE;

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

	EOSP2PClient::~EOSP2PClient()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_RemoveNotifyPeerConnectionRequest(world->GetP2PHandle(), _connectionRequestNotificationID);
		EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
	}

	void EOSP2PClient::Connect(EOS_ProductUserId remoteProductUserID)
	{
		RNDebug("Connecting to " << remoteProductUserID);
		Lock();
		RN_ASSERT(_status != Disconnecting, "Cannot connect in current status." + _status);

		if(_status == Disconnected) _status = Connecting;
		_connectionTimeout = 0.0f;

		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		ProtocolPacketHeader packetHeader;
		packetHeader.packetType = ProtocolPacketTypeConnectRequest;
		packetHeader.packetID = 0;
		packetHeader.dataLength = 0;

		EOS_P2P_SendPacketOptions connectionOptions = {};
		connectionOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		connectionOptions.SocketId = &socketID;
		connectionOptions.LocalUserId = world->GetUserID();
		connectionOptions.RemoteUserId = remoteProductUserID;
		connectionOptions.Channel = 0;
		connectionOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		connectionOptions.bAllowDelayedDelivery = true;
		connectionOptions.DataLengthBytes = sizeof(packetHeader);
		connectionOptions.Data = &packetHeader;

		EOS_EResult result = EOS_P2P_SendPacket(world->GetP2PHandle(), &connectionOptions);

		if(result != EOS_EResult::EOS_Success) //TODO only do this when connecting to host failed, this should usually succeed, even if the peer is not reachable as the sending happens later!
		{
			RNDebug("Failed to connect to " << remoteProductUserID);
			Unlock();
			Disconnect();
			return;
		}

		//Create peer with unknown user ID. Will be set via connection response. The insert will just quietly fail if there already was a peer with the same product id
		_peers.insert(std::pair(remoteProductUserID, CreatePeer(CLIENT_ID_NONE, remoteProductUserID)));

		Unlock();
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

		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionsOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;

		Retain();
		if(EOS_P2P_CloseConnections(world->GetP2PHandle(), &options) == EOS_EResult::EOS_Success)
		{
			RNDebug("Closed all connections");
			_status = Disconnected;
			Unlock();
			HandleDidDisconnect(_clientID, 0); //OnConnectionClosedCallback is not guaranteed to be called when lobby closed, so explicitly call handler here
			Lock();
			_peers.clear();
			_idMap.clear();
			_clientID = CLIENT_ID_NONE;
			_hostClientID = CLIENT_ID_NONE;
			Unlock();
		}
		else
		{
			RNWarning("Failed closing all connections");
			ForceDisconnect(0);
			Unlock();
		}
		Release();
	}
	
	void EOSP2PClient::DisconnectClient(EOS_ProductUserId productUserId)
	{
		if(productUserId == EOSWorld::GetInstance()->GetUserID())
		{
			Disconnect();
		}
		else if(_peers.find(productUserId) != _peers.end())
		{
			DisconnectClient(_peers[productUserId].clientID);
		}
		else
		{
			RNWarning("Trying to disconnect unknown peer " << productUserId);
		}
	}

	void EOSP2PClient::DisconnectClient(uint8 clientID)
	{
		if(clientID == _clientID)
		{
			Disconnect();
			return;
		}
		if(_idMap.find(clientID) == _idMap.end())
		{
			RNWarning("Trying to disconnect unknown client " << clientID);
			return;
		}
		
		RNDebug("Disconnecting user " << clientID);
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_CloseConnectionOptions options;
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = _idMap[clientID];
		options.SocketId = &socketID;

		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSP2PClient::DisconnectClientDelayed(uint8 clientID, float delay)
	{
		RNDebug("Disconnecting client " << clientID << " in " << delay << " seconds");
		Lock();
		EOS_ProductUserId internalID = _idMap[clientID];
		if(_peers.find(internalID) != _peers.end())
		{
			_peers[internalID]._disconnectDelay = delay;
			_peers[internalID]._wantsDisconnect = true;
		}
		Unlock();
	}

	void EOSP2PClient::MigrateHost(EOS_ProductUserId hostProductUserId)
	{
		bool isNewClient = false;
		if(_clientID == CLIENT_ID_NONE)
		{
			isNewClient = true;
			if(hostProductUserId == EOSWorld::GetInstance()->GetUserID())
			{
				_clientID = 0; //This is save as 0 is reserved for hosts, though hosts can also have other ids
			}
			else
			{
				//Don't have a client id yet, but the host changed, so the original host won't provide it. Request it again from the new host.
				Connect(hostProductUserId);
			}
		}

		if(hostProductUserId == EOSWorld::GetInstance()->GetUserID())
		{
			_hostClientID = _clientID;
			RNDebug("Took over host role.");
		}
		else if(_peers.find(hostProductUserId) != _peers.end())
		{
			_hostClientID = _peers[hostProductUserId].clientID;
			RNDebug("Server role was transferred to client " << _hostClientID);
		}
		else
		{
			RNWarning("Host migrated to " << hostProductUserId << ", but that peer is not known.");
		}

		if(!isNewClient)
		{
			HandleHostMigration();
		}
	}

	void EOSP2PClient::ForceDisconnect(uint16 reason)
	{
		RNDebug("ForceDisconnect()");
		Retain();
		Lock();
		_status = Disconnected;
		Unlock();
		HandleDidDisconnect(_clientID, reason);
		Lock();
		_peers.clear();
		_idMap.clear();
		_clientID = CLIENT_ID_NONE;
		_hostClientID = CLIENT_ID_NONE;
		Unlock();
		Release();
	}

	uint8 EOSP2PClient::GetUnusedClientID()
	{
		_lastUsedClientID += 1;
		_lastUsedClientID = std::max(_lastUsedClientID, (uint8)1); //Starting by 1 so nobody but the host can have 0, so now in case of the host getting migrated to a newly joined user that doesn't have an id yet, 0 can safely be picked
		for(; _lastUsedClientID < CLIENT_ID_RESERVED; _lastUsedClientID++)
		{
			if(_idMap.find(_lastUsedClientID) == _idMap.end() && _lastUsedClientID != _clientID)
			{
				return _lastUsedClientID;
			}
		}
		
		//Search again from the beginning before giving up
		_lastUsedClientID = 1;
		for(; _lastUsedClientID < CLIENT_ID_RESERVED; _lastUsedClientID++)
		{
			if(_idMap.find(_lastUsedClientID) == _idMap.end() && _lastUsedClientID != _clientID)
			{
				return _lastUsedClientID;
			}
		}

		return CLIENT_ID_NONE;
	}

	void EOSP2PClient::AssignClientID(uint8 clientID)
	{
		RNDebug("Was assigned client ID " << clientID);
		_clientID = clientID;

		//Let peers know
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		for(auto peer : _peers)
		{
			ProtocolPacketHeader packetHeader;
			packetHeader.packetType = ProtocolPacketTypeConnectResponse;
			packetHeader.packetID = 0;
			packetHeader.dataLength = 2;

			Data *packetData = new Data();
			packetData->Append(&packetHeader, 4);
			packetData->Append(&_clientID, sizeof(uint8));
			packetData->Append(&peer.second.clientID, sizeof(uint8));

			EOS_P2P_SendPacketOptions sendPacketOptions = {};
			sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
			sendPacketOptions.SocketId = &socketID;
			sendPacketOptions.LocalUserId = world->GetUserID();
			sendPacketOptions.RemoteUserId = peer.first;
			sendPacketOptions.Channel = 0;
			sendPacketOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
			sendPacketOptions.bAllowDelayedDelivery = true;
			sendPacketOptions.DataLengthBytes = packetData->GetLength();
			sendPacketOptions.Data = packetData->GetBytes();

			EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
			
			packetData->Release();
		}
	}

	void EOSP2PClient::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		EOSHost::ReceivedPacketInternal(rawData, bytesWritten, senderUserID, channel);
		if(channel == 255) return; //This is a ping, handled by the Host class
		
		Lock();
		if(_status == Disconnected || _status == Disconnecting)
		{
			Unlock();
			return;
		}
		
		if(_peers.find(senderUserID) == _peers.end())
		{
			//Create peer with unknown user ID. Will be set via connection response
			RNDebug("Received packet from unknown user " << senderUserID << " (creating peer)");
			_peers.insert(std::pair(senderUserID, CreatePeer(CLIENT_ID_NONE, senderUserID)));
		}
		uint8 senderID = _peers[senderUserID].clientID;
		Peer &peer = _peers[senderUserID];

		if(static_cast<ProtocolPacketType>(rawData[0]) == ProtocolPacketTypeReliableDataMultipart)
		{
			//If this is multipart data, wait for all parts before passing them on
			ProtocolPacketHeaderMultipart packetHeader;
			packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[0]);
			packetHeader.packetID = rawData[1];
			packetHeader.dataPart = rawData[2] | rawData[3] << 8;
			packetHeader.totalDataParts = rawData[4] | rawData[5] << 8;

			//RNDebug("Received multipart data (" << packetHeader.packetID <<  "), part " << packetHeader.dataPart << " of " << packetHeader.totalDataParts);

			if(peer._multipartPacketTotalParts.count(channel) == 0)
			{
				//Received new multipart data!

				if(packetHeader.dataPart != 0)
				{
					//Received new multipart data, but it's missing previous parts!?
					//TODO: Consider disconnecting user? For now just skip the data. But this case means that something is seriously wrong.
					//Nothing to clean up here as the data does not exist yet

					RNDebug("Received multipart data but it's missing previous parts!");

					Unlock();
					return;
				}

				peer._multipartPacketTotalParts[channel] = packetHeader.totalDataParts;
				peer._multipartPacketCurrentPart[channel] = packetHeader.dataPart;
				peer._multipartPacketID[channel] = packetHeader.packetID;
				peer._multipartPacketData[channel] = new Data();
			}
			else
			{
				//Received another part of multipart data!

				if(peer._multipartPacketCurrentPart[channel] + 1 != packetHeader.dataPart || peer._multipartPacketTotalParts[channel] != packetHeader.totalDataParts || peer._multipartPacketID[channel] != packetHeader.packetID)
				{
					//Received multipart data, but found some inconsistency
					//TODO: Consider disconnecting user? For now just skip the data. But this case means that something is seriously wrong.

					RNDebug("Received multipart data but it's missing parts!");

					peer._multipartPacketTotalParts.erase(channel);
					peer._multipartPacketCurrentPart.erase(channel);
					peer._multipartPacketID.erase(channel);
					peer._multipartPacketData[channel]->Release();
					peer._multipartPacketData.erase(channel);

					Unlock();
					return;
				}

				peer._multipartPacketCurrentPart[channel] = packetHeader.dataPart;
			}

			//Append data from the packet without protocol header
			peer._multipartPacketData[channel]->Append(&rawData[6], bytesWritten - 6);

			if(packetHeader.dataPart + 1 >= packetHeader.totalDataParts)
			{
				//RNDebug("Received full multipart data");
				Unlock();
				ReceivedPacket(peer._multipartPacketData[channel], senderID, channel);
				Lock();

				peer._multipartPacketTotalParts.erase(channel);
				peer._multipartPacketCurrentPart.erase(channel);
				peer._multipartPacketID.erase(channel);
				peer._multipartPacketData[channel]->Release();
				peer._multipartPacketData.erase(channel);
			}
		}
		else
		{
			if(peer._multipartPacketTotalParts.count(channel) != 0)
			{
				if(static_cast<ProtocolPacketType>(rawData[0]) == ProtocolPacketTypeReliableData)
				{
					//Got non-multipart reliable data on a channel that got multipart data before that is still incomplete.
					//TODO: Consider disconnecting user? For now just skip the data. But this case means that something is seriously wrong.

					RNDebug("Received multipart data but it's incomplete!");

					peer._multipartPacketTotalParts.erase(channel);
					peer._multipartPacketCurrentPart.erase(channel);
					peer._multipartPacketID.erase(channel);
					peer._multipartPacketData[channel]->Release();
					peer._multipartPacketData.erase(channel);

					Unlock();
					return;
				}
				else
				{
					//Got out of order unreliable data while still waiting for multipart data. Just ignore and wait for remaining multipart data.
					Unlock();
					return;
				}
			}

			//All data fits into one packet, though multiple internal packets maybe encoded in a single networking packet and it needs to be unpacked
			uint16 dataIndex = 0;
			while(dataIndex < bytesWritten)
			{
				ProtocolPacketHeader packetHeader;
				packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[dataIndex + 0]);
				packetHeader.packetID = rawData[dataIndex + 1];
				packetHeader.dataLength = rawData[dataIndex + 2] | rawData[dataIndex + 3] << 8;
				dataIndex += 4;

				if(packetHeader.packetType == ProtocolPacketTypeConnectRequest)
				{
					RNDebug("Received connect request from " << senderUserID);
					if(packetHeader.packetID == 0)
					{
						//This is handled in OnConnectionRequestCallback
						//TODO: Maybe move some of it here instead to not require delayed delivery for the response
					}
					else
					{
						RNDebug("Malformed connect request");
					}
					dataIndex += packetHeader.dataLength; //Should be 0
					continue;
				}

				if(packetHeader.packetType == ProtocolPacketTypeConnectResponse)
				{
					if(packetHeader.packetID == 0 && packetHeader.dataLength == 2)
					{
						uint8 remoteClientID = rawData[dataIndex]; //Client id of sender
						uint8 ownClientID = rawData[dataIndex + 1]; //Client id sender has assigned to receiver
						RNDebug("Received connect response from " << senderUserID << " with client ID " << remoteClientID << " and own client ID " << ownClientID);

						//Received remote user ID. Do this first to add peer for client id broadcasting
						bool didConnect = false;
						if(_peers[senderUserID].clientID == CLIENT_ID_NONE && remoteClientID != CLIENT_ID_NONE)
						{
							_peers[senderUserID].clientID = remoteClientID;
							_idMap[remoteClientID] = senderUserID;
							didConnect = true;
						}

						//Received own client ID from server
						if(_clientID == CLIENT_ID_NONE && ownClientID != CLIENT_ID_NONE && remoteClientID != CLIENT_ID_NONE)
						{
							_hostClientID = remoteClientID;
							AssignClientID(ownClientID); //Broadcasts freshly assigned id to peers
							_status = Connected;
						}

						LogPeers();

						//Handle connection after the server client id was set
						if(didConnect)
						{
							Unlock();
							HandleDidConnect(remoteClientID);
							Lock();
						}
					}
					else
					{
						RNDebug("Malformed connect response");
					}
					
					dataIndex += packetHeader.dataLength;
					continue;
				}

				if(!IsPacketInOrder(packetHeader.packetType, senderUserID, packetHeader.packetID, channel))
				{
					dataIndex += packetHeader.dataLength;
					continue;
				}

				//Get data object from the packet without protocol header
				Data *data = Data::WithBytes(&rawData[dataIndex], packetHeader.dataLength);
				dataIndex += packetHeader.dataLength;

				Unlock();
				ReceivedPacket(data, senderID, channel);
				Lock();
			}
		}
		
		Unlock();
	}

	void EOSP2PClient::Update(float delta)
	{
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
			return;
		}

		std::vector<uint8> peersToDisconnect;
		for(auto &pair : _peers)
		{
			if(pair.second._wantsDisconnect)
			{
				pair.second._disconnectDelay -= delta;
				if(pair.second._disconnectDelay < 0.0f)
				{
					peersToDisconnect.push_back(pair.second.clientID);
				}
			}
		}

		for(uint8 clientID : peersToDisconnect)
		{
			DisconnectClient(clientID);
		}

		Unlock();
	}

	void EOSP2PClient::LogPeers() const
	{
		RNDebug("Peers known: " << _peers.size());
		for(auto peer : _peers)
		{
			RNDebug("Peer: " << peer.first << " | client id: " << peer.second.clientID);
		}
	}

	void EOSP2PClient::OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data)
	{
		EOSP2PClient *client = static_cast<EOSP2PClient *>(Data->ClientData);
		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_AcceptConnectionOptions acceptOptions;
		acceptOptions.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
		acceptOptions.LocalUserId = Data->LocalUserId;
		acceptOptions.RemoteUserId = Data->RemoteUserId;
		acceptOptions.SocketId = Data->SocketId;

		EOS_P2P_AcceptConnection(EOSWorld::GetInstance()->GetP2PHandle(), &acceptOptions);

		RNDebug("Accepted connection request from " << Data->RemoteUserId);

		ProtocolPacketHeader packetHeader;
		packetHeader.packetType = ProtocolPacketTypeConnectResponse;
		packetHeader.packetID = 0;
		packetHeader.dataLength = 2;

		RN::Data *packetData = new RN::Data();
		packetData->Append(&packetHeader, 4);
		packetData->Append(&client->_clientID, sizeof(uint8));
		uint8 remoteClientID = client->IsHost() ? client->GetUnusedClientID() : CLIENT_ID_NONE;
		packetData->Append(&remoteClientID, sizeof(uint8));

		const Peer &peer = client->CreatePeer(remoteClientID, Data->RemoteUserId);
		client->_peers.insert(std::pair(Data->RemoteUserId, peer));

		//If hosting the session, assign a new client user id, else ask for user id by sending connection request
		if(client->IsHost())
		{
			RNDebug("Assigning client id " << peer.clientID);
			client->_idMap[remoteClientID] = Data->RemoteUserId;
			client->HandleDidConnect(peer.clientID);
		}
		else
		{
			RNDebug("Requesting user id from " << Data->RemoteUserId);
			client->Connect(Data->RemoteUserId); //Request client id
		}

		client->LogPeers();

		EOS_P2P_SendPacketOptions sendPacketOptions = {};
		sendPacketOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		sendPacketOptions.SocketId = Data->SocketId;
		sendPacketOptions.LocalUserId = world->GetUserID();
		sendPacketOptions.RemoteUserId = Data->RemoteUserId;
		sendPacketOptions.Channel = 0;
		sendPacketOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		sendPacketOptions.bAllowDelayedDelivery = true;
		sendPacketOptions.DataLengthBytes = packetData->GetLength();
		sendPacketOptions.Data = packetData->GetBytes();

		EOS_P2P_SendPacket(world->GetP2PHandle(), &sendPacketOptions);
		
		packetData->Release();
	}

	void EOSP2PClient::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSP2PClient *client = static_cast<EOSP2PClient *>(Data->ClientData);
		if(client->_peers.find(Data->RemoteUserId) == client->_peers.end()) return;
		
		uint8 id = client->_peers[Data->RemoteUserId].clientID;
		client->_idMap.erase(id);
		client->_peers.erase(Data->RemoteUserId);
		RNDebug("Peer disconnected. peer id: " << id << " | product ID: " << Data->RemoteUserId);

		client->HandleDidDisconnect(id, static_cast<uint8>(Data->Reason));
	}
} // namespace RN
