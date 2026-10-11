//
//  RNEOSHost.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSHOST_H_
#define __RAYNE_EOSHOST_H_

#include "RNEOS.h"
#include <queue>

struct EOS_ProductUserIdDetails;
typedef EOS_ProductUserIdDetails *EOS_ProductUserId;
using EOSClientID = RN::uint64;
constexpr EOSClientID CLIENT_ID_NONE = std::numeric_limits<EOSClientID>::max();
constexpr EOSClientID CLIENT_ID_RESERVED = std::numeric_limits<EOSClientID>::max() - 1;

typedef struct _tagEOS_P2P_OnPeerConnectionInterruptedInfo EOS_P2P_OnPeerConnectionInterruptedInfo;
typedef struct _tagEOS_P2P_OnPeerConnectionEstablishedInfo EOS_P2P_OnPeerConnectionEstablishedInfo;

namespace RN
{
	class EOSHost : public Object
	{
	public:
		friend class EOSWorld;

		enum ProtocolPacketType : uint8
		{
			ProtocolPacketTypeConnectRequest,
			ProtocolPacketTypeConnectResponse,
			ProtocolPacketTypePingRequest,
			ProtocolPacketTypePingResponse,
			ProtocolPacketTypeReliableEnvelope,
			ProtocolPacketTypeUnreliableEnvelope
		};

		enum ProtocolPayloadType : uint8
		{
			ProtocolPayloadTypeData,
			ProtocolPayloadTypeMultipart
		};

		#pragma pack(push, 1)
		struct ProtocolPacketHeader
		{
			ProtocolPacketType packetType;
			uint32 packetID;
			uint16 dataLength;
		};

		struct ProtocolPayloadHeader
		{
			ProtocolPayloadType payloadType;
			uint32 packetID;
			uint16 dataLength; // Allows batching multiple application packets in one envelope.
		};

		struct ProtocolPayloadHeaderMultipart
		{
			ProtocolPayloadType payloadType; //Has to be ProtocolPayloadTypeMultipart
			uint32 packetID; //The packet ID is the same for all parts!
			uint16 dataPart; //The current part of the data, starts with 0
			uint16 totalDataParts; //The number of parts to wait for before being able to use it
		};
		struct ProtocolPacketEnvelope
		{
			ProtocolPacketType packetType;
			uint64 streamID;
			uint64 sentBytes;
		};

		struct ReliableAcknowledgement
		{
			uint64 streamID;
			uint64 receivedBytes;
			uint8 channel;
		};
		#pragma pack(pop)
		static_assert(sizeof(ProtocolPacketEnvelope) == 17, "Unexpected EOS packet envelope layout");
		static_assert(sizeof(ReliableAcknowledgement) == 17, "Unexpected EOS acknowledgement layout");
		static_assert(sizeof(ProtocolPacketHeader) == 7, "Unexpected EOS packet header layout");
		static_assert(sizeof(ProtocolPayloadHeader) == 7, "Unexpected EOS payload header layout");
		static_assert(sizeof(ProtocolPayloadHeaderMultipart) == 9, "Unexpected EOS multipart packet header layout");

		struct Packet
		{
			bool isReliable = false;
			Data *data = nullptr;
			uint32 multipartPacketID = 0;
			uint16 multipartNextPart = 0;
			bool isMultipartStarted = false;
			Clock::time_point queuedAt;
		};

		struct ChannelState
		{
			uint32 nextPacketID = 0;
			uint32 lastReceivedUnreliablePacketID = 0;
			bool hasReceivedUnreliablePacket = false;
			uint64 sentReliableBytes = 0;
			uint64 acknowledgedReliableBytes = 0;
			uint64 receivedReliableStream = 0;
			uint64 receivedReliableBytes = 0;
			uint64 lastAcknowledgedBytes = 0;
			double lastAcknowledgementTime = -1.0;
			double lastReliableProgressTime = 0.0;
		};

		struct MultipartAssembly
		{
			uint32 packetID;
			uint16 totalParts;
			uint16 currentPart;
			Data *data;
		};

		struct Peer
		{
			EOSClientID clientID = CLIENT_ID_NONE;
			EOS_ProductUserId internalID = nullptr;
			double smoothedRoundtripTime = 0.05;

