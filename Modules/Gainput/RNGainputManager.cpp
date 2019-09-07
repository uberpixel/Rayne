//
//  RNGainputManager.cpp
//  Rayne-Gainput
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNGainputManager.h"
#include "RNGainputInternals.h"

namespace RN
{
	RNDefineMeta(GainputManager, Object)

	GainputManager *GainputManager::_instance = nullptr;

	GainputManager* GainputManager::GetInstance()
	{
		return _instance;
	}

	GainputManager::GainputManager() : _internals(new GainputManagerInternals())
	{
		
	}
		
	GainputManager::~GainputManager()
	{
		_instance = nullptr;
		delete _internals;
	}
}
