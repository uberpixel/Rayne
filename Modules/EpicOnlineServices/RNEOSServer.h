//
//  RNEOSServer.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSSERVER_H_
#define __RAYNE_EOSSERVER_H_

#include "RNEOSHost.h"
#include <set>

typedef struct _tagEOS_P2P_OnIncomingConnectionRequestInfo EOS_P2P_OnIncomingConnectionRequestInfo;
typedef struct _tagEOS_P2P_OnRemoteConnectionClosedInfo EOS_P2P_OnRemoteConnectionClosedInfo;

namespace RN
{
	class EOSServer : public EOSHost
	{
	public:
		EOSAPI EOSServer(uint16 maxConnections = 16);
		EOSAPI ~EOSServer();

		EOSAPI void DisconnectUser(uint16 userID, uint16 data);
		EOSAPI void DisconnectAll();
		
		EOSAPI size_t GetNumberOfConnectedUsers() const;

	protected:
		EOSAPI virtual void Update(float delta) override;

		uint16 _maxConnections;
		std::set<uint16> _activeUserIDs;
			
	private:
		static void OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data);
		static void OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data);
		
		uint16 GetUserID();
		void ReleaseUserID(uint16 userID);

		uint64 _connectionClosedNotificationID;
		uint64 _connectionRequestNotificationID;
			
		RNDeclareMetaAPI(EOSServer, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSSERVER_H_) */