			ChannelState _channels[256];

			uint32 _lastPingID = 0;
			Clock::time_point _sentPingTime;
			Clock::time_point _backgroundSpareUntil {};

			float _disconnectDelay = 0.0f;
			bool _wantsDisconnect = false;
			bool _isConnectionActive = false;
			bool _isConnectionInterrupted = false;
			double _connectionInterruptedAt = 0.0;
			bool _didNotifyConnection = false;

			std::map<uint32, std::deque<Packet>> _scheduledPackets;
			size_t _scheduledPacketBytes = 0;
			std::map<uint32, MultipartAssembly> _multipartAssemblies;
			float _reconnectTimer = 0.0f;
			bool _isReconnectScheduled = false;
			uint32 _nextSendChannel = 0;
			uint64 _reliableSendStream = 0;
			uint64 _remoteStream = 0;
			uint64 _pendingLocalStream = 0;
			uint64 _pendingRemoteStream = 0;
			bool _requiresBoundStream = false;
			float _reconnectInterval = 3.0f;
			size_t _outstandingReliableBytes = 0;
		};

		struct MultipartReceiveResult
		{
			Data *data = nullptr;
			bool progressed = false;
			bool lostData = false;
		};

		struct DecodeResult
		{
			std::vector<Data *> packets;
			bool connectRequest = false;
			bool connectResponse = false;
			bool multipartProgress = false;
			bool lostReliableData = false;
		};

		enum Status
		{
			Disconnected,
			Connected,
			Connecting,
			Disconnecting,
			Server
		};

		EOSAPI EOSHost(RN::String *socketID);
		EOSAPI ~EOSHost() override;

