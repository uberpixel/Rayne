//
//  RNAssetCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Data/RNRRef.h"
#include "../Debug/RNLogger.h"
#include "RNAssetCoordinator.h"

#include "RNPNGAssetLoader.h"

namespace RN
{
	static AssetCoordinator *__sharedInstance = nullptr;

	AssetCoordinator::AssetCoordinator() :
		_loaders(new Array())
	{
		__sharedInstance = this;

		PNGAssetLoader::Register();
	}
	AssetCoordinator::~AssetCoordinator()
	{
		SafeRelease(_loaders);
		__sharedInstance = nullptr;
	}

	AssetCoordinator *AssetCoordinator::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void AssetCoordinator::RegisterAssetLoader(AssetLoader *loader)
	{
		std::lock_guard<std::mutex> lock(_lock);

		_loaders->AddObject(loader);
		_loaders->Sort<AssetLoader>([] (const AssetLoader *loader1, const AssetLoader *loader2) -> ComparisonResult {

			uint32 priority1 = loader1->GetPriority();
			uint32 priority2 = loader2->GetPriority();

			if(priority1 > priority2)
				return ComparisonResult::GreaterThan;

			if(priority1 < priority2)
				return ComparisonResult::LessThan;

			return ComparisonResult::EqualTo;
		});

		UpdateMagicSize();
	}

	void AssetCoordinator::UnregisterAssetLoader(AssetLoader *loader)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_loaders->RemoveObject(loader);

		UpdateMagicSize();
	}

	void AssetCoordinator::UpdateMagicSize()
	{
		_maxMagicSize = 0;
		_loaders->Enumerate<AssetLoader>([&](AssetLoader *loader, size_t index, bool &stop) {

			if(loader->_magicBytes)
				_maxMagicSize = std::max(loader->_magicBytes->GetLength(), _maxMagicSize);

		});
	}



	Asset *AssetCoordinator::ValidateAsset(MetaClass *base, Asset *asset)
	{
		if(!asset->IsKindOfClass(base))
			throw InconsistencyException(RNSTR("Failed to validate asset for class " << base->GetName()));

		return asset;
	}
	void AssetCoordinator::PrepareAsset(Asset *asset, String *name, MetaClass *meta, Dictionary *settings)
	{
		asset->__AwakeWithCoordinator(this, name, meta);

		auto &vector = _resources[name];
		vector.emplace_back(asset, meta);

		RNDebug("Loaded asset " << asset);
	}
	void AssetCoordinator::__RemoveAsset(Asset *asset, String *name)
	{
		{
			std::unique_lock<std::mutex> lock(_lock);

			auto &vector = _resources[name];

			for(auto iterator = vector.begin(); iterator != vector.end(); iterator ++)
			{
				if(iterator->meta == asset->_meta)
				{
					vector.erase(iterator);
					break;
				}
			}

			if(vector.empty())
				_resources.erase(name);
		}

		RNDebug("Unloaded asset " << asset);
	}


	std::shared_future<Asset *> AssetCoordinator::__GetFutureMatching(MetaClass *base, String *name)
	{
		// Check if the resource is already loaded
		Asset *asset = nullptr;

		{
			auto iterator = _resources.find(name);
			if(iterator != _resources.end())
			{
				auto &vector = iterator->second;

				for(auto &entry : vector)
				{
					if(base->InheritsFromClass(entry.meta))
					{
						asset = entry.asset.Load();
						if(!asset)
							_resources.erase(iterator);

						goto foundEntry;
					}
				}
			}

		foundEntry:;
		}

		if(asset)
		{
			std::promise<Asset *> promise;
			std::shared_future<Asset *> future = promise.get_future().share();

			try
			{
				asset = ValidateAsset(base, asset);
				promise.set_value(asset);
			}
			catch(Exception &e)
			{
				promise.set_exception(std::current_exception());
			}

			return future;
		}

		// Check if there is a pending request for the resource
		auto iterator = _requests.find(name);
		if(iterator != _requests.end())
		{
			auto &vector = iterator->second;

			for(auto &entry : vector)
			{
				if(base->InheritsFromClass(entry.meta))
					return entry.future;
			}
		}

		throw InconsistencyException("No matching future found");
	}

	Asset *AssetCoordinator::__GetAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings)
	{
		String *name = tname->Copy();
		std::unique_lock<std::mutex> lock(_lock);

		try
		{
			std::shared_future<Asset *> future = __GetFutureMatching(base, name);
			name->Release();

			future.wait();
			return future.get();
		}
		catch(InconsistencyException &e)
		{}

		// Load the resource
		File *file;
		Dictionary *settings = SafeCopy(tsettings);

		try
		{
			file = File::WithName(name, File::Mode::Read);
		}
		catch(Exception &e)
		{
			file = nullptr;
		}

		AssetLoader *loader = PickAssetLoader(base, file, name, true);

		std::promise<Asset *> promise;

		std::vector<PendingAsset> vector;
		vector.emplace_back(std::move(promise.get_future().share()), base);

		_requests.emplace(name, std::move(vector));

		lock.unlock();


		if(!settings)
			settings = new Dictionary();

		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);
		Expected<Asset *> asset = loader->__Load(fileOrName, base, settings);

		name->Release();
		settings->Release();

		lock.lock();
		_requests.erase(name);

		if(!asset.IsValid())
		{
			promise.set_exception(asset.GetException());
			std::rethrow_exception(asset.GetException());
		}

		PrepareAsset(asset.Get(), name, base, settings);
		lock.unlock();

		return asset.Get();
	}

	std::shared_future<Asset *> AssetCoordinator::__GetFutureAssetWithName(MetaClass *base, const String *tname, const Dictionary *tsettings)
	{
		String *name = tname->Copy();
		std::unique_lock<std::mutex> lock(_lock);

		try
		{
			std::shared_future<Asset *> future = __GetFutureMatching(base, name);
			name->Release();

			return future;
		}
		catch(InconsistencyException &e)
		{}

		// Load the resource
		File *file;
		Dictionary *settings = SafeCopy(tsettings);

		try
		{
			file = File::WithName(name, File::Mode::Read);
		}
		catch(Exception &e)
		{
			file = nullptr;
		}

		AssetLoader *loader = PickAssetLoader(base, file, name, true);

		if(!settings)
			settings = new Dictionary();

		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);

		std::future<Asset *> future = std::move(loader->LoadInBackground(fileOrName, base, settings, [this, name, settings, base](Asset *result) {

			std::unique_lock<std::mutex> lock(_lock);

			_requests.erase(name);

			if(!result)
			{
				lock.unlock();

				name->Release();
				settings->Release();

				return;
			}

			PrepareAsset(result, name, base, settings);
			lock.unlock();

			name->Release();
			settings->Release();

		}));

		std::shared_future<Asset *> shared = future.share();

		auto &vector = _requests[name];
		vector.emplace_back(std::move(shared), base);

		return shared;
	}


	AssetLoader *AssetCoordinator::PickAssetLoader(MetaClass *base, File *file, const String *name, bool requiresBackgroundSupport)
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

					if(loader->_magicBytes)
					{
						size_t offset = loader->_magicBytesOffset;
						size_t size = loader->_magicBytes->GetLength();

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
				catch(Exception &e)
				{
					file->Seek(0, true);
				}

			});

			delete[] buffer;
			delete[] magic;
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
				catch(Exception &e)
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
}
