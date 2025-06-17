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
constexpr RN::uint16 CLIENT_ID_NONE = std::numeric_limits<RN::uint8>::max();

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

		struct ProtocolPacketHeader
		{
			//32 bit

			ProtocolPacketType packetType;
			uint8 packetID;
			uint16 dataLength; //This allows to bundle multiple packets into one networking packet to send, improving EOS sending performance a lot
		};

		struct ProtocolPacketHeaderMultipart
		{
			//48 bit

			ProtocolPacketType packetType; //Has to be ProtocolPacketTypeReliableDataMultipart
			uint8 packetID; //The packet ID is the same for all parts!
			uint16 dataPart; //The current part of the data, starts with 0
			uint16 totalDataParts; //The number of parts to wait for before being able to use it
		};

		struct Packet
		{
			EOS_ProductUserId receiverID;
			uint32 channel;
			bool isReliable;
			Data *data;
		};

		struct Peer
		{
			uint16 clientID;
			EOS_ProductUserId internalID;
			double smoothedRoundtripTime;

			uint8 _packetIDForChannel[256];
			uint8 _receivedIDForChannel[256];

			uint8 _lastPingID;
			Clock::time_point _sentPingTime;

			float _disconnectDelay;
			bool _wantsDisconnect;

			std::map<uint32, std::queue<Packet>> _scheduledPackets;

			std::map<uint32, uint32> _multipartPacketTotalParts; //Maps channel to total number of parts for multipart data
			std::map<uint32, uint32> _multipartPacketCurrentPart; //Maps channel to current part index for multipart data
			std::map<uint32, uint32> _multipartPacketID; //Maps channel to current packet id for multipart data, should be the same until all parts are received
			std::map<uint32, Data *> _multipartPacketData; //Maps channel to current data collecting all multipart data into one piece
		};

		enum Status
		{
			Disconnected,
			Connected,
			Connecting,
			Disconnecting,
			Server
		};

		EOSAPI EOSHost();
		EOSAPI ~EOSHost() override;

		EOSAPI void SendPacket(Data *data, uint16 receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI void BroadcastPacket(Data *data, uint32 channel = 0, bool reliable = false, uint16 excludeClientID = CLIENT_ID_NONE);
		EOSAPI virtual void ReceivedPacket(Data *data, uint16 senderID, uint32 channel) {}

		EOSAPI Status GetStatus() const { return _status; }
		EOSAPI double GetLastRoundtripTime(uint16 peerID);
		EOSAPI virtual void Disconnect() = 0;
		EOSAPI virtual void DisconnectClient(EOS_ProductUserId productUserId) = 0;

	protected:
		EOSAPI virtual void Update(float delta);

		EOSAPI Peer CreatePeer(uint16 clientID, EOS_ProductUserId internalID);
		EOSAPI uint16 GetUserIDForInternalID(EOS_ProductUserId internalID);

		EOSAPI virtual void HandleDidConnect(uint16 clientID) {}
		EOSAPI virtual void HandleDidDisconnect(uint16 clientID, uint16 reason) {}

		EOSAPI bool IsPacketInOrder(ProtocolPacketType packetType, EOS_ProductUserId senderID, uint8 packetID, uint8 channel);
		EOSAPI void SendPacket(Data *data, EOS_ProductUserId receiverID, uint32 channel = 0, bool reliable = false);
		EOSAPI void SendPing(EOS_ProductUserId receiverID, bool isResponse, uint8 responseID);

		uint16 _clientID;
		Status _status;
		float _pingTimer;

		std::map<EOS_ProductUserId, Peer> _peers;
		std::map<uint16, EOS_ProductUserId> _idMap;

	private:
		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSHOST_H_) */
