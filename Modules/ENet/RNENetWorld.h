//
//  RNENetWorld.h
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENETWORLD_H_
#define __RAYNE_ENETWORLD_H_

#include "RNENet.h"
#include "RNENetClient.h"
#include "RNENetServer.h"

namespace RN
{
	class ENetWorld : public SceneAttachment
	{
	public:
		ENAPI static ENetWorld *GetInstance();

		ENAPI ENetWorld();
		ENAPI ~ENetWorld() override;
		
		ENAPI void AddHost(ENetHost *host);
		ENAPI void RemoveHost(ENetHost *host);
		
		ENAPI void Ping(String *address, size_t repetitions);

	protected:
		void Update(float delta) override;
			
	private:
		static ENetWorld *_instance;

		Array *_hosts;
			
		RNDeclareMetaAPI(ENetWorld, ENAPI)
	};
}

#endif /* defined(__RAYNE_ENETWORLD_H_) */
