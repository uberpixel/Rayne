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

struct EOS_ProductUserIdDetails;
typedef struct EOS_ProductUserIdDetails* EOS_ProductUserId;

namespace RN
{
	class EOSHost : public Object
	{
	public:
		friend class EOSWorld;

		struct Peer
		{
			uint16 id;
			EOS_ProductUserId peer;
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
		EOSAPI virtual void Update(float delta) = 0;

		EOSAPI virtual void HandleDidConnect(uint16 userID) {};
		EOSAPI virtual void HandleDidDisconnect(uint16 userID, uint16 data) {};

		Status _status;

		std::map<uint16, Peer> _peers;
			
	private:
			
		RNDeclareMetaAPI(EOSHost, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSHOST_H_) */
