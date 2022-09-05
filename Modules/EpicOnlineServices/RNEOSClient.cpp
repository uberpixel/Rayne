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
		packetHeader.dataLength = 0;
		
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
		
		EOS_EResult result = EOS_P2P_SendPacket(world->GetP2PHandle(), &connectionOptions);
		
		if(result != EOS_EResult::EOS_Success)
		{
			RNDebug("Couldn't connect to server!");
			_status = Status::Disconnected;
			Unlock();
			return;
		}

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
		EOSHost::Update(delta); //Needs to go first as it picks out some packets! TODO: This also handles sending new packets, would reduce some latency if this was done at the end of this method
		
		Lock();
		if(_status == Status::Disconnected)
		{
			Unlock();
			return;
		}
		
		EOSWorld *world = EOSWorld::GetInstance();
		
		Peer &peer = _peers[0];
		
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
						//Nothing to clean up here as the data does not exist yet
						
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
					
					peer._multipartPacketTotalParts.erase(channel);
					peer._multipartPacketCurrentPart.erase(channel);
					peer._multipartPacketID.erase(channel);
					peer._multipartPacketData[channel]->Release();
					peer._multipartPacketData.erase(channel);
					
					continue;
				}
				
				//All data fits into one packet, though multiple internal packets maybe encoded in a single networking packet and it needs to be unpacked
				uint16 dataIndex = 0;
				while(dataIndex < bytesWritten)
				{
					ProtocolPacketHeader packetHeader;
					packetHeader.packetType = static_cast<ProtocolPacketType>(rawData[dataIndex + 0]);
					packetHeader.packetID = rawData[dataIndex + 1];
					packetHeader.dataLength = rawData[dataIndex + 2] | (rawData[dataIndex + 3] << 8);
					
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
						dataIndex += packetHeader.dataLength + 4;
						continue;
					}
					
					if(!IsPacketInOrder(packetHeader.packetType, 0, packetHeader.packetID, channel))
					{
						dataIndex += packetHeader.dataLength + 4;
						continue;
					}
					
					//Get data object from the packet without protocol header
					Data *data = Data::WithBytes(&rawData[dataIndex + 4], packetHeader.dataLength);
					dataIndex += packetHeader.dataLength + 4;
					
					Unlock();
					ReceivedPacket(data, 0, channel);
					Lock();
				}
			}
			
			delete[] rawData;
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
