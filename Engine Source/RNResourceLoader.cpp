//
//  RNResourceLoader.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourceLoader.h"
#include "RNThreadPool.h"

namespace RN
{
	RNDefineMeta(ResourceLoader)
	
	ResourceLoader::ResourceLoader(MetaClassBase *resourceClass) :
		_magicBytes(nullptr),
		_resourceClass(resourceClass),
		_imagianryFiles(false)
	{}
	
	ResourceLoader::~ResourceLoader()
	{
		SafeRelease(_magicBytes);
	}

	
	Asset *ResourceLoader::Load(File *file, Dictionary *settings)
	{
		throw Exception(Exception::Type::InconsistencyException, "");
	}
	
	Asset *ResourceLoader::Load(String *name, Dictionary *settings)
	{
		throw Exception(Exception::Type::InconsistencyException, "");
	}
	
	
	bool ResourceLoader::SupportsBackgroundLoading()
	{
		return false;
	}
	
	std::future<Asset *> ResourceLoader::LoadInBackground(Object *fileOrName, Dictionary *settings, Tag tag, Callback callback)
	{
		ThreadPool *pool = ThreadPool::GetSharedInstance();
		
		fileOrName->Retain();
		settings->Retain();
		
		return pool->AddTaskWithFuture([=] {
			Asset *result;
			
			if(fileOrName->IsKindOfClass(File::MetaClass()))
			{
				File *file = static_cast<File *>(fileOrName);
				result = Load(file, settings);
			}
			else
			{
				String *name = static_cast<String *>(fileOrName);
				result = Load(name, settings);
			}
			
			fileOrName->Release();
			settings->Release();
			
			if(callback)
				callback(result, tag);
			
			return result;
		});
	}
	
	void ResourceLoader::SetFileExtensions(const std::vector<std::string>& extensions)
	{
		_fileExtensions = extensions;
	}
	
	void ResourceLoader::SetMagicBytes(const Data *data, size_t begin)
	{
		RN_ASSERT(data->GetLength() <= 32, "MagicBytes can have a maximum of 32 bytes only!");
		
		SafeRelease(_magicBytes);
		_magicBytes = data ? data->Copy() : nullptr;
	}
	
	void ResourceLoader::SetSupportsImaginaryFiles(bool support)
	{
		_imagianryFiles = support;
	}
	
	bool ResourceLoader::SupportsLoadingFile(File *file)
	{
		return true;
	}
	
	bool ResourceLoader::SupportsLoadingName(String *name)
	{
		return _imagianryFiles;
	}
	
	uint32 ResourceLoader::GetPriority() const
	{
		return 10;
	}
}
