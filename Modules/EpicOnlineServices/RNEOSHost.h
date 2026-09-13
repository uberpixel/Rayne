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
			ProtocolPacketTypeData,
			ProtocolPacketTypeReliableData,
			ProtocolPacketTypeReliableDataMultipart
		};

		#pragma pack(push, 1)
		struct ProtocolPacketHeader
		{
			//56 bit

			ProtocolPacketType packetType;
			uint32 packetID;
			uint16 dataLength; //This allows to bundle multiple packets into one networking packet to send, improving EOS sending performance a lot
		};

		struct ProtocolPacketHeaderMultipart
		{
			//72 bit

			ProtocolPacketType packetType; //Has to be ProtocolPacketTypeReliableDataMultipart
			uint32 packetID; //The packet ID is the same for all parts!
			uint16 dataPart; //The current part of the data, starts with 0
			uint16 totalDataParts; //The number of parts to wait for before being able to use it
		};
		#pragma pack(pop)
		static_assert(sizeof(ProtocolPacketHeader) == 7, "Unexpected EOS packet header layout");
		static_assert(sizeof(ProtocolPacketHeaderMultipart) == 9, "Unexpected EOS multipart packet header layout");

		struct Packet
		{
			bool isReliable = false;
			Data *data = nullptr;
			uint32 multipartPacketID = 0;
			uint16 multipartNextPart = 0;
			bool isMultipartStarted = false;
		};

		struct ChannelState
		{
			uint32 nextPacketID = 0;
			uint32 lastReceivedUnreliablePacketID = 0;
			bool hasReceivedUnreliablePacket = false;
		};

		struct MultipartAssembly
		{
			uint32 packetID;
			uint16 totalParts;
			uint16 currentPart;
			Data *data;
			float age;
		};

		struct Peer
		{
			EOSClientID clientID = CLIENT_ID_NONE;
			EOS_ProductUserId internalID = nullptr;
			double smoothedRoundtripTime = 0.05;

			ChannelState _channels[256];

			uint32 _lastPingID = 0;
			Clock::time_point _sentPingTime;

			float _disconnectDelay = 0.0f;
			bool _wantsDisconnect = false;
			bool _isConnectionActive = false;
			bool _didNotifyConnection = false;

			std::map<uint32, std::queue<Packet>> _scheduledPackets;
			size_t _scheduledPacketBytes = 0;
			std::map<uint32, MultipartAssembly> _multipartAssemblies;
			float _reconnectTimer = 0.0f;
			bool _isReconnectScheduled = false;
		};

		struct MultipartReceiveResult
		{
			Data *data = nullptr;
			bool progressed = false;
			bool lostData = false;
		};

		struct DecodedPacket
		{
			ProtocolPacketType type;
			uint32 packetID;
			Data *data;
		};

		struct DecodeResult
		{
			std::vector<DecodedPacket> packets;
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

		EOSAPI void SendPacket(Data *data, EOSClientID receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI void BroadcastPacket(Data *data, uint32 channel = 0, bool reliable = false, EOSClientID excludeClientID = CLIENT_ID_NONE);
		EOSAPI virtual void ReceivedPacket(Data *data, EOSClientID senderID, uint32 channel) {}

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
		static constexpr size_t MaximumPacketSize = 1000;
		static constexpr size_t MaximumReassembledPacketSize = 16 * 1024 * 1024;

		EOSAPI virtual void Update(float delta);
		
		EOSAPI virtual void ReceivedPacketInternal(uint8 *rawData, uint32 bytesWritten, EOS_ProductUserId senderUserID, uint8 channel);

		EOSAPI Peer CreatePeer(EOSClientID clientID, EOS_ProductUserId internalID);
		EOSAPI void ClearPeerData(Peer &peer);
		EOSAPI void ClearScheduledPackets(Peer &peer);
		EOSAPI void ClearScheduledPackets(Peer &peer, uint32 channel);
		EOSAPI void ClearMultipartPacket(Peer &peer, uint32 channel);
		EOSAPI EOSClientID GetUserIDForInternalID(EOS_ProductUserId internalID);
		EOSAPI EOSClientID GetClientIDForProductUserID(EOS_ProductUserId productUserID) const;
		EOSAPI DecodeResult DecodePackets(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel);

		EOSAPI virtual void HandleDidConnect(EOSClientID clientID) {}
		EOSAPI virtual void HandleDidDisconnect(EOSClientID clientID, uint16 reason) {}
		EOSAPI virtual void HandleReliableMultipartProgress(EOSClientID clientID, uint32 channel) {}
		EOSAPI virtual void HandleReliablePacketLoss(EOSClientID clientID);
		EOSAPI virtual void HandleReliablePacketLoss(EOSClientID clientID, uint32 channel);

		EOSAPI bool IsUnreliablePacketInOrder(Peer &peer, uint32 packetID, uint8 channel);
		EOSAPI void SendPacket(Data *data, EOS_ProductUserId receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI bool SendRawPacket(EOS_ProductUserId receiverID, uint32 channel, const void *bytes, size_t length, bool reliable);
		EOSAPI void SendPing(EOS_ProductUserId receiverID, bool isResponse, uint32 responseID);

		EOSClientID _clientID;
		Status _status;
		float _pingTimer;
		
		RN::String *_socketID;

		std::map<EOS_ProductUserId, Peer> _peers;
		std::map<EOSClientID, EOS_ProductUserId> _idMap;

	private:
		bool ReadPacketHeader(const uint8 *rawData, size_t bytesWritten, size_t &dataIndex, ProtocolPacketHeader &packetHeader, const uint8 *&payload) const;
		MultipartReceiveResult ReceiveMultipartPacket(Peer &peer, const uint8 *rawData, size_t bytesWritten, uint32 channel);
		bool SendMultipartPacket(Peer &peer, uint32 channel, std::queue<Packet> &scheduledPackets, size_t &remainingSendBytes);
		bool SendPacketBatch(Peer &peer, uint32 channel, std::queue<Packet> &scheduledPackets, size_t &remainingSendBytes);

		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSHOST_H_) */
