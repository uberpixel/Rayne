//
//  RNAsset.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Objects/RNString.h"
#include "RNAsset.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(Asset, Object)

	Asset::Asset() :
		_coordinator(nullptr),
		_name(nullptr),
		_meta(nullptr)
	{}

	void Asset::Dealloc()
	{
		if(_coordinator)
		{
			_coordinator->__RemoveAsset(this, _name);
			_coordinator = nullptr;
			_meta = nullptr;

			SafeRelease(_name);
		}

		Object::Dealloc();
	}

	void Asset::__AwakeWithCoordinator(AssetManager *coordinator, String *name, MetaClass *meta)
	{
		_meta = meta;
		_coordinator = coordinator;
		_name = SafeCopy(name);
	}

	const String *Asset::GetDescription() const
	{
		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << ", " << _name << ">");
	}
}
