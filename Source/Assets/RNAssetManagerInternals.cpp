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

	PendingAsset::PendingAsset(std::shared_future<Asset *> &&future, MetaClass *meta) :
		_meta(meta),
		_future(std::move(future))
	{}
}
