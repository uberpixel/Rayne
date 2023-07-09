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
			ProtocolPacketTypeReliableDataMultipart,
			ProtocolPacketTypeReliableDataAck
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
			uint16 receiverID;
			uint32 channel;
			bool isReliable;
			Data *data;
		};

		struct Peer
		{
			uint16 userID;
			EOS_ProductUserId internalID;
			double smoothedRoundtripTime;
			
			uint8 _packetIDForChannel[256];
			uint8 _receivedIDForChannel[256];
			uint8 _lastReliableIDForChannel[256];
			
			uint8 _lastPingID;
			Clock::time_point _sentPingTime;
			
			bool _hasReliableInTransit;
			
			float _disconnectDelay;
			bool _wantsDisconnect;
			
			std::map<uint32, std::queue<Packet>> _scheduledPackets;
			
			std::map<uint32, uint32> _multipartPacketTotalParts; //Maps channel to total number of parts for multipart data
			std::map<uint32, uint32> _multipartPacketCurrentPart; //Maps channel to current part index for multipart data
			std::map<uint32, uint32> _multipartPacketID; //Maps channel to current packet id for multipart data, should be the same until all parts are received
			std::map<uint32, Data*> _multipartPacketData; //Maps channel to current data collecting all multipart data into one piece
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
		EOSAPI virtual void HandleDidDisconnect(uint16 userID, uint16 reason) {};
		
		EOSAPI void SendPing(uint16 receiverID, bool isResponse, uint8 responseID);
		EOSAPI bool IsPacketInOrder(EOSHost::ProtocolPacketType packetType, uint16 senderID, uint8 packetID, uint8 channel);

		Status _status;
		float _pingTimer;
		
		std::map<uint16, Peer> _peers;
			
	private:
			
		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSHOST_H_) */
