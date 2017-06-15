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

		ENAPI Status GetStatus() const { return _status; }

	protected:
		virtual void Update(float delta) = 0;

		Status _status;

		String *_ip;
		uint32 _port;

		::ENetHost *_enetHost;
			
	private:
			
		RNDeclareMetaAPI(ENetHost, ENAPI)
	};
}

#endif /* defined(__RAYNE_ENETHOST_H_) */
