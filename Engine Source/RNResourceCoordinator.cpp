//
//  RNResourceCoordinator.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourceCoordinator.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNShader.h"
#include "RNLogging.h"

namespace RN
{
	RNDefineSingleton(ResourceCoordinator)
	
	ResourceCoordinator::ResourceCoordinator()
	{}
	
	ResourceCoordinator::~ResourceCoordinator()
	{}
	
	
	void ResourceCoordinator::WaitForResources()
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		while(!_requests.empty())
		{
			std::shared_future<Asset *> future = _requests.begin()->second;
			lock.Unlock();
			
			future.wait();
		}
	}
	
	void ResourceCoordinator::RemoveResource(Asset *object)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		for(auto i = _resources.begin(); i != _resources.end(); i ++)
		{
			Asset *asset = i->second;
			
			if(asset == object)
			{
				RNDebug("Removed asset %s", asset->GetName().c_str());
				
				_resources.erase(i);
				return;
			}
		}
	}
	
	
	
	Asset *ResourceCoordinator::ValidateResource(MetaClassBase *base, Asset *object)
	{
		if(!object->IsKindOfClass(base))
			throw Exception(Exception::Type::InconsistencyException, "Failed to validate asset for class %s", base->Name().c_str());
		
		return object;
	}
	
	void ResourceCoordinator::PrepareResource(Asset *object, String *name, Dictionary *settings)
	{
		object->WakeUpFromResourceCoordinator(name->GetUTF8String(), settings);
		object->_signal.Connect(std::bind(&ResourceCoordinator::RemoveResource, this, std::placeholders::_1));
		
		_resources[name->Retain()] = object;
		
		RNDebug("Added asset %s", name->GetUTF8String());
	}
	
	
	ResourceLoader *ResourceCoordinator::PickResourceLoader(MetaClassBase *base, File *file, String *name, bool requiresBackgroundSupport)
	{
		ResourceLoader *resourceLoader = nullptr;
		
		if(file)
		{
			uint8 buffer[32];
			uint8 magic[32];
			
			std::string extension = file->GetExtension();
			
			file->ReadIntoBuffer(magic, 32);
			file->Seek(0, true);
			
			_loader.Enumerate<ResourceLoader>([&] (ResourceLoader *loader, size_t index, bool &stop) {
				
				try
				{
					if(!loader->GetResourceClass()->InheritsFromClass(base))
						return;
					
					if(requiresBackgroundSupport && !loader->SupportsBackgroundLoading())
						return;
					
					if(!loader->_fileExtensions.empty())
					{
						std::vector<std::string> &extensions = loader->_fileExtensions;
						
						if(std::find(extensions.begin(), extensions.end(), extension) == extensions.end())
							return;
					}
					
					if(loader->_magicBytes)
					{
						size_t offset = loader->_magicBytesOffset;
						size_t size = loader->_magicBytes->GetLength();
						
						if(offset > 0)
						{
							file->Seek(offset, true);
							file->ReadIntoBuffer(buffer, size);
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
						resourceLoader = loader;
						stop = true;
						
						return;
					}
				}
				catch(Exception e)
				{
					file->Seek(0, true);
				}
				
			});
		}
		else if(name)
		{
			std::string extension = PathManager::Extension(name->GetUTF8String());
			
			_loader.Enumerate<ResourceLoader>([&] (ResourceLoader *loader, size_t index, bool &stop) {
				
				try
				{
					if(!loader->GetResourceClass()->InheritsFromClass(base))
						return;
					
					if(requiresBackgroundSupport && !loader->SupportsBackgroundLoading())
						return;
					
					if(!loader->_imagianryFiles)
						return;
					
					if(!loader->_fileExtensions.empty())
					{
						std::vector<std::string> &extensions = loader->_fileExtensions;
						
						if(std::find(extensions.begin(), extensions.end(), extension) == extensions.end())
							return;
					}
					
					if(loader->SupportsLoadingName(name))
					{
						resourceLoader = loader;
						stop = true;
						
						return;
					}
				}
				catch(Exception e)
				{}
				
			});
		}
		
		if(!resourceLoader)
			throw Exception(Exception::Type::InconsistencyException, std::string("No resource loader matching ") + name->GetUTF8String());
		
		return resourceLoader;
	}
	
	
	
	std::shared_future<Asset *> ResourceCoordinator::RequestFutureResourceWithName(MetaClassBase *base, String *name, Dictionary *settings)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		// Check if the resource is already loaded
		Asset *object = nullptr;
		
		{
			auto iterator = _resources.find(name);
			if(iterator != _resources.end())
				object = iterator->second;
		}
		
		if(object)
		{
			std::promise<Asset *> promise;
			std::shared_future<Asset *> future = promise.get_future().share();
			
			try
			{
				object = ValidateResource(base, object);
				promise.set_value(object);
			}
			catch(Exception e)
			{
				promise.set_exception(std::current_exception());
			}
			
			return future;
		}
		
		// Check if there is a pending request for the resource
		auto iterator = _requests.find(name);
		if(iterator != _requests.end())
		{
			std::shared_future<Asset *> future(iterator->second);
			lock.Unlock();
			
			return future;
		}
		
		// Load the resource
		File *file = nullptr;
		name = name->Copy();
		settings = settings ? settings->Copy() : settings;
		
		try
		{
			file = new File(name->GetUTF8String());
			file->Autorelease();
		}
		catch(Exception e)
		{}
		
		ResourceLoader *resourceLoader = PickResourceLoader(base, file, name, true);
		
		if(!settings)
		{
			settings = new Dictionary();
			settings->Autorelease();
		}
		
		
		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);
		
		std::future<Asset *> future = std::move(resourceLoader->LoadInBackground(fileOrName, settings, 0, [this, name, settings] (Asset *object, Tag tag) {
			
			LockGuard<decltype(_lock)> lock(_lock);
			_requests.erase(name);
			PrepareResource(object, name, settings);
			lock.Unlock();

			name->Release();
			if(settings)
				settings->Release();
			
		}));
		
		std::shared_future<Asset *> shared = future.share();
		_requests.emplace(name, shared);
		lock.Unlock();
		
		return shared;
	}
	
	Asset *ResourceCoordinator::RequestResourceWithName(MetaClassBase *base, String *name, Dictionary *settings)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		// Check if the resource is already loaded
		Asset *object = nullptr;
		
		{
			auto iterator = _resources.find(name);
			if(iterator != _resources.end())
				object = iterator->second;
		}
		
		if(object)
			return ValidateResource(base, object);
		
		// Check if there is a pending request for the resource
		auto iterator = _requests.find(name);
		if(iterator != _requests.end())
		{
			std::shared_future<Asset *> future(iterator->second);
			lock.Unlock();
			
			future.wait();
			return ValidateResource(base, future.get());
		}
		
		
		// Load the resource
		File *file = nullptr;
		
		try
		{
			file = new File(name->GetUTF8String());
			file->Autorelease();
		}
		catch(Exception e)
		{}
		
		name = name->Copy();
		settings = settings ? settings->Copy() : settings;
		
		ResourceLoader *resourceLoader = PickResourceLoader(base, file, name, false);
		
		std::promise<Asset *> promise;
		_requests.emplace(name, std::move(promise.get_future().share()));
		lock.Unlock();
		
		try
		{
			if(!settings)
				settings = (new Dictionary())->Autorelease();
			
			object = (file) ? resourceLoader->Load(file, settings) : resourceLoader->Load(name, settings);
			object->WakeUpFromResourceCoordinator(name->GetUTF8String(), settings);
			
			promise.set_value(object);
		}
		catch(Exception e)
		{
			promise.set_exception(std::current_exception());
			
			lock.Lock();
			_requests.erase(name);
			lock.Unlock();
			
			name->Release();
			throw e;
		}
		
		lock.Lock();
		
		_requests.erase(name);
		PrepareResource(object, name, settings);
		
		lock.Unlock();
		
		name->Release();
		SafeRelease(settings);
		
		return object->Autorelease();
	}
	
	void ResourceCoordinator::RegisterResourceLoader(ResourceLoader *loader)
	{
		_lock.Lock();
		
		_loader.AddObject(loader);
		_loader.Sort<ResourceLoader>([] (const ResourceLoader *loader1, const ResourceLoader *loader2) -> ComparisonResult {
			
			uint32 priority1 = loader1->GetPriority();
			uint32 priority2 = loader2->GetPriority();
			
			if(priority1 > priority2)
				return ComparisonResult::GreaterThan;
			
			if(priority1 < priority2)
				return ComparisonResult::LessThan;
			
			return ComparisonResult::EqualTo;
		});
		
		_lock.Unlock();
	}
	
	void ResourceCoordinator::UnregisterResourceLoader(ResourceLoader *loader)
	{
		_lock.Lock();
		_loader.RemoveObject(loader);
		_lock.Unlock();
	}
	
	
	void ResourceCoordinator::LoadShader(String *name, String *key)
	{
		Shader *shader = GetResourceWithName<Shader>(name, nullptr)->Retain();
		_resources.emplace(key->Copy(), shader);
	}
	
	void ResourceCoordinator::LoadEngineResources()
	{
		LoadShader(RNCSTR("shader/rn_Texture1"), kRNResourceKeyTexture1Shader);
		LoadShader(RNCSTR("shader/rn_Water"), kRNResourceKeyWaterShader);
		LoadShader(RNCSTR("shader/rn_Particle"), kRNResourceKeyParticleShader);
		
		LoadShader(RNCSTR("shader/rn_UIImage"), kRNResourceKeyUIImageShader);
		LoadShader(RNCSTR("shader/rn_UIText"), kRNResourceKeyUITextShader);
		
		LoadShader(RNCSTR("shader/rn_LightTileSampleFirst"), kRNResourceKeyLightTileSampleFirstShader);
		LoadShader(RNCSTR("shader/rn_LightTileSample"), kRNResourceKeyLightTileSampleShader);
		LoadShader(RNCSTR("shader/rn_LightDepth"), kRNResourceKeyLightDepthShader);
		LoadShader(RNCSTR("shader/rn_ShadowDepthSingle"), kRNResourceKeyDirectionalShadowDepthShader);
		LoadShader(RNCSTR("shader/rn_ShadowDepthCube"), kRNResourceKeyPointShadowDepthShader);
		
		LoadShader(RNCSTR("shader/rn_DrawFramebuffer"), kRNResourceKeyDrawFramebufferShader);
	}
}
