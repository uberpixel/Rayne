//
//  RNEOSClient.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSClient.h"
#include "RNEOSWorld.h"

#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"
#include "eos_platform_prereqs.h"

namespace RN
{
	RNDefineMeta(EOSClient, EOSHost)

	EOSClient::EOSClient() : EOSHost(RNCSTR("FuckYeah"))
	{
		Lock();
		_status = Disconnected;
		_clientID = CLIENT_ID_NONE;

		EOSWorld *world = EOSWorld::GetInstance();

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_AddNotifyPeerConnectionClosedOptions disconnectListenerOptions;
		disconnectListenerOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONCLOSED_API_LATEST;
		disconnectListenerOptions.LocalUserId = world->GetUserID();
		disconnectListenerOptions.SocketId = &socketID;
		_connectionClosedNotificationID = EOS_P2P_AddNotifyPeerConnectionClosed(world->GetP2PHandle(), &disconnectListenerOptions, this, OnConnectionClosedCallback);

		Unlock();
	}

	EOSClient::~EOSClient()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(world && world->GetP2PHandle() && _connectionClosedNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionClosed(world->GetP2PHandle(), _connectionClosedNotificationID);
	}

	void EOSClient::Connect(EOS_ProductUserId serverProductID)
	{
		Lock();
		RN_ASSERT(_status == Status::Disconnected, "Already connected to a server.");

		_status = Connecting;
		ProtocolPacketHeader packetHeader {ProtocolPacketTypeConnectRequest, 0, 0};
		if(!SendRawPacket(serverProductID, 0, &packetHeader, sizeof(packetHeader), true))
		{
			RNDebug("Couldn't connect to server!");
			_status = Disconnected;
			Unlock();
			return;
		}

		const Peer &peer = CreatePeer(0, serverProductID);
		if(peer.internalID == NULL)
		{
			RNDebug("Couldn't connect to server!");
			_status = Disconnected;
			Unlock();
			return;
		}

		_peers.insert(std::pair(serverProductID, peer));
		_idMap[0] = serverProductID;

		Unlock();
	}
	
	void EOSClient::Disconnect()
	{
		Lock();
		if(_status == Disconnected || _status == Disconnecting)
		{
			Unlock();
			return;
		}

		if(_status == Connecting)
		{
			Unlock();
			ForceDisconnect(0);
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

		EOS_P2P_CloseConnections(world->GetP2PHandle(), &options);
		Unlock();
	}
	
	void EOSClient::DisconnectClient(EOS_ProductUserId productUserId)
	{
		if(productUserId == EOSWorld::GetInstance()->GetUserID() || _peers.find(productUserId) != _peers.end()) Disconnect();
		else
		{
			RNWarning("Trying to disconnect from unknown peer " << productUserId);
		}
	}

	void EOSClient::ForceDisconnect(uint16 reason)
	{
		Lock();
		_status = Disconnected;
		for(auto &peer : _peers) ClearPeerData(peer.second);
		_peers.clear();
		_idMap.clear();
		Unlock();

		RNDebug("Disconnected!");
		HandleDidDisconnect(0, reason);
	}

	void EOSClient::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader)) return;
		EOSHost::ReceivedPacketInternal(rawData, bytesWritten, senderUserID, channel);
		if(channel == 255) return; //This is a ping, handled by the Host class

		Lock();
		if(_status == Disconnected || _status == Disconnecting || _peers.empty() || _peers.begin()->first != senderUserID)
		{
			Unlock();
			return;
		}

		Peer &peer = _peers.begin()->second; //Server is the only peer
		peer._isConnectionActive = true;
		DecodeResult result = DecodePackets(peer, rawData, bytesWritten, channel);
		bool didConnect = false;
		for(const DecodedPacket &packet : result.packets)
		{
			if(packet.type == ProtocolPacketTypeConnectResponse && packet.packetID == 0 && packet.data->GetLength() == 0 && _status == Connecting)
			{
				_status = Connected;
				didConnect = true;
			}
		}
		Unlock();

		if(result.multipartProgress) HandleReliableMultipartProgress(0, channel);
		if(result.lostReliableData)
		{
			HandleReliablePacketLoss(0, channel);
			for(const DecodedPacket &packet : result.packets) packet.data->Release();
			return;
		}
		if(didConnect)
		{
			RNDebug("Connected!");
			HandleDidConnect(0);
		}
		for(const DecodedPacket &packet : result.packets)
		{
			if(packet.type == ProtocolPacketTypeData || packet.type == ProtocolPacketTypeReliableData) ReceivedPacket(packet.data, 0, channel);
			packet.data->Release();
		}
	}

	void EOSClient::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSClient *client = static_cast<EOSClient *>(Data->ClientData);

		RNDebug("Disconnected from Server");
		client->ForceDisconnect(static_cast<uint16>(Data->Reason));
	}
} // namespace RN
