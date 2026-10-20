//
//  RNAssetManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAssetManager.h"
#include "../Debug/RNLogger.h"
#include "RNAssetManagerInternals.h"

#include "RNASTCAssetLoader.h"
#include "RNDDSAssetLoader.h"
#include "RNPNGAssetLoader.h"
#include "RNSGAAssetLoader.h"
#include "RNSGMAssetLoader.h"

namespace RN
{
	static AssetManager *__sharedInstance = nullptr;
	static RecursiveLockable __sharedInstanceLock;

	AssetManager::AssetManager() :
		_isShuttingDown(false),
		_loaders(new Array()),
		_resources(new Dictionary()),
		_requests(new Dictionary()),
		_defaultQueue(nullptr)
	{
		__sharedInstance = this;

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX
		_preferredTextureFileExtension = RNSTR("dds")->Retain();
#elif RN_PLATFORM_ANDROID || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		_preferredTextureFileExtension = RNSTR("astc")->Retain();
#else
		_preferredTextureFileExtension = RNSTR("png")->Retain();
#endif

		PNGAssetLoader::Register();
		ASTCAssetLoader::Register();
		DDSAssetLoader::Register();
		SGMAssetLoader::Register();
		SGAAssetLoader::Register();
	}
	AssetManager::~AssetManager()
	{
		CancelPendingLoads();
		ReleaseKeepAliveAssets();

		SafeRelease(_loaders);
		SafeRelease(_requests);
		SafeRelease(_resources);

		SafeRelease(_preferredTextureFileExtension);

		SafeRelease(_defaultQueue);

		__sharedInstance = nullptr;
	}

	void AssetManager::ReleaseKeepAliveAssets()
	{
		std::unordered_set<Asset *> keepAliveAssets;

		{
			LockGuard<Lockable> lock(_lock);
			_resources->Enumerate<Array, String>([&](Array *resources, const String *name, bool &stop) {
				resources->Enumerate<LoadedAsset>([&](LoadedAsset *wrapper, size_t index, bool &stop) {
					Asset *asset = wrapper->GetAsset();
					if(asset && asset->_keepAlive)
						keepAliveAssets.insert(asset);
				});
			});
		}

		for(Asset *asset : keepAliveAssets)
		{
			if(!asset->_keepAlive)
				continue;

			asset->_keepAlive = false;
			asset->Release();
		}
	}

	void AssetManager::CancelPendingLoads()
	{
		Array *pendingRequests = new Array();
		WorkQueue *defaultQueue = nullptr;

		{
			LockGuard<Lockable> lock(_lock);
			_isShuttingDown = true;

			if(_requests)
			{
				_requests->Enumerate<Array, String>([&](Array *requests, const String *name, bool &stop) {
					size_t count = requests->GetCount();
					for(size_t i = 0; i < count; i++)
					{
						PendingAsset *wrapper = requests->GetObjectAtIndex<PendingAsset>(i);
						pendingRequests->AddObject(wrapper);
					}
				});

				_requests->RemoveAllObjects();
			}

			defaultQueue = _defaultQueue;
			_defaultQueue = nullptr;
		}

		pendingRequests->Enumerate<PendingAsset>([](PendingAsset *wrapper, size_t index, bool &stop) {
			wrapper->SetAsset(nullptr);
		});
		pendingRequests->Release();

		SafeRelease(defaultQueue);
	}

	AssetManager *AssetManager::GetSharedInstance()
	{
		LockGuard<RecursiveLockable> guard(__sharedInstanceLock);
		if(!__sharedInstance)
		{
			new AssetManager(); //Will assign __sharedInstance internally, would get stuck in endless recursion otherwise
		}
		return __sharedInstance;
	}


	void AssetManager::SetDefaultQueue(WorkQueue *queue)
	{
		RN_ASSERT(queue != WorkQueue::GetMainQueue(), "The default queue can't be the main queue");

		LockGuard<Lockable> lock(_lock);

		SafeRelease(_defaultQueue);
		_defaultQueue = SafeRetain(queue);
	}


	void AssetManager::RegisterAssetLoader(AssetLoader *loader)
	{
		LockGuard<Lockable> lock(_lock);

		_loaders->AddObject(loader);
		_loaders->Sort<AssetLoader>([](const AssetLoader *loader1, const AssetLoader *loader2) -> bool {
			uint32 priority1 = loader1->GetPriority();
			uint32 priority2 = loader2->GetPriority();

			return priority1 < priority2;
		});

		UpdateMagicSize();
	}

