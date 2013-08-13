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
		batch->AddTask(std::bind(&ResourcePool::LoadShader, this, "shader/rn_ShadowDepthSingle", kRNResourceKeyShadowDepthShader));
		
		batch->AddTask(std::bind(&ResourcePool::LoadFont, this, "Helvetica", 12.0f, 0, kRNResourceKeyDefaultFont));
		batch->AddTask(std::bind(&ResourcePool::LoadFont, this, "Helvetica", 12.0f, UI::FontDescriptor::FontStyleBold, kRNResourceKeyDefaultFontBold));
		
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
	
	{
	}
}
