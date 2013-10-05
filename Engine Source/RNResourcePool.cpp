//
//  RNResourcePool.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourcePool.h"
#include "RNPathManager.h"
#include "RNJSONSerialization.h"

#include "RNModel.h"
#include "RNTexture.h"
#include "RNShader.h"
#include "RNUIFont.h"

namespace RN
{
	ResourcePool::ResourcePool()
	{
		try
		{
			Data *data = Data::WithContentsOfFile("resources.json");
			_sections = JSONSerialization::JSONObjectFromData(data)->Downcast<Dictionary>();
			_sections->Retain();
		}
		catch(Exception e)
		{
			_sections = new Dictionary();
		}
		
		AddResourceLoader(new CallbackResourceLoader([](String *file, Dictionary *info) -> Object * {
			return Model::WithFile(file->GetUTF8String());
		}), "sgm");
		
		AddResourceLoader(new CallbackResourceLoader([](String *file, Dictionary *info) -> Object * {
			return Texture::WithFile(file->GetUTF8String());
		}), "png");
	}
	
	ResourcePool::~ResourcePool()
	{
		_sections->Release();
	}
	
	
	void ResourcePool::LoadDefaultResources(ThreadPool::Batch *batch)
	{
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Texture1", kRNResourceKeyTexture1Shader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Water", kRNResourceKeyWaterShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Particle", kRNResourceKeyParticleShader));

		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_UIImage", kRNResourceKeyUIImageShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_UIText", kRNResourceKeyUITextShader));

		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightTileSampleFirst", kRNResourceKeyLightTileSampleFirstShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightTileSample", kRNResourceKeyLightTileSampleShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightDepth", kRNResourceKeyLightDepthShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_ShadowDepthSingle", kRNResourceKeyDirectionalShadowDepthShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_ShadowDepthCube", kRNResourceKeyPointShadowDepthShader));
		
		// Must be done on this thread and blocking because there are cameras created that require this shader
		LoadShader("shader/rn_DrawFramebuffer", kRNResourceKeyDrawFramebufferShader);
	}
	
	void ResourcePool::LoadShader(const std::string& name, String *key)
	{
		Shader *shader = Shader::WithFile(name);
		AddResource(shader, key);
	}
	
	void ResourcePool::LoadFont(const std::string& name, float size, uint32 traits, String *key)
	{
		UI::FontDescriptor descriptor;
		descriptor.style = traits;
		descriptor.mipMaps = true;
		descriptor.filtering = true;
		
		UI::Font *font = UI::Font::WithNameAndDescriptor(name, size, descriptor);
		AddResource(font, key);
	}
	
	
	
	void ResourcePool::AddResource(Object *resource, String *key)
	{
		_lock.Lock();
		_objects.SetObjectForKey(resource, key);
		_lock.Unlock();
	}
	
	void ResourcePool::RemoveResource(String *key)
	{
		_lock.Lock();
		_objects.RemoveObjectForKey(key);
		_lock.Unlock();
	}
	
	void ResourcePool::__RemoveResource(String *key)
	{
		_objects.RemoveObjectForKey(key);
	}
	
	
	
	void ResourcePool::AddResourceLoader(ResourceLoader *loader, const std::string& extension)
	{
		_lock.Lock();
		_loader[extension] = loader;
		_lock.Unlock();
	}
	
	void ResourcePool::RemoveResourceLoader(const std::string& extension)
	{
		_lock.Lock();
		_loader.erase(extension);
		_lock.Unlock();
	}
	
	
	void ResourcePool::LoadResource(String *file, String *key, Dictionary *info)
	{
		std::string path = file->GetUTF8String();
		std::string extension = PathManager::Extension(path);
		
		_lock.Lock();
		auto iterator = _loader.find(extension);
		ResourceLoader *loader = (iterator != _loader.end()) ? iterator->second : nullptr;
		_lock.Unlock();
		
		if(loader)
		{
			Object *resource = loader->LoadResource(file, info);
			
			if(resource)
				AddResource(resource, key);
		}
	}
	
	void ResourcePool::LoadResourceSection(String *name)
	{
		Array *section = _sections->GetObjectForKey<Array>(name);
		ThreadPool::Batch *batch = ThreadPool::GetSharedInstance()->CreateBatch();
		
		if(section)
		{
			section->Enumerate([&](Object *value, size_t index, bool *stop) {
				try
				{
					Dictionary *object = value->Downcast<Dictionary>();
					String *key = object->GetObjectForKey<String>(RNCSTR("key"));
					String *file = object->GetObjectForKey<String>(RNCSTR("file"));
					
					if(key && file)
						batch->AddTask(std::bind(&ResourcePool::LoadResource, this, file, key, object));
				}
				catch(Exception e)
				{}
			});
		}
		
		batch->Commit();
		batch->Wait();
		batch->Release();
	}
	
	void ResourcePool::UnloadResourceSection(String *name)
	{
		_lock.Lock();
		
		AutoreleasePool pool;
		Array *section = _sections->GetObjectForKey<Array>(name);
		
		if(section)
		{
			section->Enumerate([&](Object *value, size_t index, bool *stop) {
				try
				{
					Dictionary *object = value->Downcast<Dictionary>();
					String *key = object->GetObjectForKey<String>(RNCSTR("key"));
					
					if(key)
						_objects.RemoveObjectForKey(key);
				}
				catch(Exception e)
				{}
			});
		}
		
		_lock.Unlock();
	}
}
