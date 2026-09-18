//
//  RNEOSHost.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEOSHost.h"

#include <RayneConfig.h>

#include "RNEOSWorld.h"

#include "eos_common.h"
#include "eos_p2p.h"
#include "eos_p2p_types.h"
#include "eos_platform_prereqs.h"
#include "eos_sdk.h"

namespace RN
{
	constexpr size_t EOSMaxSendBytesPerPeerPerUpdate = 128 * 1024;
	constexpr size_t EOSMaxScheduledBytesPerPeer = 32 * 1024 * 1024;
	constexpr float EOSMultipartTimeout = 10.0f;
	constexpr size_t EOSMaxMultipartChannelsPerPeer = 4;

	RNDefineMeta(EOSHost, Object)

	EOSHost::EOSHost(RN::String *socketID) :
		_clientID(CLIENT_ID_NONE), _status(Disconnected), _pingTimer(10.0f), _socketID(SafeRetain(socketID))
	{
		RN_ASSERT(_socketID, "Socket id needs to be set and needs to be unique per host if there are multiple!");
		RN_ASSERT(_socketID->GetLength() < EOS_P2P_SOCKETID_SOCKETNAME_SIZE, "Socket ID length needs to be 32 or less!");
	}

	EOSHost::~EOSHost()
	{
		for(auto &peer : _peers)
		{
			ClearPeerData(peer.second);
		}
		SafeRelease(_socketID);
	}

	bool EOSHost::IsUnreliablePacketInOrder(Peer &peer, uint32 packetID, uint8 channel)
	{
		ChannelState &state = peer._channels[channel];
		uint32 forwardDistance = packetID - state.lastReceivedUnreliablePacketID;
		if(!state.hasReceivedUnreliablePacket || (forwardDistance > 0 && forwardDistance < 0x80000000U))
		{
			state.lastReceivedUnreliablePacketID = packetID;
			state.hasReceivedUnreliablePacket = true;
			return true;
		}

		return false;
	}

	void EOSHost::SendPing(EOS_ProductUserId receiverID, bool isResponse, uint32 responseID)
	{
		if(!receiverID) return;
		Lock();
		auto peer = _peers.find(receiverID);
		if(!isResponse && peer == _peers.end())
		{
			Unlock();
			return;
		}

		ProtocolPacketHeader packetHeader {isResponse ? ProtocolPacketTypePingResponse : ProtocolPacketTypePingRequest, responseID, 0};
		if(!isResponse)
		{
			packetHeader.packetID = peer->second._lastPingID++;
			peer->second._sentPingTime = Clock::now();
		}
		Unlock();
		SendRawPacket(receiverID, 255, &packetHeader, sizeof(packetHeader), false);
	}

	void EOSHost::SendPacket(Data *data, EOS_ProductUserId receiverID, uint32 channel, bool reliable)
	{
		if(!data || channel > 255) return;
		size_t packetLength = data->GetLength();
		RN_DEBUG_ASSERT(packetLength < MaximumPacketSize || reliable, "Large EOS packets need to be reliable");
		if(!reliable && packetLength >= MaximumPacketSize) return;
		RN_ASSERT(!reliable || packetLength <= MaximumReassembledPacketSize, "Reliable EOS packet exceeds the maximum reassembled size");

		Lock();
		auto peer = _peers.find(receiverID);
		if(peer == _peers.end())
		{
			Unlock();
			RNDebug("Unknown peer " << receiverID);
			return;
		}
		if(peer->second._wantsDisconnect)
		{
			Unlock();
			return; //Don't allow sending more data to users that are about to be disconnected.
		}
		bool exceedsQueueLimit = packetLength > EOSMaxScheduledBytesPerPeer || peer->second._scheduledPacketBytes > EOSMaxScheduledBytesPerPeer - packetLength;
		if(exceedsQueueLimit)
		{
			EOSClientID clientID = peer->second.clientID;
			Unlock();
			if(reliable)
			{
				RNWarning("Reliable EOS send queue reached its memory limit on channel " << channel);
				HandleReliablePacketLoss(clientID, channel);
			}
			else
			{
				RNWarning("Dropping an unreliable EOS packet because the peer send queue reached its memory limit");
			}
			return;
		}

		if(peer->second._scheduledPackets.find(channel) == peer->second._scheduledPackets.end())
		{
			peer->second._scheduledPackets.insert(std::pair(channel, std::queue<Packet>()));
		}

		peer->second._scheduledPackets[channel].push({reliable, data->Retain()});
		peer->second._scheduledPacketBytes += packetLength;

		Unlock();
	}

