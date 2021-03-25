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
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;

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
			ProtocolPacketTypeReliableDataAck
		};
		
		struct ProtocolPacketHeader
		{
			ProtocolPacketType packetType;
			uint8 packetID;
		};

		struct Peer
		{
			uint16 userID;
			EOS_ProductUserId internalID;
			double smoothedPing;
			
			
			uint8 _packetIDForChannel[254];
			uint8 _receivedIDForChannel[254];
			uint8 _lastReliableIDForChannel[254];
			
			uint8 _lastPingID;
			struct timespec _sentPingTime;
			
			bool _hasReliableInTransit;
		};
		
		struct Packet
		{
			uint16 receiverID;
			uint32 channel;
			bool isReliable;
			Data *data;
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
		EOSAPI ~EOSHost();

		EOSAPI void SendPacket(Data *data, uint16 receiverID = 0, uint32 channel = 0, bool reliable = false);
		EOSAPI virtual void ReceivedPacket(Data *data, uint32 senderID, uint32 channel) {};

		EOSAPI Status GetStatus() const { return _status; }
		EOSAPI bool HasReliableDataInTransit();
		EOSAPI double GetLastRoundtripTime(uint16 peerID);
		EOSAPI void SetTimeout(uint16 peerID, size_t limit, size_t minimum, size_t maximum);
		EOSAPI void SetPingInterval(uint16 peerID, size_t interval);

	protected:
		EOSAPI virtual void Update(float delta);
		
		EOSAPI Peer CreatePeer(uint16 userID, EOS_ProductUserId internalID);
		EOSAPI uint16 GetUserIDForInternalID(EOS_ProductUserId internalID);

		EOSAPI virtual void HandleDidConnect(uint16 userID) {};
		EOSAPI virtual void HandleDidDisconnect(uint16 userID, uint16 data) {};
		
		EOSAPI void SendPing(uint16 receiverID, bool isResponse, uint8 responseID);
		EOSAPI bool IsPacketInOrder(EOSHost::ProtocolPacketType packetType, uint16 senderID, uint8 packetID, uint8 channel);

		Status _status;
		float _pingTimer;
		
		std::map<uint16, Peer> _peers;
		std::queue<Packet> _scheduledPackets;
			
	private:
			
		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSHOST_H_) */