		enum class SendResult { Queued, InvalidPacket, PeerUnavailable, QueueFull };
		EOSAPI SendResult TrySendPacket(Data *data, EOSClientID receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI void LobbyMemberJoined(EOS_ProductUserId productUserID);
		EOSAPI void LobbyMemberLeft(EOS_ProductUserId productUserID);

		EOSAPI void SendPacket(Data *data, EOSClientID receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI void BroadcastPacket(Data *data, uint32 channel = 0, bool reliable = false, EOSClientID excludeClientID = CLIENT_ID_NONE);
		EOSAPI virtual void ReceivedPacket(Data *data, EOSClientID senderID, uint32 channel) {}

		EOSAPI void SetUploadBytesPerSecond(size_t bytes) { _uploadBytesPerSecond = std::max(size_t(64 * 1024), std::min(bytes, size_t(8192 * 1024))); }
		EOSAPI size_t GetUploadBytesPerSecond() const { return _uploadBytesPerSecond; }
		// Caller-controlled allowance for background work after foreground; reconnect does not renew it.
		EOSAPI void SetPeerBackgroundSpareTime(EOSClientID clientID, uint32 seconds);
		EOSAPI bool HasPeerBackgroundSpareTime(EOSClientID clientID);
		// Opt in to a single lower priority channel; reserved channels disable it.
		EOSAPI void SetBackgroundChannel(uint32 channel) { _backgroundChannel = channel < 254 ? channel : 256; }

		EOSAPI Status GetStatus() const { return _status; }
		EOSAPI double GetLastRoundtripTime(EOSClientID peerID);
		EOSAPI virtual void Disconnect() = 0;
		EOSAPI virtual void DisconnectClient(EOS_ProductUserId productUserId) = 0;
		
		EOSAPI virtual bool HasClient(EOSClientID clientID);
		EOSAPI virtual bool HasClient(const RN::String *eosUserIDString);
		EOSAPI bool IsClientConnectionActive(EOSClientID clientID);
		EOSAPI const RN::String *GetEOSUserIDStringForClient(EOSClientID clientID) const;
		
		const String *GetSocketID() const { return _socketID; }

	protected:
		// Keep the previous 993-byte application payload capacity with the 17-byte envelope.
		static constexpr size_t MaximumPacketSize = 1017;
		static constexpr size_t MaximumReassembledPacketSize = 16 * 1024 * 1024;

		EOSAPI virtual void Update(float delta);
		
		EOSAPI virtual void ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel);

		EOSAPI Peer CreatePeer(EOSClientID clientID, EOS_ProductUserId internalID);
		EOSAPI void ClearPeerData(Peer &peer);
		bool IsReservedChannel(uint32 channel) const { return channel >= 254; }
		EOSAPI void ClearScheduledPackets(Peer &peer);
		EOSAPI void ClearScheduledPackets(Peer &peer, uint32 channel);
		EOSAPI void ClearMultipartPacket(Peer &peer, uint32 channel);
		EOSAPI EOSClientID GetUserIDForInternalID(EOS_ProductUserId internalID);
		EOSAPI EOSClientID GetClientIDForProductUserID(EOS_ProductUserId productUserID) const;
		EOSAPI DecodeResult DecodePackets(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel);

		EOSAPI virtual void HandleLobbyMemberJoined(EOSClientID clientID) {}
		EOSAPI virtual void HandleLobbyMemberLeft(EOSClientID clientID) {}
		EOSAPI virtual void HandlePeerFailure(EOS_ProductUserId productUserID) { DisconnectClient(productUserID); }
		EOSAPI virtual void HandleDidConnect(EOSClientID clientID) {}
		EOSAPI virtual void HandleDidDisconnect(EOSClientID clientID, uint16 reason) {}
		EOSAPI virtual void HandleReliableMultipartProgress(EOSClientID clientID, uint32 channel) {}
		EOSAPI virtual void HandleReliablePacketLoss(EOSClientID clientID);
		EOSAPI virtual void HandleReliablePacketLoss(EOSClientID clientID, uint32 channel);

		EOSAPI bool IsUnreliablePacketInOrder(Peer &peer, uint32 packetID, uint8 channel);
		EOSAPI void SendPacket(Data *data, EOS_ProductUserId receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI bool SendRawPacket(EOS_ProductUserId receiverID, uint32 channel, const void *bytes, size_t length, bool reliable, bool throttleReliable = false);
		EOSAPI void SendPing(EOS_ProductUserId receiverID, bool isResponse, uint32 responseID);
		EOSAPI void ClosePeerTransport(EOS_ProductUserId productUserID);

		EOSClientID _clientID;
		Status _status;
		float _pingTimer;
		double _peerProgressTime = 0.0;
		bool _limitAggregateSending = false;
		double _aggregateSendCredit = 128 * 1024;
		double _backgroundSendCredit = 32 * 1024;
		size_t _uploadBytesPerSecond = 512 * 1024;
		uint32 _backgroundChannel = 256;
		
		RN::String *_socketID;

		std::map<EOS_ProductUserId, Peer> _peers;
		std::map<EOSClientID, EOS_ProductUserId> _idMap;

	private:
		SendResult QueuePacket(Data *data, EOS_ProductUserId receiverID, uint32 channel, bool reliable, bool requireActive);
		size_t GetRemainingReliableWindow(const Peer &peer, uint32 channel) const;
		bool SendScheduledPacket(Peer &peer, uint32 channel, const void *bytes, size_t length, bool reliable);
		static void OnConnectionInterruptedCallback(const EOS_P2P_OnPeerConnectionInterruptedInfo *Data);
		static void OnConnectionEstablishedCallback(const EOS_P2P_OnPeerConnectionEstablishedInfo *Data);
		uint64 _connectionInterruptedNotificationID = 0;
		uint64 _connectionEstablishedNotificationID = 0;
		uint64 _nextReliableStream = static_cast<uint64>(Clock::now().time_since_epoch().count());
		size_t _nextSendPeer = 0;
		bool ReadPacketHeader(const uint8 *rawData, size_t bytesWritten, size_t &dataIndex, ProtocolPacketHeader &packetHeader, const uint8 *&payload) const;
		MultipartReceiveResult ReceiveMultipartPacket(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel);
		bool SendMultipartPacket(Peer &peer, uint32 channel, std::deque<Packet> &scheduledPackets, size_t &remainingSendBytes);
		bool SendPacketBatch(Peer &peer, uint32 channel, std::deque<Packet> &scheduledPackets, size_t &remainingSendBytes);

		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSHOST_H_) */
