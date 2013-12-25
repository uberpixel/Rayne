//
//  RNResourceCoordinator.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourceCoordinator.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNShader.h"
#include "RNLogging.h"

namespace RN
{
	RNDeclareSingleton(ResourceCoordinator)
	
	ResourceCoordinator::ResourceCoordinator()
	{}
	
	ResourceCoordinator::~ResourceCoordinator()
	{}
	
	
	void ResourceCoordinator::WaitForResources()
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		while(!_requests.empty())
		{
			std::shared_future<Object *> future = _requests.begin()->second;
			lock.Unlock();
			
			future.wait();
		}
	}
	
	void ResourceCoordinator::__AddResourceAlias(Object *object, String *name)
	{
		if(_resources.GetObjectForKey(name) || _requests.find(name) != _requests.end())
			throw Exception(Exception::Type::InconsistencyException, "");
		
		_resources.SetObjectForKey(object, name);
		
		Array *aliases = _resourceToAlias.GetObjectForKey<Array>(object);
		if(!aliases)
		{
			aliases = new Array();
			aliases->AddObject(name);
			
			_resourceToAlias.SetObjectForKey(aliases->Autorelease(), object);
			
			RNDebug("Loaded %s", name->GetUTF8String());
		}
		else
		{
			String *original = aliases->GetFirstObject<String>();
			RNDebug("Aliased %s as %s", original->GetUTF8String(), name->GetUTF8String());
			
			aliases->AddObject(name);
		}
	}
	
	
	
	void ResourceCoordinator::AddResourceAlias(Object *object, String *name)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		__AddResourceAlias(object, name);
	}
	
	void ResourceCoordinator::AddResource(Object *object, String *name)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		__AddResourceAlias(object, name);
	}
	
	void ResourceCoordinator::RemoveResource(Object *object)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		Array *aliases = _resourceToAlias.GetObjectForKey<Array>(object);
		if(aliases)
		{
			object->Retain();
			aliases->Enumerate<String>([&](String *alias, size_t index, bool *stop) {
				
				_resources.RemoveObjectForKey(alias);
				
			});
			
			_resourceToAlias.RemoveObjectForKey(object);
			object->Release();
		}
	}
	
	void ResourceCoordinator::RemoveResourceWithName(String *name)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		Object *object = _resources.GetObjectForKey(name);
		if(object)
		{
			object->Retain();
			lock.Unlock();
			
			RemoveResource(object);
			object->Release();
		}
	}
	
	
	
	Object *ResourceCoordinator::ValidateResource(MetaClassBase *base, Object *object)
	{
		if(!object->IsKindOfClass(base))
			throw Exception(Exception::Type::InconsistencyException, "");
		
		return object;
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
			
			_loader.Enumerate<ResourceLoader>([&] (ResourceLoader *loader, size_t index, bool *stop) {
				
				try
				{
					if(!base->InheritsFromClass(loader->GetResourceClass()))
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
						*stop = true;
						
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
			
			_loader.Enumerate<ResourceLoader>([&] (ResourceLoader *loader, size_t index, bool *stop) {
				
				try
				{
					if(!base->InheritsFromClass(loader->GetResourceClass()))
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
						*stop = true;
						
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
	
	
	std::shared_future<Object *> ResourceCoordinator::RequestFutureResourceWithName(MetaClassBase *base, String *name, Dictionary *settings)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		// Check if the resource is already loaded
		Object *object = _resources.GetObjectForKey(name);
		if(object)
		{
			std::promise<Object *> promise;
			std::shared_future<Object *> future = promise.get_future().share();
			
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
			std::shared_future<Object *> future(iterator->second);
			lock.Unlock();
			
			return future;
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
		
		ResourceLoader *resourceLoader = PickResourceLoader(base, file, name, true);
		
		if(!settings)
		{
			settings = new Dictionary();
			settings->Autorelease();
		}
		
		name->Retain();
		
		Object *fileOrName = file ? static_cast<Object *>(file) : static_cast<Object *>(name);
		
		std::future<Object *> future = std::move(resourceLoader->LoadInBackground(fileOrName, settings, 0, [this, name] (Object *object, Tag tag) {
			
			LockGuard<decltype(_lock)> lock(_lock);
			_requests.erase(name);
			__AddResourceAlias(object, name);
			lock.Unlock();

			name->Release();
			
		}));
		
		std::shared_future<Object *> shared = future.share();
		
		_requests.insert(decltype(_requests)::value_type(name, shared));
		lock.Unlock();
		
		return shared;
	}
	
	Object *ResourceCoordinator::RequestResourceWithName(MetaClassBase *base, String *name, Dictionary *settings)
	{
		LockGuard<decltype(_lock)> lock(_lock);
		
		// Check if the resource is already loaded
		Object *object = _resources.GetObjectForKey(name);
		if(object)
			return ValidateResource(base, object);
		
		// Check if there is a pending request for the resource
		auto iterator = _requests.find(name);
		if(iterator != _requests.end())
		{
			std::shared_future<Object *> future(iterator->second);
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
		
		ResourceLoader *resourceLoader = PickResourceLoader(base, file, name, false);
		
		std::promise<Object *> promise;
		_requests.insert(decltype(_requests)::value_type(name->Retain(), std::move(promise.get_future().share())));
		lock.Unlock();
		
		try
		{
			if(!settings)
			{
				settings = new Dictionary();
				settings->Autorelease();
			}
			
			if(file)
			{
				object = resourceLoader->Load(file, settings);
			}
			else
			{
				object = resourceLoader->Load(name, settings);
			}
			
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
		__AddResourceAlias(object, name);
		lock.Unlock();
		
		name->Release();
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
		Shader *shader = GetResourceWithName<Shader>(name, nullptr);
		AddResourceAlias(shader, key);
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