	void AssetManager::UnregisterAssetLoader(AssetLoader *loader)
	{
		LockGuard<Lockable> lock(_lock);
		_loaders->RemoveObject(loader);

		UpdateMagicSize();
	}

	void AssetManager::SetPreferredTextureFileExtension(const String *preferredFileExtension)
	{
		SafeRelease(_preferredTextureFileExtension);
		_preferredTextureFileExtension = RNSTR(preferredFileExtension)->Retain();
	}

	const String *AssetManager::GetPreferredTextureFileExtension() const
	{
		return _preferredTextureFileExtension;
	}

	void AssetManager::UpdateMagicSize()
	{
		_maxMagicSize = 0;
		_loaders->Enumerate<AssetLoader>([&](AssetLoader *loader, size_t index, bool &stop) {
			if(loader->_magicBytes)
				_maxMagicSize = std::max(loader->_magicBytes->GetLength(), _maxMagicSize);
		});
	}


	Asset *AssetManager::ValidateAsset(MetaClass *base, Asset *asset)
	{
		if(!asset->IsKindOfClass(base))
			throw InconsistencyException(RNSTR("Failed to validate asset for class " << base->GetName()));

		return asset;
	}
	void AssetManager::PrepareAsset(Asset *asset, String *name, MetaClass *meta, Dictionary *settings)
	{
		asset->__AwakeWithCoordinator(this, name, meta);

		Array *resources = _resources->GetObjectForKey<Array>(name);

		if(!resources)
		{
			resources = new Array();
			_resources->SetObjectForKey(resources, name);
			resources->Release();
		}

		LoadedAsset *wrapper = new LoadedAsset(asset, meta);
		resources->AddObject(wrapper);
		wrapper->Release();

		RNDebug("Loaded asset " << asset);
	}
	void AssetManager::__RemoveAsset(Asset *asset, String *name)
	{
		{
			LockGuard<Lockable> lock(_lock);

			Array *resources = _resources->GetObjectForKey<Array>(name);
			size_t count = resources->GetCount();

			for(size_t i = 0; i < count; i++)
			{
				LoadedAsset *wrapper = static_cast<LoadedAsset *>(resources->GetObjectAtIndex(i));

				if(wrapper->GetMeta() == asset->_meta)
				{
					resources->RemoveObjectAtIndex(i);
					break;
				}
			}

			if(resources->GetCount() == 0)
				_resources->RemoveObjectForKey(name);
		}

		RNDebug("Unloaded asset " << asset);
	}


	Asset *AssetManager::__GetAssetMatching(MetaClass *base, String *name)
	{
		Asset *asset = nullptr;

		{
			Array *resources = _resources->GetObjectForKey<Array>(name);
			if(resources)
			{
				size_t count = resources->GetCount();

				for(size_t i = 0; i < count; i++)
				{
					LoadedAsset *wrapper = static_cast<LoadedAsset *>(resources->GetObjectAtIndex(i));

					if(base->InheritsFromClass(wrapper->GetMeta()))
					{
						asset = wrapper->GetAsset();
						if(!asset)
							resources->RemoveObjectAtIndex(i);

						break;
					}
				}
			}
		}

		return asset;
	}

	AssetLoadFuture AssetManager::__GetFutureMatching(MetaClass *base, String *name)
	{
		// Check if there is a pending request for the resource
		Array *requests = _requests->GetObjectForKey<Array>(name);
		if(requests)
		{
			size_t count = requests->GetCount();

			for(size_t i = 0; i < count; i++)
			{
				PendingAsset *wrapper = static_cast<PendingAsset *>(requests->GetObjectAtIndex(i));

				if(base->InheritsFromClass(wrapper->GetMeta()))
					return wrapper->GetFuture();
			}
		}

		return AssetLoadFuture();
	}

