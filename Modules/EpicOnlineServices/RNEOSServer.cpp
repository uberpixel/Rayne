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
		_connectionRequestNotificationID = EOS_P2P_AddNotifyPeerConnectionRequest(world->GetP2PHandle(), &connectListenerOptions, this, OnConnectionRequestCallback);
		
		EOS_P2P_AddNotifyPeerConnectionClosedOptions disconnectListenerOptions = {0};
		disconnectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
		disconnectListenerOptions.LocalUserId = world->GetUserID();
		disconnectListenerOptions.SocketId = &socketID;
		_connectionClosedNotificationID = EOS_P2P_AddNotifyPeerConnectionClosed(world->GetP2PHandle(), &disconnectListenerOptions, this, OnConnectionClosedCallback);
		
		Unlock();
	}
		
	EOSServer::~EOSServer()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
		EOS_P2P_RemoveNotifyPeerConnectionRequest(world->GetP2PHandle(), _connectionRequestNotificationID);
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
			
			uint8 *rawData = new uint8[nextPacketSize];
			
			if(EOS_P2P_ReceivePacket(world->GetP2PHandle(), &receiveOptions, &senderUserID, &socketID, &channel, rawData, &bytesWritten) != EOS_EResult::EOS_Success)
			{
				RNDebug("Failed receiving Data");
				break;
			}
			
			uint16 senderID = GetUserIDForInternalID(senderUserID);
			Peer &peer = _peers[senderID];
			
			if(static_cast<ProtocolPacketType>(rawData[0]) == ProtocolPacketTypeReliableDataMultipart)
			{
				//If this is multipart data, wait for all parts before passing them on
				ProtocolPacketHeaderMultipart packetHeader;
				packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[0]);
				packetHeader.packetID = rawData[1];
				packetHeader.dataPart = rawData[2] | (rawData[3] << 8);
				packetHeader.totalDataParts = rawData[4] | (rawData[5] << 8);
				
				RNDebug("Received multipart data (" << packetHeader.packetID <<  "), part " << packetHeader.dataPart << " of " << packetHeader.totalDataParts);
				
				if(peer._multipartPacketTotalParts.count(channel) == 0)
				{
					//Received new multipart data!
					
					if(packetHeader.dataPart != 0)
					{
						//Received new multipart data, but it's missing previous parts!?
						//TODO: Consider disconnecting user? For now just skip the data. But this case means that something is seriously wrong.
						//Nothing to clean up here, as no data exists at this point
						
						RNDebug("Received multipart data but it's bad!");
						
						continue;
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
						
						RNDebug("Received multipart data but it's bad!");
						
						peer._multipartPacketTotalParts.erase(channel);
						peer._multipartPacketCurrentPart.erase(channel);
						peer._multipartPacketID.erase(channel);
						peer._multipartPacketData[channel]->Release();
						peer._multipartPacketData.erase(channel);
						
						continue;
					}
					
					peer._multipartPacketCurrentPart[channel] = packetHeader.dataPart;
				}
				
				//Get data object from the packet without protocol header
				Data *data = Data::WithBytes(&rawData[6], bytesWritten - 6);
				peer._multipartPacketData[channel]->Append(data);
				
				if(packetHeader.dataPart + 1 >= packetHeader.totalDataParts)
				{
					RNDebug("Received full multipart data");
					
					Unlock();
					ReceivedPacket(peer._multipartPacketData[channel], 0, channel);
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
					//Got non-multipart data on a channel that got multipart data before that is still incomplete.
					//TODO: Consider disconnecting user? For now just skip the data. But this case means that something is seriously wrong.
					
					RNDebug("Received multipart data but it's incomplete!");
					
					peer._multipartPacketTotalParts.erase(channel);
					peer._multipartPacketCurrentPart.erase(channel);
					peer._multipartPacketID.erase(channel);
					peer._multipartPacketData[channel]->Release();
					peer._multipartPacketData.erase(channel);
					
					continue;
				}
			
				size_t dataIndex = 0;
				while(dataIndex < bytesWritten)
				{
					ProtocolPacketHeader packetHeader;
					packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[dataIndex + 0]);
					packetHeader.packetID = rawData[dataIndex + 1];
					packetHeader.dataLength = rawData[dataIndex + 2] | (rawData[dataIndex + 3] << 8);
					
					if(packetHeader.packetType == ProtocolPacketTypeConnectRequest)
					{
						if(packetHeader.packetID == 0)
						{
							//This is handled in OnConnectionRequestCallback
							//TODO: Maybe move some of it here instead to not require delayed delivery for the response
						}
						else
						{
							RNDebug("Malformed connect request");
						}
						dataIndex += packetHeader.dataLength + 4;
						continue;
					}
					
					if(!IsPacketInOrder(packetHeader.packetType, senderID, packetHeader.packetID, channel) || _peers[senderID]._wantsDisconnect) //Don't process any more data from a user that is about to be disconnected
					{
						dataIndex += packetHeader.dataLength + 4;
						continue;
					}
					
					//Get data object from the packet without protocol header
					Data *data = Data::WithBytes(&rawData[dataIndex + 4], packetHeader.dataLength);
					dataIndex += packetHeader.dataLength + 4;
					
					Unlock();
					ReceivedPacket(data, senderID, channel);
					Lock();
				}
			}
			
			delete[] rawData;
		}
		
		std::vector<uint16> peersToDisconnect;
		for(auto &pair : _peers)
		{
			if(pair.second._wantsDisconnect)
			{
				pair.second._disconnectDelay -= delta;
				if(pair.second._disconnectDelay < 0.0f)
				{
					peersToDisconnect.push_back(pair.first);
				}
			}
		}
		
		for(uint16 clientID : peersToDisconnect)
		{
			DisconnectUser(clientID, 0);
		}
		
		Unlock();
	}

	size_t EOSServer::GetNumberOfConnectedUsers() const
	{
		return _activeUserIDs.size();
	}

	void EOSServer::DisconnectUserDelayed(uint16 userID, uint16 data, float delay)
	{
		Lock();
		_peers[userID]._disconnectDelay = delay;
		_peers[userID]._wantsDisconnect = true;
		Unlock();
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
		options.RemoteUserId = _peers[userID].internalID;
		options.SocketId = &socketID;
		
		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
		Unlock();
	}

	void EOSServer::DisconnectAll()
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
		
		EOS_P2P_CloseConnectionsOptions options = {0};
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.SocketId = &socketID;
		
		EOS_P2P_CloseConnections(world->GetP2PHandle(), &options);
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
		const Peer &peer = server->CreatePeer(server->GetUserID(), Data->RemoteUserId);
		server->SetTimeout(peer.userID, 0, 0, 0);
		server->_peers.insert(std::pair<uint16, Peer>(peer.userID, peer));
		
		
		ProtocolPacketHeader packetHeader;
		packetHeader.packetType = ProtocolPacketTypeConnectResponse;
		packetHeader.packetID = 0;
		packetHeader.dataLength = 0;
		
		EOS_P2P_SendPacketOptions connectConfirmOptions = {0};
		connectConfirmOptions.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		connectConfirmOptions.SocketId = &socketID;
		connectConfirmOptions.LocalUserId = world->GetUserID();
		connectConfirmOptions.RemoteUserId = Data->RemoteUserId;
		connectConfirmOptions.Channel = 0;
		connectConfirmOptions.Reliability = EOS_EPacketReliability::EOS_PR_ReliableOrdered;
		connectConfirmOptions.bAllowDelayedDelivery = true;
		connectConfirmOptions.DataLengthBytes = sizeof(packetHeader);
		connectConfirmOptions.Data = &packetHeader;
		
		EOS_P2P_SendPacket(world->GetP2PHandle(), &connectConfirmOptions);

		server->HandleDidConnect(peer.userID);
	}

	void EOSServer::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSServer *server = static_cast<EOSServer*>(Data->ClientData);
		
		uint16 id = server->GetUserIDForInternalID(Data->RemoteUserId);
		
		RNDebug("Client disconnected: " << id);
		server->_peers.erase(id);
		server->ReleaseUserID(id);
		server->HandleDidDisconnect(id, static_cast<uint16>(Data->Reason));
	}
}
