//
//  RNAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Objects/RNAutoreleasePool.h"
#include "../Threads/RNWorkQueue.h"
#include "../Debug/RNLogger.h"
#include "RNAssetLoader.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(AssetLoader, Object)

	AssetLoader::AssetLoader(const Config &config) :
		_magicBytes(nullptr),
		_magicBytesOffset(0),
		_fileExtensions(nullptr),
		_resourceClasses(config.resourceClasses),
		_supportsBackgroundLoading(config.supportsBackgroundLoading),
		_supportsVirtualFiles(config.supportsVirtualFiles),
		_priority(config.priority)
	{
		_magicBytes = SafeRetain(config._magicBytes);
		_magicBytesOffset = config._magicBytesOffset;
		_fileExtensions = SafeRetain(config._extensions);

		for(MetaClass *meta : _resourceClasses)
		{
			RN_ASSERT(meta->InheritsFromClass(Asset::GetMetaClass()), "AssetLoader must support loading Asset subclasses only");
		}
	}

	AssetLoader::~AssetLoader()
	{
		SafeRelease(_magicBytes);
		SafeRelease(_fileExtensions);
	}


	Asset *AssetLoader::Load(File *file, const LoadOptions &options)
	{
		throw NotImplementedException("Load(File *, const LoadOptions &) not implemented");
	}

	Asset *AssetLoader::Load(const String *name, const LoadOptions &options)
	{
		throw NotImplementedException("Load(const String *, const LoadOptions &) not implemented");
	}

	Expected<Asset *> AssetLoader::__Load(Object *fileOrName, const LoadOptions &options) RN_NOEXCEPT
	{
		try
		{
			if(fileOrName->IsKindOfClass(File::GetMetaClass()))
			{
				File *file = static_cast<File *>(fileOrName);
				return Load(file, options);
			}
			else
			{
				String *name = static_cast<String *>(fileOrName);
				return Load(name, options);
			}
		}
		catch(...)
		{
			return std::current_exception();
		}
	}

	void AssetLoader::__LoadInBackground(Object *fileOrName, const LoadOptions &options, void *token)
	{
		fileOrName->Retain();

		options.queue->Perform([=]() {

			Asset *result;

			try
			{
				AutoreleasePool::PerformBlock([&] {

					if(fileOrName->IsKindOfClass(File::GetMetaClass()))
					{
						File *file = static_cast<File *>(fileOrName);
						result = SafeRetain(Load(file, options));
					}
					else
					{
						String *name = static_cast<String *>(fileOrName);
						result = SafeRetain(Load(name, options));
					}
				});
			}
			catch(std::exception &e)
			{
				RNError("Encountered exception " << e << " while loading asset");

				AssetManager *manager = AssetManager::GetSharedInstance();
				manager->__FinishLoadingAsset(token, e);

				fileOrName->Release();
			}

			AssetManager *manager = AssetManager::GetSharedInstance();
			manager->__FinishLoadingAsset(token, result);

			fileOrName->Release();

		});
	}

	bool AssetLoader::SupportsLoadingFile(File *file) const
	{
		return true;
	}

	bool AssetLoader::SupportsLoadingName(const String *name) const
	{
		return _supportsVirtualFiles;
	}

	bool AssetLoader::SupportsResourceClass(MetaClass *meta) const
	{
		for(auto tmeta : _resourceClasses)
		{
			if(meta->InheritsFromClass(tmeta))
				return true;
		}

		return false;
	}
}
