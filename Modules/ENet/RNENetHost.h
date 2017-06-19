//
//  RNENetHost.h
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENETHOST_H_
#define __RAYNE_ENETHOST_H_

#include "RNENet.h"

struct _ENetHost;
typedef _ENetHost ENetHost;

struct _ENetPeer;
typedef _ENetPeer ENetPeer;

namespace RN
{
	class ENetHost : public Object
	{
	public:
		friend class ENetWorld;

		struct Peer
		{
			uint32 id;
			ENetPeer *peer;
		};

		enum Status
		{
			Disconnected,
			Connected,
			Connecting,
			Disconnecting,
			Server
		};

		ENAPI ENetHost();
		ENAPI ~ENetHost();

		ENAPI void SendPackage(Data *data, uint32 receiverID = 0, uint32 channel = 0);
		ENAPI virtual void ReceivedPackage(Data *data, uint32 senderID, uint32 channel) {};

		ENAPI Status GetStatus() const { return _status; }

	protected:
		ENAPI virtual void Update(float delta) = 0;

		Status _status;

		String *_ip;
		uint32 _port;

		std::vector<Peer> _peers;

		::ENetHost *_enetHost;
		uint32 _channelCount;
			
	private:
			
		RNDeclareMetaAPI(ENetHost, ENAPI)
	};
}

#endif /* defined(__RAYNE_ENETHOST_H_) */
