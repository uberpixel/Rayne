//
//  RNENetServer.h
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENETSERVER_H_
#define __RAYNE_ENETSERVER_H_

#include "RNENetHost.h"
#include <set>

namespace RN
{
	class ENetServer : public ENetHost
	{
	public:
		ENAPI ENetServer(uint32 port = 1234, uint16 maxConnections = 16, uint32 channelCount = 0);
		ENAPI ~ENetServer();

		ENAPI void Connect(String *ip, uint32 port);
		ENAPI void Disconnect();

	protected:
		ENAPI virtual void Update(float delta) override;
		ENAPI virtual void HandleNewConnection(uint16 userID) {};

		uint16 _maxConnections;
		std::set<uint16> _activeUserIDs;
			
	private:
		void HandleDisconnect();
		uint16 GetUserID();
		void ReleaseUserID(uint16 userID);
			
		RNDeclareMetaAPI(ENetServer, ENAPI)
	};
}

#endif /* defined(__RAYNE_ENETSERVER_H_) */
