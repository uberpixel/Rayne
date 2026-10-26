//
//  RNEOSHost.cpp
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Include zlib before RNBase.h removes the Windows near/far macros.
#include <zlib.h>

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
	constexpr size_t EOSReliableWindow = 256 * 1024;
	constexpr size_t EOSMaxSendBytesPerPeerPerUpdate = 128 * 1024;
	constexpr size_t EOSSendChannelShare = 16 * 1024;
	constexpr size_t EOSBackgroundReliableWindow = 64 * 1024;
	constexpr size_t EOSMeshScheduledBytesPerPeer = 1024 * 1024;
	constexpr double EOSMeshInterruptionTimeout = 5.0;
	constexpr float EOSUnreliablePacketLifetime = 0.25f;
	constexpr double EOSPeerProgressTimeout = 60.0;
	constexpr size_t EOSMaxScheduledBytesPerPeer = 32 * 1024 * 1024;
	constexpr size_t EOSMaxMultipartChannelsPerPeer = 4;

	RNDefineMeta(EOSHost, Object)

	EOSHost::EOSHost(RN::String *socketID) :
		_clientID(CLIENT_ID_NONE), _status(Disconnected), _pingTimer(10.0f), _socketID(SafeRetain(socketID))
	{
		RN_ASSERT(_socketID, "Socket id needs to be set and needs to be unique per host if there are multiple!");
		RN_ASSERT(_socketID->GetLength() < EOS_P2P_SOCKETID_SOCKETNAME_SIZE, "Socket ID length needs to be 32 or less!");
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_SocketId sdkSocketID = {};
		sdkSocketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(sdkSocketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);
		EOS_P2P_AddNotifyPeerConnectionInterruptedOptions interruptedOptions = {};
		interruptedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
		interruptedOptions.LocalUserId = world->GetUserID();
		interruptedOptions.SocketId = &sdkSocketID;
		_connectionInterruptedNotificationID = EOS_P2P_AddNotifyPeerConnectionInterrupted(world->GetP2PHandle(), &interruptedOptions, this, OnConnectionInterruptedCallback);

		EOS_P2P_AddNotifyPeerConnectionEstablishedOptions establishedOptions = {};
		establishedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
		establishedOptions.LocalUserId = world->GetUserID();
		establishedOptions.SocketId = &sdkSocketID;
		_connectionEstablishedNotificationID = EOS_P2P_AddNotifyPeerConnectionEstablished(world->GetP2PHandle(), &establishedOptions, this, OnConnectionEstablishedCallback);
	}

	EOSHost::~EOSHost()
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(world && world->GetP2PHandle())
		{
			if(_connectionInterruptedNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionInterrupted(world->GetP2PHandle(), _connectionInterruptedNotificationID);
			if(_connectionEstablishedNotificationID != EOS_INVALID_NOTIFICATIONID) EOS_P2P_RemoveNotifyPeerConnectionEstablished(world->GetP2PHandle(), _connectionEstablishedNotificationID);
		}

		for(auto &peer : _peers)
		{
			ClearPeerData(peer.second);
		}
		SafeRelease(_socketID);
	}

	void EOSHost::SetPeerBackgroundSpareTime(EOSClientID clientID, uint32 seconds)
	{
		Lock();
		auto mapped = _idMap.find(clientID);
		auto peer = mapped == _idMap.end() ? _peers.end() : _peers.find(mapped->second);
		if(peer != _peers.end()) peer->second._backgroundSpareUntil = Clock::now() + std::chrono::seconds(seconds);
		Unlock();
	}

	bool EOSHost::HasPeerBackgroundSpareTime(EOSClientID clientID)
	{
		Lock();
		auto mapped = _idMap.find(clientID);
		auto peer = mapped == _idMap.end() ? _peers.end() : _peers.find(mapped->second);
		bool eligible = peer != _peers.end() && Clock::now() < peer->second._backgroundSpareUntil;
		Unlock();
		return eligible;
	}

	void EOSHost::ClosePeerTransport(EOS_ProductUserId productUserID)
	{
		EOSWorld *world = EOSWorld::GetInstance();
		EOS_P2P_SocketId socketID = {};
		socketID.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
		strncpy(socketID.SocketName, _socketID->GetUTF8String(), EOS_P2P_SOCKETID_SOCKETNAME_SIZE);
		EOS_P2P_CloseConnectionOptions options = {};
		options.ApiVersion = EOS_P2P_CLOSECONNECTION_API_LATEST;
		options.LocalUserId = world->GetUserID();
		options.RemoteUserId = productUserID;
		options.SocketId = &socketID;
		EOS_P2P_CloseConnection(world->GetP2PHandle(), &options);
	}

	size_t EOSHost::GetRemainingReliableWindow(const Peer &peer, uint32 channel, bool allowBackgroundSpare) const
	{
		size_t remaining = EOSReliableWindow - std::min<uint64>(EOSReliableWindow, peer._outstandingReliableBytes);
		if(channel == _backgroundChannel)
		{
			const ChannelState &state = peer._channels[channel];
			uint64 outstanding = state.sentReliableBytes - std::min(state.sentReliableBytes, state.acknowledgedReliableBytes);
			// Only the shared spare-capacity pass may expand initial catchup's window.
			bool spare = allowBackgroundSpare && Clock::now() < peer._backgroundSpareUntil;
			size_t window = spare ? 2 * EOSBackgroundReliableWindow : EOSBackgroundReliableWindow;
			remaining = std::min<size_t>(remaining, window - std::min<uint64>(window, outstanding));
		}
		return remaining;
	}

	bool EOSHost::SendScheduledPacket(Peer &peer, uint32 channel, const void *bytes, size_t length, bool reliable, bool allowBackgroundSpare)
	{
		if(peer._isConnectionInterrupted || (peer._requiresBoundStream && !peer._didNotifyConnection)) return false;

		size_t wireLength = length + sizeof(ProtocolPacketEnvelope);
		if(reliable && wireLength > GetRemainingReliableWindow(peer, channel, allowBackgroundSpare)) return false;
		ProtocolPacketEnvelope envelope {reliable ? ProtocolPacketTypeReliableEnvelope : ProtocolPacketTypeUnreliableEnvelope,
			peer._reliableSendStream, reliable ? peer._channels[channel].sentReliableBytes + wireLength : 0};
		uint8 encoded[MaximumPacketSize];
		if(wireLength > sizeof(encoded)) return false;
		memcpy(encoded, &envelope, sizeof(envelope));
		memcpy(encoded + sizeof(envelope), bytes, length);
		if(!SendRawPacket(peer.internalID, channel, encoded, wireLength, reliable, reliable)) return false;
		if(reliable)
		{
			ChannelState &state = peer._channels[channel];
			if(state.sentReliableBytes == state.acknowledgedReliableBytes)
			{
				state.lastReliableProgressTime = _peerProgressTime;
			}
			state.sentReliableBytes = envelope.sentBytes;
			peer._outstandingReliableBytes += wireLength;
		}
		return true;
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

	Data *EOSHost::PreparePacket(Data *data, bool &isCompressed)
	{
		// Payload high bit: four-byte little-endian original length followed by one zlib stream.
		// Try level 1 from 256 bytes, keeping it only when the full payload saves 16 bytes.
		isCompressed = false;
		const size_t length = data->GetLength();
		if(length < 256 || length > MaximumReassembledPacketSize) return data->Retain();

		struct Compressor
		{
			z_stream stream = {};
			bool initialized;

			Compressor() :
				initialized(deflateInit(&stream, Z_BEST_SPEED) == Z_OK)
			{}

			~Compressor()
			{
				if(initialized) deflateEnd(&stream);
			}
		};
		thread_local Compressor compressor;
		if(!compressor.initialized || deflateReset(&compressor.stream) != Z_OK) return data->Retain();

		uLongf compressedLength = compressBound(static_cast<uLong>(length));
		Data *buffer = new Data(compressedLength + 4);
		uint8 *bytes = static_cast<uint8 *>(buffer->GetBytes());
		for(size_t index = 0; index < 4; index++)
		{
			bytes[index] = static_cast<uint8>(length >> (index * 8));
		}

		z_stream &stream = compressor.stream;
		stream.next_in = static_cast<Bytef *>(data->GetBytes());
		stream.avail_in = static_cast<uInt>(length);
		stream.next_out = bytes + 4;
		stream.avail_out = static_cast<uInt>(compressedLength);
		int status = deflate(&stream, Z_FINISH);
		compressedLength = stream.total_out;
		Data *prepared = data;
		if(status == Z_STREAM_END && compressedLength + 4 + 16 <= length)
		{
			prepared = new Data(bytes, compressedLength + 4);
			isCompressed = true;
		}
		else prepared->Retain();
		buffer->Release();
		return prepared;
	}

	Data *EOSHost::InflatePacket(Data *data, size_t &remainingBytes)
	{
		if(data->GetLength() < 4) return nullptr;
		const uint8 *bytes = static_cast<const uint8 *>(data->GetBytes());
		size_t length = 0;
		for(size_t index = 0; index < 4; index++)
		{
			length |= static_cast<size_t>(bytes[index]) << (index * 8);
		}

		// Charge the declared expansion even if the compressed payload is invalid.
		bool fits = length <= remainingBytes;
		remainingBytes -= std::min(remainingBytes, length);
		if(!fits || data->GetLength() < 5 || data->GetLength() > MaximumReassembledPacketSize ||
			length < 256 || length > MaximumReassembledPacketSize || data->GetLength() + 16 > length)
		{
			return nullptr;
		}

		z_stream stream = {};
		stream.next_in = const_cast<Bytef *>(bytes + 4);
		stream.avail_in = static_cast<uInt>(data->GetLength() - 4);
		if(inflateInit(&stream) != Z_OK) return nullptr;
		Data *inflated = new Data(length);
		stream.next_out = static_cast<Bytef *>(inflated->GetBytes());
		stream.avail_out = static_cast<uInt>(length);
		int status = inflate(&stream, Z_FINISH);
		bool valid = status == Z_STREAM_END && stream.total_out == length && stream.avail_in == 0;
		inflateEnd(&stream);
		if(valid) return inflated;
		inflated->Release();
		return nullptr;
	}

	void EOSHost::InflateDecodedPackets(DecodeResult &result)
	{
		size_t remainingBytes = MaximumReassembledPacketSize;
		for(auto packet = result.packets.begin(); packet != result.packets.end();)
		{
			if(!packet->isCompressed && packet->data->GetLength() <= remainingBytes)
			{
				remainingBytes -= packet->data->GetLength();
				++packet;
				continue;
			}
			if(!packet->isCompressed) remainingBytes = 0;

			Data *inflated = packet->isCompressed ? InflatePacket(packet->data, remainingBytes) : nullptr;
			packet->data->Release();
			if(!inflated)
			{
				result.lostReliableData |= result.isReliable;
				packet = result.packets.erase(packet);
				continue;
			}
			packet->data = inflated;
			packet->isCompressed = false;
			++packet;
		}
	}

	EOSHost::SendResult EOSHost::QueuePacket(Data *data, EOS_ProductUserId receiverID, uint32 channel, bool reliable, bool requireActive,
		size_t maximumQueuedBytes, size_t *queuedBytes)
	{
		if(!data || IsReservedChannel(channel)) return SendResult::InvalidPacket;
		if(data->GetLength() > MaximumReassembledPacketSize) return SendResult::InvalidPacket;

		bool isCompressed;
		Data *prepared = PreparePacket(data, isCompressed);
		SendResult result = QueuePreparedPacket(prepared, isCompressed, receiverID, channel, reliable, requireActive,
			maximumQueuedBytes, queuedBytes);
		prepared->Release();
		return result;
	}

	EOSHost::SendResult EOSHost::QueuePreparedPacket(Data *prepared, bool isCompressed, EOS_ProductUserId receiverID, uint32 channel, bool reliable, bool requireActive,
		size_t maximumQueuedBytes, size_t *queuedBytes)
	{
		const size_t packetLength = prepared->GetLength();
		if(!reliable && packetLength + sizeof(ProtocolPayloadHeader) + sizeof(ProtocolPacketEnvelope) > MaximumPacketSize) return SendResult::InvalidPacket;
		if(packetLength > maximumQueuedBytes) return SendResult::QueueFull;

		Lock();
		auto peer = _peers.find(receiverID);
		if(peer == _peers.end())
		{
			Unlock();
			RNDebug("Unknown peer " << receiverID);
			return SendResult::PeerUnavailable;
		}
		if(peer->second._wantsDisconnect || (requireActive && (!peer->second._didNotifyConnection || peer->second._isConnectionInterrupted)))
		{
			Unlock();
			return SendResult::PeerUnavailable; //Don't allow sending more data to users that are about to be disconnected.
		}
		// Leave half the mesh queue available for reliable traffic.
		size_t queueLimit = _limitAggregateSending ? EOSMeshScheduledBytesPerPeer / (reliable ? 1 : 2) : EOSMaxScheduledBytesPerPeer;
		if(channel == _backgroundChannel)
		{
			// One logical repair packet preserves queue space for foreground traffic.
			auto queued = peer->second._scheduledPackets.find(channel);
			if(queued != peer->second._scheduledPackets.end() && !queued->second.empty())
			{
				Unlock();
				return SendResult::QueueFull;
			}
			queueLimit /= 2;
		}
		size_t queuedPacketCount = 0;
		for(const auto &entry : peer->second._scheduledPackets) queuedPacketCount += entry.second.size();
		bool exceedsQueueLimit = queuedPacketCount >= 8192 || packetLength > queueLimit || peer->second._scheduledPacketBytes > queueLimit - packetLength;
		if(exceedsQueueLimit)
		{
			Unlock();
			return SendResult::QueueFull;
		}

		Packet queued;
		queued.isReliable = reliable;
		queued.data = prepared->Retain();
		queued.isCompressed = isCompressed;
		queued.queuedAt = Clock::now();
		peer->second._scheduledPackets[channel].push_back(queued);
		peer->second._scheduledPacketBytes += packetLength;

		Unlock();
		if(queuedBytes) *queuedBytes = packetLength;
		return SendResult::Queued;
	}

	void EOSHost::SendPacket(Data *data, EOS_ProductUserId receiverID, uint32 channel, bool reliable)
	{
		if(QueuePacket(data, receiverID, channel, reliable, false) == SendResult::QueueFull && reliable)
		{
			Lock();
			auto peer = _peers.find(receiverID);
			EOSClientID clientID = peer != _peers.end() ? peer->second.clientID : CLIENT_ID_NONE;
			Unlock();
			if(clientID != CLIENT_ID_NONE) HandleReliablePacketLoss(clientID, channel);
		}
	}

	EOSHost::SendResult EOSHost::TrySendPacket(Data *data, EOSClientID receiverID, uint32 channel, bool reliable,
		size_t maximumQueuedBytes, size_t *queuedBytes)
	{
		if(queuedBytes) *queuedBytes = 0;
		Lock();
		auto receiver = _idMap.find(receiverID);
		EOS_ProductUserId internalID = receiver != _idMap.end() ? receiver->second : nullptr;
		Unlock();
		return internalID ? QueuePacket(data, internalID, channel, reliable, true, maximumQueuedBytes, queuedBytes) : SendResult::PeerUnavailable;
	}

	void EOSHost::LobbyMemberJoined(EOS_ProductUserId productUserID)
	{
		Retain();
		HandleLobbyMemberJoined(GetClientIDForProductUserID(productUserID));
		Release();
	}

	void EOSHost::LobbyMemberLeft(EOS_ProductUserId productUserID)
	{
		Retain();
		EOSClientID clientID = GetClientIDForProductUserID(productUserID);
		DisconnectClient(productUserID);
		Lock();
		bool departed = _status != Disconnected && _status != Disconnecting && !_peers.count(productUserID);
		Unlock();
		if(departed && clientID != CLIENT_ID_NONE) HandleLobbyMemberLeft(clientID);
		Release();
	}

	void EOSHost::SendPacket(Data *data, EOSClientID receiverID, uint32 channel, bool reliable)
	{
		if(!data || IsReservedChannel(channel)) return;
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
		if(!data || IsReservedChannel(channel) || data->GetLength() > MaximumReassembledPacketSize) return;
		std::vector<std::pair<EOS_ProductUserId, EOSClientID>> receivers;
		Lock();
		for(const auto &peer : _peers)
		{
			if(excludeClientID != CLIENT_ID_NONE && peer.second.clientID == excludeClientID) continue;
			receivers.emplace_back(peer.first, peer.second.clientID);
		}
		Unlock();
		if(receivers.empty()) return;
		bool isCompressed;
		Data *prepared = PreparePacket(data, isCompressed);
		for(const auto &receiver : receivers)
		{
			SendResult result = QueuePreparedPacket(prepared, isCompressed, receiver.first, channel, reliable, false);
			if(result == SendResult::QueueFull && reliable && receiver.second != CLIENT_ID_NONE)
			{
				HandleReliablePacketLoss(receiver.second, channel);
			}
		}
		prepared->Release();
	}

	size_t EOSHost::TryBroadcastPacket(Data *data, const std::vector<EOSClientID> &receivers, uint32 channel, bool reliable)
	{
		if(!data || IsReservedChannel(channel) || data->GetLength() > MaximumReassembledPacketSize || receivers.empty()) return 0;
		bool isCompressed;
		Data *prepared = PreparePacket(data, isCompressed);
		size_t accepted = 0;
		for(EOSClientID receiver : receivers)
		{
			Lock();
			auto found = _idMap.find(receiver);
			EOS_ProductUserId internalID = found != _idMap.end() ? found->second : nullptr;
			Unlock();
			if(internalID && QueuePreparedPacket(prepared, isCompressed, internalID, channel, reliable, true) == SendResult::Queued)
			{
				accepted++;
			}
		}
		prepared->Release();
		return accepted;
	}

	void EOSHost::ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel)
	{
		if(channel == 254)
		{
			if(!rawData) return;
			if(bytesWritten != sizeof(ReliableAcknowledgement)) return;
			ReliableAcknowledgement ack;
			memcpy(&ack, rawData, sizeof(ack));
			Lock();
			auto found = _peers.find(senderUserID);
			if(found != _peers.end() && ack.channel < 254 && ack.streamID == found->second._reliableSendStream)
			{
				Peer &peer = found->second;
				ChannelState &state = peer._channels[ack.channel];
				if(ack.receivedBytes > state.acknowledgedReliableBytes && ack.receivedBytes <= state.sentReliableBytes)
				{
					peer._outstandingReliableBytes -= ack.receivedBytes - state.acknowledgedReliableBytes;
					state.acknowledgedReliableBytes = ack.receivedBytes;
					state.lastReliableProgressTime = _peerProgressTime;
				}
			}
			Unlock();
			return;
		}
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
		if(!rawData || bytesWritten < sizeof(ProtocolPayloadHeaderMultipart)) return result;

		ProtocolPayloadHeaderMultipart packetHeader;
		memcpy(&packetHeader, rawData, sizeof(packetHeader));
		constexpr size_t payloadSize = MaximumPacketSize - sizeof(ProtocolPayloadHeaderMultipart) - sizeof(ProtocolPacketEnvelope);
		constexpr size_t maximumPartCount = (MaximumReassembledPacketSize + payloadSize - 1) / payloadSize;
		if((packetHeader.payloadType & ~ProtocolPayloadTypeCompressed) != ProtocolPayloadTypeMultipart || packetHeader.totalDataParts == 0 ||
			packetHeader.totalDataParts > maximumPartCount || packetHeader.dataPart >= packetHeader.totalDataParts)
		{
			return result;
		}

		auto assembly = peer._multipartAssemblies.find(channel);
		if(assembly != peer._multipartAssemblies.end() && (assembly->second.currentPart + 1 != packetHeader.dataPart ||
			assembly->second.totalParts != packetHeader.totalDataParts || assembly->second.packetID != packetHeader.packetID ||
			assembly->second.isCompressed != bool(packetHeader.payloadType & ProtocolPayloadTypeCompressed)))
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
			assembly = peer._multipartAssemblies.insert(std::pair(channel, MultipartAssembly {
				packetHeader.packetID, packetHeader.totalDataParts, 0,
				bool(packetHeader.payloadType & ProtocolPayloadTypeCompressed), new Data()})).first;
		}
		else
		{
			assembly->second.currentPart = packetHeader.dataPart;
		}

		size_t packetPayloadLength = bytesWritten - sizeof(ProtocolPayloadHeaderMultipart);
		Data *multipartData = assembly->second.data;
		if(packetPayloadLength > MaximumReassembledPacketSize - multipartData->GetLength())
		{
			ClearMultipartPacket(peer, channel);
			result.lostData = true;
			return result;
		}

		multipartData->Append(rawData + sizeof(ProtocolPayloadHeaderMultipart), packetPayloadLength);
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
		if(!rawData || bytesWritten < sizeof(ProtocolPacketHeader) || channel > 255) return result;
		ProtocolPacketType type = static_cast<ProtocolPacketType>(rawData[0]);
		if(type == ProtocolPacketTypeConnectRequest || type == ProtocolPacketTypeConnectResponse)
		{
			if(peer._requiresBoundStream || bytesWritten != sizeof(ProtocolPacketHeader)) return result;
			ProtocolPacketHeader header;
			memcpy(&header, rawData, sizeof(header));
			if(header.packetID != 0 || header.dataLength != 0) return result;
			result.connectRequest = type == ProtocolPacketTypeConnectRequest;
			result.connectResponse = type == ProtocolPacketTypeConnectResponse;
			return result;
		}
		if(type != ProtocolPacketTypeReliableEnvelope && type != ProtocolPacketTypeUnreliableEnvelope) return result;
		if(bytesWritten < sizeof(ProtocolPacketEnvelope) + sizeof(ProtocolPayloadHeader) || bytesWritten > MaximumPacketSize) return result;
		ProtocolPacketEnvelope envelope;
		memcpy(&envelope, rawData, sizeof(envelope));
		if(peer._requiresBoundStream && (!peer._remoteStream || envelope.streamID != peer._remoteStream)) return result;
		const bool reliable = type == ProtocolPacketTypeReliableEnvelope;
		result.isReliable = reliable;
		if(reliable)
		{
			if(envelope.streamID == 0 || envelope.sentBytes < bytesWritten) return result;
			ChannelState &state = peer._channels[channel];
			// EOS orders reliable datagrams independently on each channel. ACK delivery, not application validation.
			if(state.receivedReliableStream != envelope.streamID)
			{
				ClearMultipartPacket(peer, channel);
				state.receivedReliableStream = envelope.streamID;
				state.receivedReliableBytes = 0;
				state.lastAcknowledgedBytes = 0;
			}
			if(envelope.sentBytes <= state.receivedReliableBytes) return result;
			if(envelope.sentBytes - bytesWritten > state.receivedReliableBytes)
			{
				result.lostReliableData = true;
				ClearMultipartPacket(peer, channel);
			}
			state.receivedReliableBytes = envelope.sentBytes;
		}
		rawData += sizeof(envelope);
		bytesWritten -= sizeof(envelope);
		ProtocolPayloadType payloadType = static_cast<ProtocolPayloadType>(rawData[0] & ~ProtocolPayloadTypeCompressed);
		if(payloadType == ProtocolPayloadTypeMultipart)
		{
			if(!reliable) return result;
			MultipartReceiveResult multipart = ReceiveMultipartPacket(peer, rawData, bytesWritten, channel);
			result.multipartProgress = multipart.progressed;
			result.lostReliableData = result.lostReliableData || multipart.lostData;
			if(multipart.data) result.packets.push_back({multipart.data, bool(rawData[0] & ProtocolPayloadTypeCompressed)});
			return result;
		}
		if(peer._multipartAssemblies.count(channel) != 0)
		{
			if(!reliable && payloadType == ProtocolPayloadTypeData) return result;
			ClearMultipartPacket(peer, channel);
			result.lostReliableData = true;
		}

		size_t dataIndex = 0;
		while(dataIndex < bytesWritten)
		{
			if(bytesWritten - dataIndex < sizeof(ProtocolPayloadHeader)) break;
			ProtocolPayloadHeader header;
			memcpy(&header, rawData + dataIndex, sizeof(header));
			dataIndex += sizeof(header);
			if((header.payloadType & ~ProtocolPayloadTypeCompressed) != ProtocolPayloadTypeData || header.dataLength > bytesWritten - dataIndex) break;
			const uint8 *payload = rawData + dataIndex;
			dataIndex += header.dataLength;
			if(!reliable && !IsUnreliablePacketInOrder(peer, header.packetID, static_cast<uint8>(channel))) continue;
			result.packets.push_back({new Data(payload, header.dataLength), bool(header.payloadType & ProtocolPayloadTypeCompressed)});
		}
		return result;
	}

	bool EOSHost::SendRawPacket(EOS_ProductUserId receiverID, uint32 channel, const void *bytes, size_t length, bool reliable, bool throttleReliable)
	{
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world || !world->GetP2PHandle() || !receiverID || !bytes || length == 0 || length > MaximumPacketSize || channel > 255)
		{
			return false;
		}

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
		return world->SendPacket(&options, throttleReliable);
	}

	bool EOSHost::SendMultipartPacket(Peer &peer, uint32 channel, std::deque<Packet> &scheduledPackets, size_t &remainingSendBytes, bool allowBackgroundSpare)
	{
		Packet &packet = scheduledPackets.front();
		RN_ASSERT(packet.isReliable, "Multipart EOS packets need to be reliable");
		RN_ASSERT(packet.data->GetLength() <= MaximumReassembledPacketSize, "Reliable EOS packet exceeds the maximum reassembled size");

		constexpr size_t payloadSize = MaximumPacketSize - sizeof(ProtocolPayloadHeaderMultipart) - sizeof(ProtocolPacketEnvelope);
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
			size_t encodedLength = sizeof(ProtocolPayloadHeaderMultipart) + dataLength;
			size_t wireLength = encodedLength + sizeof(ProtocolPacketEnvelope);
			if(wireLength > remainingSendBytes) return false;

			Data *encodedPacket = new Data();
			ProtocolPayloadHeaderMultipart header;
			header.payloadType = static_cast<ProtocolPayloadType>(ProtocolPayloadTypeMultipart | (packet.isCompressed ? ProtocolPayloadTypeCompressed : 0));
			header.packetID = packet.multipartPacketID;
			header.dataPart = packet.multipartNextPart;
			header.totalDataParts = static_cast<uint16>(totalPartCount);
			encodedPacket->Append(&header, sizeof(header));
			encodedPacket->Append(packet.data->GetDataInRange(Range(dataOffset, dataLength)));
			bool didSend = SendScheduledPacket(peer, channel, encodedPacket->GetBytes(), encodedPacket->GetLength(), true, allowBackgroundSpare);
			encodedPacket->Release();
			if(!didSend) return false;
			remainingSendBytes -= wireLength;
			packet.multipartNextPart += 1;
		}

		peer._scheduledPacketBytes -= packet.data->GetLength();
		packet.data->Release();
		scheduledPackets.pop_front();
		return true;
	}

	bool EOSHost::SendPacketBatch(Peer &peer, uint32 channel, std::deque<Packet> &scheduledPackets, size_t &remainingSendBytes, bool allowBackgroundSpare)
	{
		bool reliable = scheduledPackets.front().isReliable;
		constexpr size_t overhead = sizeof(ProtocolPacketEnvelope);
		size_t sendLimit = std::min(MaximumPacketSize, remainingSendBytes);
		if(reliable)
		{
			sendLimit = std::min(sendLimit, std::min(EOSWorld::GetInstance()->GetRemainingReliableSendBytes(), GetRemainingReliableWindow(peer, channel, allowBackgroundSpare)));
		}
		sendLimit -= std::min(sendLimit, overhead);
		Data *encodedPacket = new Data();
		size_t packetCount = 0;
		for(const Packet &packet : scheduledPackets)
		{
			size_t encodedLength = sizeof(ProtocolPayloadHeader) + packet.data->GetLength();
			if(packet.isReliable != reliable || encodedPacket->GetLength() + encodedLength > sendLimit)
			{
				break;
			}

			ProtocolPayloadHeader header;
			header.payloadType = static_cast<ProtocolPayloadType>(ProtocolPayloadTypeData | (packet.isCompressed ? ProtocolPayloadTypeCompressed : 0));
			header.packetID = peer._channels[channel].nextPacketID++;
			header.dataLength = static_cast<uint16>(packet.data->GetLength());
			encodedPacket->Append(&header, sizeof(header));
			encodedPacket->Append(packet.data);
			packetCount += 1;
		}

		if(packetCount == 0)
		{
			encodedPacket->Release();
			return false;
		}

		bool didSend = SendScheduledPacket(peer, channel, encodedPacket->GetBytes(), encodedPacket->GetLength(), reliable, allowBackgroundSpare);
		if(didSend || !reliable)
		{
			if(didSend) remainingSendBytes -= encodedPacket->GetLength() + overhead;
			for(size_t packetIndex = 0; packetIndex < packetCount; packetIndex++)
			{
				Packet &packet = scheduledPackets.front();
				peer._scheduledPacketBytes -= packet.data->GetLength();
				packet.data->Release();
				scheduledPackets.pop_front();
			}
		}
		encodedPacket->Release();
		return didSend || !reliable;
	}

	void EOSHost::Update(float delta)
	{
		Retain();
		std::vector<EOS_ProductUserId> pingTargets;
		std::vector<std::pair<EOS_ProductUserId, uint64>> peersToDisconnect;
		Lock();
		EOSWorld *world = EOSWorld::GetInstance();
		if(!world || !world->GetP2PHandle())
		{
			Unlock();
			Release();
			return;
		}

		Clock::time_point now = Clock::now();
		double acknowledgementTime = std::chrono::duration<double>(now.time_since_epoch()).count();
		// A local suspend must not consume every remote peer's progress deadline at once.
		_peerProgressTime += std::min(std::max(delta, 0.0f), 1.0f);
		_pingTimer += std::max(delta, 0.0f);
		bool shouldPing = _pingTimer > 5.0f;
		if(shouldPing) _pingTimer = 0.0f;
		std::vector<Peer *> sendPeers;
		for(auto &entry : _peers)
		{
			Peer &peer = entry.second;
			// A mesh can replace one interrupted stream without ending the lobby session.
			double interruptionTimeout = peer._requiresBoundStream && !peer._wantsDisconnect ? EOSMeshInterruptionTimeout : EOSPeerProgressTimeout;
			bool interruptionExpired = peer._isConnectionInterrupted && _peerProgressTime - peer._connectionInterruptedAt >= interruptionTimeout;
			bool hasExpired = interruptionExpired;
			for(uint32 channel = 0; channel < 254 && !hasExpired; channel++)
			{
				const ChannelState &state = peer._channels[channel];
				hasExpired = state.sentReliableBytes > state.acknowledgedReliableBytes && _peerProgressTime - state.lastReliableProgressTime >= EOSPeerProgressTimeout;
			}
			if(hasExpired)
			{
				RNWarning("Disconnecting EOS peer " << entry.first << " after " << (interruptionExpired ? interruptionTimeout : EOSPeerProgressTimeout) << " seconds without " << (interruptionExpired ? "connection" : "reliable") << " progress");
				peersToDisconnect.emplace_back(entry.first, peer._reliableSendStream);
				continue;
			}
			if(peer._wantsDisconnect)
			{
				peer._disconnectDelay -= std::max(delta, 0.0f);
				if(peer._disconnectDelay <= 0.0f)
				{
					peersToDisconnect.emplace_back(entry.first, peer._reliableSendStream);
				}
			}
			if(shouldPing && peer._isConnectionActive && !peer._isConnectionInterrupted)
			{
				pingTargets.push_back(entry.first);
			}
			sendPeers.push_back(&peer);
		}
		_aggregateSendCredit = std::min<double>(EOSMaxSendBytesPerPeerPerUpdate,
			_aggregateSendCredit + std::max(delta, 0.0f) * _uploadBytesPerSecond);
		size_t aggregateSendBytes = static_cast<size_t>(_aggregateSendCredit);
		Clock::time_point dispatchStarted = Clock::now();
		for(size_t index = 0; index < sendPeers.size(); index++)
		{
			Peer &peer = *sendPeers[(_nextSendPeer + index) % sendPeers.size()];
			for(uint32 channel = 0; channel < 254; channel++)
			{
				ChannelState &state = peer._channels[channel];
				double elapsed = acknowledgementTime - state.lastAcknowledgementTime;
				if(!state.receivedReliableStream || elapsed < 0.05 ||
					(state.receivedReliableBytes == state.lastAcknowledgedBytes && elapsed < 0.25))
				{
					continue;
				}
				ReliableAcknowledgement ack {state.receivedReliableStream, state.receivedReliableBytes, static_cast<uint8>(channel)};
				if(SendRawPacket(peer.internalID, 254, &ack, sizeof(ack), false))
				{
					state.lastAcknowledgedBytes = state.receivedReliableBytes;
					state.lastAcknowledgementTime = acknowledgementTime;
				}
			}
		}
		// Visit all foreground peers before repair, reserving a quarter for repair.
		// Foreground reuses unused repair capacity before new peers borrow any remainder.
		std::vector<size_t> peerSentBytes(sendPeers.size(), 0);
		_backgroundSendCredit = std::min<double>(EOSMaxSendBytesPerPeerPerUpdate / 4,
			_backgroundSendCredit + std::max(delta, 0.0f) * _uploadBytesPerSecond / 4);
		size_t repairBudget = std::min(aggregateSendBytes, static_cast<size_t>(_backgroundSendCredit));
		size_t foregroundBudget = aggregateSendBytes - repairBudget;
		bool hasBackgroundChannel = _backgroundChannel < 254;
		bool limitSending = _limitAggregateSending || hasBackgroundChannel;
		for(unsigned pass = 0; pass < (hasBackgroundChannel ? 4u : 1u); ++pass)
		{
			if(pass == 3)
			{
				// Fair foreground passes may leave usable bytes when a later peer runs out of window.
				// Keep shared credit and SDK headroom for foreground to accumulate its next datagram.
				bool hasReadyForeground = false;
				for(size_t index = 0; index < sendPeers.size() && !hasReadyForeground; ++index)
				{
					const Peer &candidate = *sendPeers[(_nextSendPeer + index) % sendPeers.size()];
					if(candidate._isConnectionInterrupted || (candidate._requiresBoundStream && !candidate._didNotifyConnection)) continue;
					for(const auto &queuedChannel : candidate._scheduledPackets)
					{
						if(queuedChannel.first == _backgroundChannel || queuedChannel.second.empty()) continue;
						const Packet &packet = queuedChannel.second.front();
						size_t requiredBytes = packet.data->GetLength() + sizeof(ProtocolPayloadHeader) + sizeof(ProtocolPacketEnvelope);
						if(requiredBytes > MaximumPacketSize)
						{
							constexpr size_t payloadSize = MaximumPacketSize - sizeof(ProtocolPayloadHeaderMultipart) - sizeof(ProtocolPacketEnvelope);
							size_t dataOffset = static_cast<size_t>(packet.multipartNextPart) * payloadSize;
							requiredBytes = std::min(payloadSize, packet.data->GetLength() - dataOffset) + sizeof(ProtocolPayloadHeaderMultipart) + sizeof(ProtocolPacketEnvelope);
						}
						hasReadyForeground = requiredBytes <= EOSMaxSendBytesPerPeerPerUpdate - peerSentBytes[index] &&
							(!packet.isReliable || requiredBytes <= GetRemainingReliableWindow(candidate, queuedChannel.first));
						if(hasReadyForeground) break;
					}
				}
				if(hasReadyForeground) break;
			}
			bool background = pass == 1 || pass == 3;
			bool allowBackgroundSpare = pass == 3;
			std::vector<size_t> contendersRemaining(sendPeers.size() + 1, 0);
			for(size_t index = sendPeers.size(); index > 0; --index)
			{
				const Peer &candidate = *sendPeers[(_nextSendPeer + index - 1) % sendPeers.size()];
				contendersRemaining[index - 1] = contendersRemaining[index];
				if(candidate._isConnectionInterrupted || (pass == 3 && now >= candidate._backgroundSpareUntil)) continue;
				for(const auto &queuedChannel : candidate._scheduledPackets)
				{
					if(queuedChannel.second.empty() || (queuedChannel.first == _backgroundChannel) != background) continue;
					const Packet &packet = queuedChannel.second.front();
					size_t requiredBytes = std::min(size_t(MaximumPacketSize), packet.data->GetLength() + sizeof(ProtocolPayloadHeader) + (sizeof(ProtocolPacketEnvelope)));
					if(!packet.isReliable || requiredBytes <= GetRemainingReliableWindow(candidate, queuedChannel.first, allowBackgroundSpare))
					{
						contendersRemaining[index - 1]++;
						break;
					}
				}
			}
			size_t passBytes = pass == 0 && hasBackgroundChannel ? foregroundBudget : (pass == 1 ? std::min(aggregateSendBytes, repairBudget) : aggregateSendBytes);
			for(size_t index = 0; index < sendPeers.size(); index++)
			{
				Peer &peer = *sendPeers[(_nextSendPeer + index) % sendPeers.size()];
				if(pass == 3 && now >= peer._backgroundSpareUntil) continue;
				// Share the remaining SDK and mesh budget among peers with sendable work.
				size_t sendContenders = contendersRemaining[index];
				size_t globalSendBytes = world->GetRemainingReliableSendBytes();
				size_t remainingReliableBytes = std::max(globalSendBytes / std::max(sendContenders, size_t(1)), std::min(globalSendBytes, size_t(MaximumPacketSize)));
				size_t remainingSendBytes = EOSMaxSendBytesPerPeerPerUpdate;
				if(limitSending)
					remainingSendBytes = std::min(passBytes, std::max(passBytes / std::max(sendContenders, size_t(1)), std::min(passBytes, size_t(MaximumPacketSize))));
				remainingSendBytes = std::min(remainingSendBytes, EOSMaxSendBytesPerPeerPerUpdate - peerSentBytes[index]);
				std::vector<uint32> channels;
				for(const auto &channel : peer._scheduledPackets)
				{
					if((channel.first == _backgroundChannel) == background) channels.push_back(channel.first);
				}
				bool didMakeProgress;
				do
				{
					didMakeProgress = false;
					for(size_t offset = 0; offset < channels.size(); offset++)
					{
						uint32 channel = channels[(peer._nextSendChannel + offset) % channels.size()];
						auto queuedChannel = peer._scheduledPackets.find(channel);
						if(queuedChannel == peer._scheduledPackets.end()) continue;
						auto &scheduledPackets = queuedChannel->second;
						size_t channelSentBytes = 0;
						while(!scheduledPackets.empty())
						{
							if(!scheduledPackets.front().isReliable &&
								std::chrono::duration<double>(now - scheduledPackets.front().queuedAt).count() >= EOSUnreliablePacketLifetime)
							{
								peer._scheduledPacketBytes -= scheduledPackets.front().data->GetLength();
								scheduledPackets.front().data->Release();
								scheduledPackets.pop_front();
								continue;
							}
							if(peer._isConnectionInterrupted) break;
							if(_limitAggregateSending && std::chrono::duration<double>(Clock::now() - dispatchStarted).count() >= 0.002) break;
							bool reliable = scheduledPackets.front().isReliable;
							size_t channelBudget = std::min(remainingSendBytes, EOSSendChannelShare - std::min(channelSentBytes, EOSSendChannelShare));
							if(reliable) channelBudget = std::min(channelBudget, remainingReliableBytes);
							if(channelBudget < sizeof(ProtocolPayloadHeader)) break;
							size_t initialBudget = channelBudget;
							bool isMultipart = scheduledPackets.front().data->GetLength() + sizeof(ProtocolPayloadHeader) > MaximumPacketSize - sizeof(ProtocolPacketEnvelope);
							bool didAdvance = isMultipart ? SendMultipartPacket(peer, channel, scheduledPackets, channelBudget, allowBackgroundSpare) : SendPacketBatch(peer, channel, scheduledPackets, channelBudget, allowBackgroundSpare);
							size_t sentBytes = initialBudget - channelBudget;
							remainingSendBytes -= sentBytes;
							peerSentBytes[index] += sentBytes;
							if(pass == 1) _backgroundSendCredit = std::max(0.0, _backgroundSendCredit - sentBytes);
							if(limitSending) { aggregateSendBytes -= sentBytes; passBytes -= sentBytes; _aggregateSendCredit -= sentBytes; }
							remainingReliableBytes -= std::min(remainingReliableBytes, sentBytes);
							channelSentBytes += sentBytes;
							didMakeProgress = didMakeProgress || sentBytes > 0 || didAdvance;
							if(!didAdvance) break;
						}
						if(scheduledPackets.empty()) peer._scheduledPackets.erase(channel);
					}
				}
				while(didMakeProgress && remainingSendBytes >= sizeof(ProtocolPayloadHeader) &&
					(!_limitAggregateSending || std::chrono::duration<double>(Clock::now() - dispatchStarted).count() < 0.002));
			}
		}
		for(Peer *peer : sendPeers) peer->_nextSendChannel++;
		_nextSendPeer++;
		Unlock();

		for(EOS_ProductUserId peerID : pingTargets)
		{
			SendPing(peerID, false, 0);
		}
		for(const auto &closingPeer : peersToDisconnect)
		{
			Lock();
			auto peer = _peers.find(closingPeer.first);
			bool isCurrentPeer = peer != _peers.end() && peer->second._reliableSendStream == closingPeer.second;
			bool isExplicit = isCurrentPeer && peer->second._wantsDisconnect;
			Unlock();
			if(isCurrentPeer)
			{
				if(isExplicit) DisconnectClient(closingPeer.first);
				else HandlePeerFailure(closingPeer.first);
			}
		}
		Release();
	}

	void EOSHost::OnConnectionInterruptedCallback(const EOS_P2P_OnPeerConnectionInterruptedInfo *Data)
	{
		EOSHost *host = static_cast<EOSHost *>(Data->ClientData);
		if(!host) return;
		host->Lock();
		auto peer = host->_peers.find(Data->RemoteUserId);
		if(peer != host->_peers.end())
		{
			if(!peer->second._isConnectionInterrupted)
			{
				peer->second._connectionInterruptedAt = host->_peerProgressTime;
			}
			peer->second._isConnectionInterrupted = true;
			peer->second._isConnectionActive = false;
		}
		host->Unlock();
		RNInfo("EOS transport interrupted: local=" << host->_clientID << " remote=" << (Data->RemoteUserId ? host->GetClientIDForProductUserID(Data->RemoteUserId) : CLIENT_ID_NONE)
			<< " socket=" << (Data->SocketId ? Data->SocketId->SocketName : "<none>"));
	}

	void EOSHost::OnConnectionEstablishedCallback(const EOS_P2P_OnPeerConnectionEstablishedInfo *Data)
	{
		EOSHost *host = static_cast<EOSHost *>(Data->ClientData);
		if(!host) return;
		host->Lock();
		auto peer = host->_peers.find(Data->RemoteUserId);
		if(peer != host->_peers.end())
		{
			peer->second._isConnectionInterrupted = false;
			peer->second._connectionInterruptedAt = 0.0;
			peer->second._isConnectionActive = true;
		}
		host->Unlock();
		RNInfo("EOS transport established: local=" << host->_clientID << " remote=" << (Data->RemoteUserId ? host->GetClientIDForProductUserID(Data->RemoteUserId) : CLIENT_ID_NONE)
			<< " socket=" << (Data->SocketId ? Data->SocketId->SocketName : "<none>")
			<< " connectionType=" << static_cast<uint32>(Data->ConnectionType) << " networkType=" << static_cast<uint32>(Data->NetworkType));
	}

	EOSHost::Peer EOSHost::CreatePeer(EOSClientID clientID, EOS_ProductUserId internalID)
	{
		Peer peer;
		peer.clientID = clientID;
		peer.internalID = internalID;
		peer._reliableSendStream = ++_nextReliableStream;

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
		while(!peer._scheduledPackets.empty())
		{
			ClearScheduledPackets(peer, peer._scheduledPackets.begin()->first);
		}
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
			scheduled->second.pop_front();
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
			HandlePeerFailure(productUserID);
		}
	}

	EOSClientID EOSHost::GetUserIDForInternalID(EOS_ProductUserId internalID)
	{
		auto peer = _peers.find(internalID);
		return peer != _peers.end() ? peer->second.clientID : CLIENT_ID_NONE;
	}

	EOSClientID EOSHost::GetClientIDForProductUserID(EOS_ProductUserId productUserID) const
	{
		const String *userIDString = productUserID ? EOSWorld::GetInstance()->GetUserIDString(productUserID) : nullptr;
		const char *productUserIDString = userIDString ? userIDString->GetUTF8String() : nullptr;
		size_t stringLength = productUserIDString ? strlen(productUserIDString) : 0;
		RN_ASSERT(productUserIDString && stringLength >= 16, "Failed to derive a network ID from an EOS product user ID.");
		if(!productUserIDString || stringLength < 16) return CLIENT_ID_NONE;

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
		bool isActive = peer != _peers.end() && peer->second._isConnectionActive && !peer->second._isConnectionInterrupted;
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
