//
//  RNAssetManager.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSETMANAGER_H_
#define __RAYNE_ASSETMANAGER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"
#include "RNAssetLoader.h"

namespace RN
{
	class AssetManager
	{
	public:
		friend class Kernel;
		friend class Asset;
		friend class AssetLoader;

		RNAPI static AssetManager *GetSharedInstance();

		template<class T>
		T *GetAssetWithName(const String *name, const Dictionary *settings)
		{
			return static_cast<T *>(__GetAssetWithName(T::GetMetaClass(), name, settings));
		}

		template<class T>
		std::shared_future<StrongRef<Asset>> GetFutureAssetWithName(const String *name, const Dictionary *settings, WorkQueue *queue = nullptr)
		{
			return __GetFutureAssetWithName(T::GetMetaClass(), name, settings, queue);
		}

		RNAPI void RegisterAssetLoader(AssetLoader *loader);
		RNAPI void UnregisterAssetLoader(AssetLoader *loader);

		RNAPI void SetDefaultQueue(WorkQueue *queue);

	private:
		AssetManager();
		~AssetManager();

		void UpdateMagicSize();

		AssetLoader *PickAssetLoader(MetaClass *base, File *file, const String *name, bool requiresBackgroundSupport);

		Asset *ValidateAsset(MetaClass *base, Asset *asset);

		void __RemoveAsset(Asset *asset, String *name);
		void __FinishLoadingAsset(void *token, Expected<Asset *> asset);
		void PrepareAsset(Asset *asset, String *name, MetaClass *meta, Dictionary *settings);

		Asset *__GetAssetMatching(MetaClass *base, String *name);
		Expected<std::shared_future<StrongRef<Asset>>> __GetFutureMatching(MetaClass *base, String *name);

		RNAPI Asset *__GetAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings);
		RNAPI std::shared_future<StrongRef<Asset>> __GetFutureAssetWithName(MetaClass *base, const String *name, const Dictionary *settings, WorkQueue *queue);

		Lockable _lock;
		Array *_loaders;
		size_t _maxMagicSize;

		Dictionary *_resources;
		Dictionary *_requests;

		WorkQueue *_defaultQueue;
	};
}

#endif /* __RAYNE_ASSETMANAGER_H_ */
