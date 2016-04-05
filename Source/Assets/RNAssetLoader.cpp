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


	Asset *AssetLoader::Load(File *file, MetaClass *meta, Dictionary *settings)
	{
		throw NotImplementedException("Load() not implemented");
	}

	Asset *AssetLoader::Load(const String *name, MetaClass *meta, Dictionary *settings)
	{
		throw NotImplementedException("Load() not implemented");
	}

	Expected<Asset *> AssetLoader::__Load(Object *fileOrName, MetaClass *meta, Dictionary *settings) RN_NOEXCEPT
	{
		try
		{
			if(fileOrName->IsKindOfClass(File::GetMetaClass()))
			{
				File *file = static_cast<File *>(fileOrName);
				return Load(file, meta, settings);
			}
			else
			{
				String *name = static_cast<String *>(fileOrName);
				return Load(name, meta, settings);
			}
		}
		catch(...)
		{
			return std::current_exception();
		}
	}

	void AssetLoader::LoadInBackground(Object *fileOrName, MetaClass *meta, Dictionary *settings, Callback &&callback)
	{
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::High);

		fileOrName->Retain();
		settings->Retain();

		queue->Perform([=]() {

			Asset *result;

			try
			{
				AutoreleasePool::PerformBlock([&] {

					if(fileOrName->IsKindOfClass(File::GetMetaClass()))
					{
						File *file = static_cast<File *>(fileOrName);
						result = SafeRetain(Load(file, meta, settings));
					}
					else
					{
						String *name = static_cast<String *>(fileOrName);
						result = SafeRetain(Load(name, meta, settings));
					}
				});
			}
			catch(std::exception &e)
			{
				RNError("Encountered exception " << e << " while loading asset");

				callback(nullptr);

				fileOrName->Release();
				settings->Release();
			}

			callback(result);

			fileOrName->Release();
			settings->Release();

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
