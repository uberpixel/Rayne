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
		_status = Disconnecting;
		for(auto &peer : _peers)
		{
			peer.second._wantsDisconnect = true;
		}
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
		EOS_P2P_CloseConnections(world->GetP2PHandle(), &options);
		ForceDisconnect(0);
		Release();
	}

	void EOSClient::DisconnectClient(EOS_ProductUserId productUserId)
	{
		Lock();
		bool isKnownPeer = productUserId == EOSWorld::GetInstance()->GetUserID() || _peers.find(productUserId) != _peers.end();
		Unlock();
		if(isKnownPeer) Disconnect();
		else
		{
			RNWarning("Trying to disconnect from unknown peer " << productUserId);
		}
	}

	void EOSClient::ForceDisconnect(uint16 reason)
	{
		Lock();
		if(_status == Disconnected && _peers.empty())
		{
			Unlock();
			return;
		}
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
		if(IsReservedChannel(channel)) return; //Host control traffic

		Lock();
		if(_status == Disconnected || _status == Disconnecting || _peers.empty() || _peers.begin()->first != senderUserID)
		{
			Unlock();
			return;
		}

		Peer &peer = _peers.begin()->second; //Server is the only peer
		peer._isConnectionActive = true;
		uint64 senderStream = peer._reliableSendStream;
		DecodeResult result = DecodePackets(peer, rawData, bytesWritten, channel);
		bool didConnect = false;
		if(result.connectResponse && _status == Connecting)
		{
			_status = Connected;
			peer._didNotifyConnection = true;
			didConnect = true;
		}
		Unlock();

		Retain();
		auto isSenderCurrent = [&]()
		{
			Lock();
			auto currentPeer = _peers.find(senderUserID);
			bool isCurrent = _status != Disconnected && _status != Disconnecting && currentPeer != _peers.end() &&
				currentPeer->second.clientID == 0 && currentPeer->second._reliableSendStream == senderStream && !currentPeer->second._wantsDisconnect;
			Unlock();
			return isCurrent;
		};

		if(!result.lostReliableData && isSenderCurrent()) InflateDecodedPackets(result);

		if(result.multipartProgress && isSenderCurrent()) HandleReliableMultipartProgress(0, channel);
		if(result.lostReliableData && isSenderCurrent()) HandleReliablePacketLoss(0, channel);
		if(!result.lostReliableData && didConnect && isSenderCurrent())
		{
			RNDebug("Connected!");
			HandleDidConnect(0);
		}
		for(const DecodedPacket &decoded : result.packets)
		{
			Data *packet = decoded.data;
			if(!result.lostReliableData && isSenderCurrent()) ReceivedPacket(packet, 0, channel);
			packet->Release();
		}
		Release();
	}

	void EOSClient::OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data)
	{
		EOSClient *client = static_cast<EOSClient *>(Data->ClientData);

		if(!client || Data->Reason == EOS_EConnectionClosedReason::EOS_CCR_ClosedByLocalUser) return;
		client->Lock();
		bool isKnownPeer = client->_peers.count(Data->RemoteUserId) > 0;
		client->Unlock();
		if(!isKnownPeer) return;
		client->Retain();
		RNDebug("Disconnected from Server");
		client->ForceDisconnect(static_cast<uint16>(Data->Reason));
		client->Release();
	}
} // namespace RN
