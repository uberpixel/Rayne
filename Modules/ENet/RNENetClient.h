//
//  RNENetClient.h
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENETCLIENT_H_
#define __RAYNE_ENETCLIENT_H_

#include "RNENetHost.h"

namespace RN
{
	class ENetClientEncryptorSharedInternals;
	class ENetClientEncryptor;
	class ENetClient : public ENetHost
	{
	public:
		friend ENetClientEncryptor;
		
		ENAPI ENetClient(uint32 channelCount = 1);
		ENAPI ~ENetClient();
		
		ENAPI void EnableEncryption(String *trustedCertStorePath);

		ENAPI void Connect(String *ip, uint32 port = 1234);
		ENAPI void Disconnect();

	protected:
		ENAPI virtual void Update(float delta) override;
			
	private:
		void ForceDisconnect();

		float _connectionTimeOut;
		
		ENetClientEncryptorSharedInternals *_encryptorSharedInternals;
			
		RNDeclareMetaAPI(ENetClient, ENAPI)
	};
}

#endif /* defined(__RAYNE_ENETCLIENT_H_) */
