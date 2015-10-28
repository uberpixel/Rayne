//
//  RNAssetCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSETCOORDINATOR_H_
#define __RAYNE_ASSETCOORDINATOR_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"
#include "RNAssetLoader.h"

namespace RN
{
	class AssetCoordinator
	{
	public:
		friend class Kernel;

		RNAPI static AssetCoordinator *GetSharedInstance();

		template<class T>
		T *GetAssetWithName(const String *name, const Dictionary *settings)
		{
			return static_cast<T *>(__GetAssetWithName(T::GetMetaClass(), name, settings));
		}

		template<class T>
		std::shared_future<Asset *> GetFutureAssetWithName(const String *name, const Dictionary *settings)
		{
			return __GetFutureAssetWithName(T::GetMetaClass(), name, settings);
		}

		RNAPI void RegisterAssetLoader(AssetLoader *loader);
		RNAPI void UnregisterAssetLoader(AssetLoader *loader);

	private:
		AssetCoordinator();
		~AssetCoordinator();

		void UpdateMagicSize();

		AssetLoader *PickAssetLoader(MetaClass *base, File *file, const String *name, bool requiresBackgroundSupport);

		Asset *ValidateAsset(MetaClass *base, Asset *asset);
		void PrepareAsset(Asset *asset, String *name, Dictionary *settings);

		std::shared_future<Asset *> __GetFutureMatching(MetaClass *base, String *name);

		RNAPI Asset *__GetAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings);
		RNAPI std::shared_future<Asset *> __GetFutureAssetWithName(MetaClass *base, const String *name, const Dictionary *settings);

		std::mutex _lock;
		Array *_loaders;
		size_t _maxMagicSize;

		std::unordered_map<StringRef, WeakRef<Asset>, std::hash<Object>, std::equal_to<Object>> _resources;
		std::unordered_map<StringRef, std::shared_future<Asset *>, std::hash<Object>, std::equal_to<Object>> _requests;
	};
}

#endif /* __RAYNE_ASSETCOORDINATOR_H_ */
