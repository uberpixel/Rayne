//
//  RNAssetManagerInternals.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAssetManagerInternals.h"

namespace RN
{
	RNDefineMeta(PendingAsset, Object)
	RNDefineMeta(LoadedAsset, Object)

	LoadedAsset::LoadedAsset(Asset *asset, MetaClass *meta) :
		_meta(meta),
		_asset(asset)
	{}

	PendingAsset::PendingAsset(MetaClass *meta) :
		_meta(meta)
	{
		_future = _promise.get_future().share();
	}

	void PendingAsset::SetAsset(Asset *asset)
	{
		_promise.set_value(asset);
	}
	void PendingAsset::SetException(std::exception_ptr exception)
	{
		_promise.set_exception(exception);
	}
}