	Asset *AssetManager::__GetAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings)
	{
		String *name = tname->GetNormalizedPath()->Retain();
		UniqueLock<Lockable> lock(_lock);

		if(_isShuttingDown)
		{
			lock.Unlock();
			name->Autorelease();
			throw InconsistencyException(RNSTR("Can't load resource " << name << " while the AssetManager is shutting down"));
		}

		{
			Asset *asset = __GetAssetMatching(base, name);
			if(asset)
			{
				asset->Retain();
				lock.Unlock();

				name->Release();
				return asset->Autorelease();
			}
		}

		AssetLoadFuture future = __GetFutureMatching(base, name);

		if(future.valid())
		{
			lock.Unlock();

			WorkQueue *queue = WorkQueue::GetCurrentWorkQueue();

			if(queue)
				queue->YieldWithFuture(future);

			StrongRef<Asset> pendingAsset = future.get();
			if(!pendingAsset)
			{
				name->Autorelease();
				throw InconsistencyException(RNSTR("Failed loading resource " << name));
			}
			name->Release();
			pendingAsset->Retain();
			return pendingAsset->Autorelease();
		}

		RNDebug("Loading asset " << name);

		// Load the resource
		File *file;
		Dictionary *settings = SafeCopy(tsettings);

		try
		{
			file = File::WithName(name, File::Mode::Read);
		}
		catch(Exception &exception)
		{
			RNWarning("Failed opening asset " << name << ": " << exception.GetReason());
			file = nullptr;
		}

		AssetLoader *loader = PickAssetLoader(base, file, name, true);

		PendingAsset *wrapper;

		{
			Array *requests = _requests->GetObjectForKey<Array>(name);
			if(!requests)
			{
				requests = new Array();
				_requests->SetObjectForKey(requests, name);
				requests->Release();
			}

			wrapper = new PendingAsset(base, name);
			requests->AddObject(wrapper);
			// Keep the pending wrapper alive while the lock is released for the synchronous load.
			wrapper->Retain();
			wrapper->Release();
		}

		lock.Unlock();

		if(!settings)
			settings = new Dictionary();

		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);
		AssetLoader::LoadOptions options;
		options.settings = settings;
		options.queue = nullptr;
		options.meta = base;

		Expected<Asset *> asset = loader->__Load(fileOrName, options);

		settings->Release();

		lock.Lock();

		bool shouldPublishAsset = !_isShuttingDown;

		{
			Array *requests = _requests->GetObjectForKey<Array>(name);

			if(requests)
			{
				requests->RemoveObject(wrapper);

				if(requests->GetCount() == 0)
					_requests->RemoveObjectForKey(name);
			}
		}

		if(!asset.IsValid())
		{
			lock.Unlock();
			RNWarning("Failed loading resource: " << name);
			wrapper->SetAsset(nullptr);
			wrapper->Release();
			name->Release();
			std::rethrow_exception(asset.GetException());
		}

		Asset *result = asset.Get();

		// Send the result out
		result->Retain();

		if(shouldPublishAsset)
			PrepareAsset(result, name, base, settings);
		lock.Unlock();

		wrapper->SetAsset(shouldPublishAsset ? result : nullptr);
		wrapper->Release();

		if(!shouldPublishAsset)
		{
			result->Release();
			name->Autorelease();
			throw InconsistencyException(RNSTR("Finished loading resource " << name << " while the AssetManager is shutting down"));
		}

		name->Release();
		return result->Autorelease();
	}

	AssetLoadFuture AssetManager::__GetFutureAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings, WorkQueue *queue)
	{
		String *name = tname->GetNormalizedPath()->Retain();
		UniqueLock<Lockable> lock(_lock);

		if(_isShuttingDown)
		{
			lock.Unlock();
			name->Release();

			std::promise<StrongRef<Asset>> promise;
			AssetLoadFuture future = promise.get_future().share();
			promise.set_value(StrongRef<Asset>());

			return future;
		}

		Asset *asset = __GetAssetMatching(base, name);
		if(asset)
		{
			asset->Retain();
			lock.Unlock();

			std::promise<StrongRef<Asset>> promise;
			AssetLoadFuture future = promise.get_future().share();

			try
			{
				asset = ValidateAsset(base, asset);
				promise.set_value(StrongRef<Asset>(asset));
			}
			catch(Exception &)
			{
				promise.set_value(StrongRef<Asset>());
			}

			asset->Release();
			name->Release();

			return future;
		}

		AssetLoadFuture future = __GetFutureMatching(base, name);
		if(future.valid())
		{
			name->Release();

			return future;
		}

		// Load the resource
		File *file;
		Dictionary *settings = SafeCopy(tsettings);

		try
		{
			file = File::WithName(name, File::Mode::Read);
		}
		catch(Exception &exception)
		{
			RNWarning("Failed opening asset " << name << ": " << exception.GetReason());
			file = nullptr;
		}

		AssetLoader *loader = PickAssetLoader(base, file, name, true);

		if(!settings)
			settings = new Dictionary();

		PendingAsset *wrapper = new PendingAsset(base, name);
		Array *requests = _requests->GetObjectForKey<Array>(name);
		if(!requests)
		{
			requests = new Array();
			_requests->SetObjectForKey(requests, name);
			requests->Release();
		}

		requests->AddObject(wrapper);
		future = wrapper->GetFuture();

		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);

		AssetLoader::LoadOptions options;
		options.settings = settings;
		options.queue = queue ? queue : _defaultQueue;
		options.meta = base;

		if(options.queue == nullptr)
			options.queue = WorkQueue::GetCurrentWorkQueue();

		if(options.queue == nullptr)
			options.queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		wrapper->Retain();
		loader->__LoadInBackground(fileOrName, options, wrapper);
		settings->Release();
		wrapper->Release();
		name->Release();

		return future;
	}

	void AssetManager::__FinishLoadingAsset(void *token, Asset *asset)
	{
		PendingAsset *wrapper = reinterpret_cast<PendingAsset *>(token);
		String *name = wrapper->GetName();
		bool shouldPublishAsset;

		{
			LockGuard<Lockable> lock(_lock);

			shouldPublishAsset = !_isShuttingDown;

			if(shouldPublishAsset && asset)
				PrepareAsset(asset, name, wrapper->GetMeta(), nullptr);

			Array *requests = _requests ? _requests->GetObjectForKey<Array>(name) : nullptr;

			if(requests)
			{
				requests->RemoveObject(wrapper);

				if(requests->GetCount() == 0)
					_requests->RemoveObjectForKey(name);
			}
		}

		wrapper->SetAsset(shouldPublishAsset ? asset : nullptr);
		wrapper->Release();
	}

	AssetLoader *AssetManager::PickAssetLoader(MetaClass *base, File *file, const String *name, bool requiresBackgroundSupport)
	{
		AssetLoader *assetLoader = nullptr;

		if(file)
		{
			uint8 *buffer = (_maxMagicSize > 0) ? new uint8[_maxMagicSize] : nullptr;
			uint8 *magic = (_maxMagicSize > 0) ? new uint8[_maxMagicSize] : nullptr;

			String *extension = file->GetPath()->GetPathExtension();

			size_t magicSize = 0;

			if(buffer)
			{
				magicSize = file->Read(magic, _maxMagicSize);
				file->Seek(0);
			}

			_loaders->Enumerate<AssetLoader>([&](AssetLoader *loader, size_t index, bool &stop) {
				try
				{
					if(!loader->SupportsResourceClass(base))
						return;

					if(requiresBackgroundSupport && !loader->_supportsBackgroundLoading)
						return;

					if(loader->_fileExtensions->GetCount() > 0 && !loader->_fileExtensions->ContainsObject(extension))
						return;

					if(loader->_magicBytes && loader->_magicBytes->GetLength() > 0)
					{
						size_t offset = loader->_magicBytesOffset;
						size_t size = loader->_magicBytes->GetLength();

						if(size > magicSize)
							return;

						if(offset > 0)
						{
							file->Seek(offset, true);
							size = file->Read(buffer, size);
							file->Seek(0, true);
						}
						else
						{
							std::copy(magic, magic + size, buffer);
						}

						if(memcmp(buffer, loader->_magicBytes->GetBytes(), size) != 0)
							return;
					}

					bool result = loader->SupportsLoadingFile(file);
					file->Seek(0, true);

					if(result)
					{
						assetLoader = loader;
						stop = true;

						return;
					}
				}
				catch(Exception &)
				{
					file->Seek(0, true);
				}
			});

			if(buffer) delete[] buffer;
			if(magic) delete[] magic;
		}
		else if(name)
		{
			String *extension = name->GetPathExtension();

			_loaders->Enumerate<AssetLoader>([&](AssetLoader *loader, size_t index, bool &stop) {
				try
				{
					if(!loader->SupportsResourceClass(base))
						return;

					if(requiresBackgroundSupport && !loader->_supportsBackgroundLoading)
						return;

					if(!loader->_supportsVirtualFiles)
						return;

					if(extension && loader->_fileExtensions->GetCount() > 0 && !loader->_fileExtensions->ContainsObject(extension))
						return;

					if(loader->SupportsLoadingName(name))
					{
						assetLoader = loader;
						stop = true;

						return;
					}
				}
				catch(Exception &)
				{}
			});
		}

		if(!assetLoader)
		{
			if(file)
				throw InconsistencyException(RNSTR("No resource loader matching " << name << " (" << file->GetPath() << ")"));

			throw InconsistencyException(RNSTR("No resource loader matching " << name << " (couldn't resolve name to a path)"));
		}

		return assetLoader;
	}
} // namespace RN
