//
//  RNEOSClient.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOSCLIENT_H_
#define __RAYNE_EOSCLIENT_H_

#include "RNEOSHost.h"

typedef struct _tagEOS_P2P_OnIncomingConnectionRequestInfo EOS_P2P_OnIncomingConnectionRequestInfo;
typedef struct _tagEOS_P2P_OnRemoteConnectionClosedInfo EOS_P2P_OnRemoteConnectionClosedInfo;

namespace RN
{
	class EOSClient : public EOSHost
	{
	public:
		EOSAPI EOSClient();
		EOSAPI ~EOSClient();

		EOSAPI void Connect(EOS_ProductUserId serverProductID);
		EOSAPI void Disconnect();

	protected:
		EOSAPI virtual void Update(float delta) override;
			
	private:
		static void OnConnectionClosedCallback(const EOS_P2P_OnRemoteConnectionClosedInfo *Data);
		
		void ForceDisconnect(RN::uint16 reason);
		uint64 _connectionClosedNotificationID;
			
		RNDeclareMetaAPI(EOSClient, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSCLIENT_H_) */
