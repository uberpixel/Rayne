//
//  RNAssetManagerInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSETMANAGERINTERNALS_H_
#define __RAYNE_ASSETMANAGERINTERNALS_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNAsset.h"

namespace RN
{
	class LoadedAsset : public Object
	{
	public:
		LoadedAsset(Asset *asset, MetaClass *meta);

		MetaClass *GetMeta() const { return _meta; }
		Asset *GetAsset() const { return _asset.Load(); }

	private:
		WeakRef<Asset> _asset;
		MetaClass *_meta;

		__RNDeclareMetaInternal(LoadedAsset)
	};

	class PendingAsset : public Object
	{
	public:
		PendingAsset(std::shared_future<Asset *> &&future, MetaClass *meta);

		MetaClass *GetMeta() const { return _meta; }
		std::shared_future<Asset *> &GetFuture() const { return const_cast<std::shared_future<Asset *> &>(_future); }

	private:
		std::shared_future<Asset *> _future;
		MetaClass *_meta;

		__RNDeclareMetaInternal(PendingAsset)
	};
}

#endif /* __RAYNE_ASSETMANAGERINTERNALS_H_ */
