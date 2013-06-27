//
//  RNResourcePool.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResourcePool.h"
#include "RNShader.h"
#include "RNUIFont.h"

namespace RN
{
	void ResourcePool::LoadDefaultResources(ThreadPool::Batch& batch)
	{
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Texture1", kRNResourceKeyTexture1Shader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Water", kRNResourceKeyWaterShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_Particle", kRNResourceKeyParticleShader));

		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_UIImage", kRNResourceKeyUIImageShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_UIText", kRNResourceKeyUITextShader));

		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightTileSampleFirst", kRNResourceKeyLightTileSampleFirstShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightTileSample", kRNResourceKeyLightTileSampleShader));
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_LightDepth", kRNResourceKeyLightDepthShader));
		
		batch->AddTask(std::bind(&ResourcePool::LoadFont, this, "Helvetica Neue", kRNResourceKeyDefaultFont));
		
		// Must be done on this thread and blocking because there are cameras created that require this shader
		LoadShader("shader/rn_DrawFramebuffer", kRNResourceKeyDrawFramebufferShader);
	}
	
	void ResourcePool::LoadShader(const std::string& name, const std::string& key)
	{
		Shader *shader = Shader::WithFile(name);
		AddResource(shader, key);
	}
	
	void ResourcePool::LoadFont(const std::string& name, const std::string& key)
	{
		UI::Font *font = UI::Font::WithName(name, 14.0f);
		AddResource(font, key);
	}
	
	
	
	void ResourcePool::AddResource(Object *resource, const std::string& name)
	{
		_lock.Lock();
		
		__RemoveResource(name);
		if(resource)
		{
			_objects.insert(std::unordered_map<std::string, Object *>::value_type(name, resource));
			resource->Retain();
		}
		
		_lock.Unlock();
	}
	
	void ResourcePool::RemoveResource(const std::string& name)
	{
		_lock.Lock();
		__RemoveResource(name);
		_lock.Unlock();
	}
	
	void ResourcePool::__RemoveResource(const std::string& name)
	{
		auto iterator = _objects.find(name);
		if(iterator != _objects.end())
		{
			Object *object = iterator->second;
			object->Release();
			
			_objects.erase(iterator);
		}
	}
	
	Object *ResourcePool::ObjectWithName(const std::string& name)
	{
		_lock.Lock();
		
		auto iterator = _objects.find(name);
		Object *object = (iterator != _objects.end()) ? iterator->second : 0;
		
		_lock.Unlock();
		
		return object;
	}
}
