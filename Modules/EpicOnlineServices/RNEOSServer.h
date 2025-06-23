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
		EOSAPI EOSServer(uint8 maxConnections = 16);
		EOSAPI ~EOSServer() override;

		EOSAPI void DisconnectUser(uint8 userID);
		EOSAPI void DisconnectUserDelayed(uint8 userID, uint16 data, float delay = 1.0f); //Using this, will not immediately force disconnect the user, leaving some time for previously sent data to arrive (like a reason for getting disconnected)
		EOSAPI void Disconnect() override;
		EOSAPI void DisconnectClient(EOS_ProductUserId productUserId) override;

		EOSAPI size_t GetNumberOfConnectedUsers() const;

	protected:
		EOSAPI virtual void Update(float delta) override;

		uint8 _maxConnections;
		std::set<uint8> _activeUserIDs;

	private:
		static void OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data);
		static void OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data);

		uint8 GetUserID();
		void ReleaseUserID(uint8 userID);

		uint64 _connectionClosedNotificationID;
		uint64 _connectionRequestNotificationID;

		RNDeclareMetaAPI(EOSServer, EOSAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_EOSSERVER_H_) */
