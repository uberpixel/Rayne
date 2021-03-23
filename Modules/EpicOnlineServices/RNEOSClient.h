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
		static void OnConnectionRequestCallback(const EOS_P2P_OnIncomingConnectionRequestInfo *Data);
		void ForceDisconnect();
			
		RNDeclareMetaAPI(EOSClient, EOSAPI)
	};
}

#endif /* defined(__RAYNE_EOSCLIENT_H_) */