	void EOSHost::SendPacket(Data *data, EOSClientID receiverID, uint32 channel, bool reliable)
	{
		if(!data || channel > 255) return;
		Lock();
		auto receiver = _idMap.find(receiverID);
		if(receiver == _idMap.end())
		{
			Unlock();
			RNDebug("Unknown receiver ID " << receiverID);
			return;
		}
		EOS_ProductUserId internalReceiverID = receiver->second;
		Unlock();
		SendPacket(data, internalReceiverID, channel, reliable);
	}

	void EOSHost::BroadcastPacket(Data *data, uint32 channel, bool reliable, EOSClientID excludeClientID)
	{
		if(!data || channel > 255) return;
		std::vector<EOS_ProductUserId> receivers;
		Lock();
		for(const auto &peer : _peers)
		{
			if(excludeClientID != CLIENT_ID_NONE && peer.second.clientID == excludeClientID) continue;
			receivers.push_back(peer.first);
		}
		Unlock();
		for(EOS_ProductUserId receiver : receivers) SendPacket(data, receiver, channel, reliable);
	}

	void EOSHost::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader) || channel != 255) return;
		Lock();
		size_t dataIndex = 0;
		while(dataIndex < bytesWritten)
		{
			ProtocolPacketHeader packetHeader;
			const uint8 *payload = nullptr;
			if(!ReadPacketHeader(rawData, bytesWritten, dataIndex, packetHeader, payload)) break;
			if(packetHeader.dataLength == 0 && packetHeader.packetType == ProtocolPacketTypePingRequest)
			{
				SendPing(senderUserID, true, packetHeader.packetID);
			}
			else if(packetHeader.dataLength == 0 && packetHeader.packetType == ProtocolPacketTypePingResponse)
			{
				auto peer = _peers.find(senderUserID);
				if(peer != _peers.end() && peer->second._lastPingID - 1 == packetHeader.packetID)
				{
					Clock::time_point receivedPingTime = Clock::now();
					auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(receivedPingTime - peer->second._sentPingTime).count();
					double timeElapsed = milliseconds / 1000.0;
					peer->second.smoothedRoundtripTime = peer->second.smoothedRoundtripTime * 0.75 + timeElapsed * 0.25;
				}
				else if(peer != _peers.end()) RNDebug("Missed a ping!");
			}
		}
		Unlock();
	}

	bool EOSHost::ReadPacketHeader(const uint8 *rawData, size_t bytesWritten, size_t &dataIndex, ProtocolPacketHeader &packetHeader, const uint8 *&payload) const
	{
		if(!rawData || dataIndex > bytesWritten || bytesWritten - dataIndex < sizeof(ProtocolPacketHeader)) return false;
		memcpy(&packetHeader, rawData + dataIndex, sizeof(packetHeader));
		dataIndex += sizeof(packetHeader);
		if(packetHeader.dataLength > bytesWritten - dataIndex) return false;
		payload = rawData + dataIndex;
		dataIndex += packetHeader.dataLength;
		return true;
	}

	EOSHost::MultipartReceiveResult EOSHost::ReceiveMultipartPacket(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel)
	{
		MultipartReceiveResult result;
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeaderMultipart)) return result;

		ProtocolPacketHeaderMultipart packetHeader;
		memcpy(&packetHeader, rawData, sizeof(packetHeader));
		constexpr size_t payloadSize = MaximumPacketSize - sizeof(ProtocolPacketHeaderMultipart);
		constexpr size_t maximumPartCount = (MaximumReassembledPacketSize + payloadSize - 1) / payloadSize;
		if(packetHeader.packetType != ProtocolPacketTypeReliableDataMultipart || packetHeader.totalDataParts == 0 ||
			packetHeader.totalDataParts > maximumPartCount || packetHeader.dataPart >= packetHeader.totalDataParts) return result;

		auto assembly = peer._multipartAssemblies.find(channel);
		if(assembly != peer._multipartAssemblies.end() && (assembly->second.currentPart + 1 != packetHeader.dataPart ||
			assembly->second.totalParts != packetHeader.totalDataParts || assembly->second.packetID != packetHeader.packetID))
		{
			ClearMultipartPacket(peer, channel);
			assembly = peer._multipartAssemblies.end();
			result.lostData = true;
		}

		if(assembly == peer._multipartAssemblies.end())
		{
			if(packetHeader.dataPart != 0 || peer._multipartAssemblies.size() >= EOSMaxMultipartChannelsPerPeer)
			{
				result.lostData = true;
				return result;
			}
			assembly = peer._multipartAssemblies.insert(std::pair(channel, MultipartAssembly {packetHeader.packetID, packetHeader.totalDataParts, 0, new Data(), 0.0f})).first;
		}
		else
		{
			assembly->second.currentPart = packetHeader.dataPart;
		}
		assembly->second.age = 0.0f;

		size_t packetPayloadLength = bytesWritten - sizeof(ProtocolPacketHeaderMultipart);
		Data *multipartData = assembly->second.data;
		if(packetPayloadLength > MaximumReassembledPacketSize - multipartData->GetLength())
		{
			ClearMultipartPacket(peer, channel);
			result.lostData = true;
			return result;
		}

		multipartData->Append(rawData + sizeof(ProtocolPacketHeaderMultipart), packetPayloadLength);
		result.progressed = true;
		if(packetHeader.dataPart + 1 == packetHeader.totalDataParts)
		{
			result.data = multipartData->Retain();
			ClearMultipartPacket(peer, channel);
		}
		return result;
	}

	EOSHost::DecodeResult EOSHost::DecodePackets(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel)
	{
		DecodeResult result;
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader)) return result;
		ProtocolPacketType type = static_cast<ProtocolPacketType>(rawData[0]);
		if(type == ProtocolPacketTypeReliableDataMultipart)
		{
			MultipartReceiveResult multipart = ReceiveMultipartPacket(peer, rawData, bytesWritten, channel);
			result.multipartProgress = multipart.progressed;
			result.lostReliableData = multipart.lostData;
			if(multipart.data) result.packets.push_back({ProtocolPacketTypeReliableData, 0, multipart.data});
			return result;
		}
		if(peer._multipartAssemblies.count(channel) != 0)
		{
			if(type == ProtocolPacketTypeData) return result;
			ClearMultipartPacket(peer, channel);
			result.lostReliableData = true;
		}

		size_t dataIndex = 0;
		while(dataIndex < bytesWritten)
		{
			ProtocolPacketHeader header;
			const uint8 *payload = nullptr;
			if(!ReadPacketHeader(rawData, bytesWritten, dataIndex, header, payload)) break;
			bool isData = header.packetType == ProtocolPacketTypeData || header.packetType == ProtocolPacketTypeReliableData;
			bool isControl = header.packetType == ProtocolPacketTypeConnectRequest || header.packetType == ProtocolPacketTypeConnectResponse;
			if(!isData && !isControl) break;
			if(header.packetType == ProtocolPacketTypeData && !IsUnreliablePacketInOrder(peer, header.packetID, static_cast<uint8>(channel))) continue;
			result.packets.push_back({header.packetType, header.packetID, new Data(payload, header.dataLength)});
		}
		return result;
	}

	bool EOSHost::SendRawPacket(EOS_ProductUserId receiverID, uint32 channel, const void *bytes, size_t length, bool reliable)
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world || !world->GetP2PHandle() || !receiverID || !bytes || length == 0 || length > MaximumPacketSize || channel > 255) return false;

		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);

		EOS_P2P_SendPacketOptions options = {};
		options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
		options.Channel = static_cast<uint8>(channel);
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = receiverID;
		options.SocketId = &socketID;
		options.Reliability = reliable ? EOS_EPacketReliability::EOS_PR_ReliableOrdered : EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
		options.bAllowDelayedDelivery = reliable;
		options.Data = bytes;
		options.DataLengthBytes = static_cast<uint32>(length);
		return EOS_P2P_SendPacket(world->GetP2PHandle(), &options) == EOS_EResult::EOS_Success;
	}

	bool EOSHost::SendMultipartPacket(Peer &peer, uint32 channel, std::queue<Packet> &scheduledPackets, size_t &remainingSendBytes)
	{
		Packet &packet = scheduledPackets.front();
		RN_ASSERT(packet.isReliable, "Multipart EOS packets need to be reliable");
		RN_ASSERT(packet.data->GetLength() <= MaximumReassembledPacketSize, "Reliable EOS packet exceeds the maximum reassembled size");

		constexpr size_t payloadSize = MaximumPacketSize - sizeof(ProtocolPacketHeaderMultipart);
		size_t totalPartCount = (packet.data->GetLength() + payloadSize - 1) / payloadSize;
		if(!packet.isMultipartStarted)
		{
			packet.multipartPacketID = peer._channels[channel].nextPacketID++;
			packet.multipartNextPart = 0;
			packet.isMultipartStarted = true;
		}

		while(packet.multipartNextPart < totalPartCount)
		{
			size_t dataOffset = static_cast<size_t>(packet.multipartNextPart) * payloadSize;
			size_t dataLength = std::min(payloadSize, packet.data->GetLength() - dataOffset);
			size_t encodedLength = sizeof(ProtocolPacketHeaderMultipart) + dataLength;
			if(encodedLength > remainingSendBytes) return false;

			Data *encodedPacket = new Data();
			ProtocolPacketHeaderMultipart header;
			header.packetType = ProtocolPacketTypeReliableDataMultipart;
			header.packetID = packet.multipartPacketID;
			header.dataPart = packet.multipartNextPart;
			header.totalDataParts = static_cast<uint16>(totalPartCount);
			encodedPacket->Append(&header, sizeof(header));
			encodedPacket->Append(packet.data->GetDataInRange(Range(dataOffset, dataLength)));
			bool didSend = SendRawPacket(peer.internalID, channel, encodedPacket->GetBytes(), encodedPacket->GetLength(), true);
			encodedPacket->Release();
			if(!didSend) return false;
			remainingSendBytes -= encodedLength;
			packet.multipartNextPart += 1;
		}

		peer._scheduledPacketBytes -= packet.data->GetLength();
		packet.data->Release();
		scheduledPackets.pop();
		return true;
	}

	bool EOSHost::SendPacketBatch(Peer &peer, uint32 channel, std::queue<Packet> &scheduledPackets, size_t &remainingSendBytes)
	{
		bool reliable = scheduledPackets.front().isReliable;
		Data *encodedPacket = new Data();
		std::vector<Packet> packets;
		while(!scheduledPackets.empty())
		{
			Packet &packet = scheduledPackets.front();
			size_t encodedLength = sizeof(ProtocolPacketHeader) + packet.data->GetLength();
			if(packet.isReliable != reliable || encodedLength >= MaximumPacketSize ||
				encodedPacket->GetLength() + encodedLength >= MaximumPacketSize || encodedPacket->GetLength() + encodedLength > remainingSendBytes) break;

			ProtocolPacketHeader header;
			header.packetType = reliable ? ProtocolPacketTypeReliableData : ProtocolPacketTypeData;
			header.packetID = peer._channels[channel].nextPacketID++;
			header.dataLength = static_cast<uint16>(packet.data->GetLength());
			encodedPacket->Append(&header, sizeof(header));
			encodedPacket->Append(packet.data);
			packets.push_back(packet);
			scheduledPackets.pop();
		}

		if(packets.empty())
		{
			encodedPacket->Release();
			return false;
		}

		bool didSend = SendRawPacket(peer.internalID, channel, encodedPacket->GetBytes(), encodedPacket->GetLength(), reliable);
		if(!didSend && reliable)
		{
			std::queue<Packet> restoredPackets;
			for(Packet &packet : packets) restoredPackets.push(packet);
			while(!scheduledPackets.empty())
			{
				restoredPackets.push(scheduledPackets.front());
				scheduledPackets.pop();
			}
			scheduledPackets.swap(restoredPackets);
		}
		else
		{
			if(didSend) remainingSendBytes -= encodedPacket->GetLength();
			for(Packet &packet : packets)
			{
				peer._scheduledPacketBytes -= packet.data->GetLength();
				packet.data->Release();
			}
		}
		encodedPacket->Release();
		return didSend || !reliable;
	}

	void EOSHost::Update(float delta)
	{
		std::vector<EOS_ProductUserId> pingTargets;
		std::vector<std::pair<EOSClientID, uint32>> peersWithExpiredMultipartPackets;
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world || !world->GetP2PHandle())
		{
			Unlock();
			return;
		}

		_pingTimer += std::max(delta, 0.0f);
		if(_pingTimer > 5.0f)
		{
			_pingTimer = 0.0f;
			for(auto &pair : _peers)
			{
				if(pair.second._isConnectionActive) pingTargets.push_back(pair.first);
			}
		}

		for(auto &peerPair : _peers)
		{
			Peer &peer = peerPair.second;
			std::vector<uint32> expiredChannels;
			for(auto &assembly : peer._multipartAssemblies)
			{
				assembly.second.age += std::max(delta, 0.0f);
				if(assembly.second.age >= EOSMultipartTimeout)
				{
					RNInfo("Reliable EOS multipart receive timed out: peer=" << peer.clientID << " channel=" << assembly.first <<
						" packetID=" << assembly.second.packetID << " receivedParts=" << (assembly.second.currentPart + 1) <<
						" expectedParts=" << assembly.second.totalParts << " bufferedBytes=" << assembly.second.data->GetLength() <<
						" idleSeconds=" << assembly.second.age);
					expiredChannels.push_back(assembly.first);
				}
			}
			for(uint32 channel : expiredChannels)
			{
				ClearMultipartPacket(peer, channel);
				if(peer.clientID != CLIENT_ID_NONE) peersWithExpiredMultipartPackets.emplace_back(peer.clientID, channel);
			}

			size_t remainingSendBytes = EOSMaxSendBytesPerPeerPerUpdate;
			for(auto channel = peer._scheduledPackets.begin(); channel != peer._scheduledPackets.end();)
			{
				std::queue<Packet> &scheduledPackets = channel->second;
				while(!scheduledPackets.empty() && remainingSendBytes >= sizeof(ProtocolPacketHeader))
				{
					bool isMultipart = scheduledPackets.front().data->GetLength() + sizeof(ProtocolPacketHeader) >= MaximumPacketSize;
					bool didAdvance = isMultipart ? SendMultipartPacket(peer, channel->first, scheduledPackets, remainingSendBytes) : SendPacketBatch(peer, channel->first, scheduledPackets, remainingSendBytes);
					if(!didAdvance) break;
				}
				if(scheduledPackets.empty()) channel = peer._scheduledPackets.erase(channel);
				else ++channel;
				if(remainingSendBytes < sizeof(ProtocolPacketHeader)) break;
			}
		}
		Unlock();

		for(EOS_ProductUserId peerID : pingTargets) SendPing(peerID, false, 0);
		for(const auto &[clientID, channel] : peersWithExpiredMultipartPackets) HandleReliablePacketLoss(clientID, channel);
	}

	EOSHost::Peer EOSHost::CreatePeer(EOSClientID clientID, EOS_ProductUserId internalID)
	{
		Peer peer;
		peer.clientID = clientID;
		peer.internalID = internalID;

		return peer;
	}

	void EOSHost::ClearMultipartPacket(Peer &peer, uint32 channel)
	{
		auto assembly = peer._multipartAssemblies.find(channel);
		if(assembly == peer._multipartAssemblies.end()) return;
		SafeRelease(assembly->second.data);
		peer._multipartAssemblies.erase(assembly);
	}

	void EOSHost::ClearPeerData(Peer &peer)
	{
		while(!peer._multipartAssemblies.empty())
		{
			ClearMultipartPacket(peer, peer._multipartAssemblies.begin()->first);
		}
		ClearScheduledPackets(peer);
	}

	void EOSHost::ClearScheduledPackets(Peer &peer)
	{
		for(auto &channel : peer._scheduledPackets)
		{
			while(!channel.second.empty())
			{
				SafeRelease(channel.second.front().data);
				channel.second.pop();
			}
		}
		peer._scheduledPackets.clear();
		peer._scheduledPacketBytes = 0;
	}

	void EOSHost::ClearScheduledPackets(Peer &peer, uint32 channel)
	{
		auto scheduled = peer._scheduledPackets.find(channel);
		if(scheduled == peer._scheduledPackets.end()) return;
		while(!scheduled->second.empty())
		{
			Data *data = scheduled->second.front().data;
			peer._scheduledPacketBytes -= data->GetLength();
			data->Release();
			scheduled->second.pop();
		}
		peer._scheduledPackets.erase(scheduled);
	}

	void EOSHost::HandleReliablePacketLoss(EOSClientID clientID, uint32)
	{
		// Preserve the existing failure policy for clients that don't distinguish channels.
		HandleReliablePacketLoss(clientID);
	}

	void EOSHost::HandleReliablePacketLoss(EOSClientID clientID)
	{
		Lock();
		auto mappedPeer = _idMap.find(clientID);
		EOS_ProductUserId productUserID = mappedPeer != _idMap.end() ? mappedPeer->second : nullptr;
		Unlock();

		RNWarning("The reliable EOS stream with peer " << clientID << " became incomplete; disconnecting");
		if(productUserID)
		{
			DisconnectClient(productUserID);
		}
		else
		{
			Disconnect();
		}
	}

	EOSClientID EOSHost::GetUserIDForInternalID(EOS_ProductUserId internalID)
	{
		auto peer = _peers.find(internalID);
		return peer != _peers.end() ? peer->second.clientID : CLIENT_ID_NONE;
	}

	EOSClientID EOSHost::GetClientIDForProductUserID(EOS_ProductUserId productUserID) const
	{
		char productUserIDString[EOS_PRODUCTUSERID_MAX_LENGTH + 1] = {};
		int32_t length = sizeof(productUserIDString);
		EOS_EResult result = EOS_ProductUserId_ToString(productUserID, productUserIDString, &length);
		size_t stringLength = strlen(productUserIDString);
		RN_ASSERT(result == EOS_EResult::EOS_Success && stringLength >= 16, "Failed to derive a network ID from an EOS product user ID.");
		if(result != EOS_EResult::EOS_Success || stringLength < 16) return CLIENT_ID_NONE;

		EOSClientID clientID = 0;
		for(const char *character = productUserIDString + stringLength - 16; *character; character += 1)
		{
			uint8 digit = *character >= '0' && *character <= '9' ? *character - '0' :
				(*character >= 'a' && *character <= 'f' ? *character - 'a' + 10 : *character - 'A' + 10);
			RN_ASSERT(digit < 16, "EOS product user ID contains a non-hexadecimal character.");
			clientID <<= 4;
			clientID |= digit;
		}
		RN_ASSERT(clientID < CLIENT_ID_RESERVED, "EOS-derived network ID conflicts with a reserved sentinel.");
		if(clientID >= CLIENT_ID_RESERVED) return CLIENT_ID_NONE;
		return clientID;
	}

	double EOSHost::GetLastRoundtripTime(EOSClientID peerID)
	{
		auto mappedPeer = _idMap.find(peerID);
		if(mappedPeer == _idMap.end()) return 0.0;
		auto peer = _peers.find(mappedPeer->second);
		return peer != _peers.end() ? peer->second.smoothedRoundtripTime : 0.0;
	}

	bool EOSHost::HasClient(EOSClientID clientID)
	{
		auto peerID = _idMap.find(clientID);
		return peerID != _idMap.end() && _peers.find(peerID->second) != _peers.end();
	}

	bool EOSHost::HasClient(const RN::String *eosUserIDString)
	{
		if(!eosUserIDString) return false;

		EOS_ProductUserId eosID = EOSWorld::GetInstance()->GetUserIDFromString(eosUserIDString);
		if(_peers.find(eosID) == _peers.end()) return false;
		return true;
	}

	bool EOSHost::IsClientConnectionActive(EOSClientID clientID)
	{
		Lock();
		auto mappedPeer = _idMap.find(clientID);
		if(mappedPeer == _idMap.end())
		{
			Unlock();
			return false;
		}
		auto peer = _peers.find(mappedPeer->second);
		bool isActive = peer != _peers.end() && peer->second._isConnectionActive;
		Unlock();
		return isActive;
	}

	const RN::String *EOSHost::GetEOSUserIDStringForClient(EOSClientID clientID) const
	{
		if(clientID == _clientID)
		{
			return EOSWorld::GetInstance()->GetUserIDString();
		}

		auto it = _idMap.find(clientID);
		if(it == _idMap.end()) return nullptr;
		return EOSWorld::GetInstance()->GetUserIDString(it->second);
	}
} // namespace RN
