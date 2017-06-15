//
//  RNENetHost.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetHost.h"
#include "RNENetWorld.h"
#include "enet/enet.h"

namespace RN
{
	RNDefineMeta(ENetHost, Object)

	ENetHost::ENetHost() : _ip(nullptr), _port(0)
	{
		RN_ASSERT(ENetWorld::GetInstance(), "You need an ENetWorld before creating a host!");

		ENetWorld::GetInstance()->AddHost(this);
	}
		
	ENetHost::~ENetHost()
	{
		ENetWorld::GetInstance()->RemoveHost(this);
	}
}
