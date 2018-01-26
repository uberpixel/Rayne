//
//  RNRecastWorld.cpp
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRecastWorld.h"

namespace RN
{
	RNDefineMeta(RecastWorld, SceneAttachment)

	RecastWorld *RecastWorld::_instance = nullptr;

	RecastWorld* RecastWorld::GetInstance()
	{
		return _instance;
	}

	RecastWorld::RecastWorld()
	{
		RN_ASSERT(!_instance, "There already is an ENetWorld!");

//		if(enet_initialize() != 0)
		{
			RNDebug("Failed initializing enet.");
			return;
		}

		_instance = this;
	}
		
	RecastWorld::~RecastWorld()
	{
		_instance = nullptr;
	}

	void RecastWorld::Update(float delta)
	{
		
	}
}
