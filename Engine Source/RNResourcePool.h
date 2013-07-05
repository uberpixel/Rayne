//
//  RNResourcePool.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCEPOOL_H__
#define __RAYNE_RESOURCEPOOL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThreadPool.h"

#define kRNResourceKeyTexture1Shader       "kRNResourceKeyTexture1Shader"
#define kRNResourceKeyWaterShader          "kRNResourceKeyWaterShader"
#define kRNResourceKeyParticleShader       "kRNResourceKeyParticleShader"

#define kRNResourceKeyDrawFramebufferShader "kRNResourceKeyDrawFramebufferShader"

#define kRNResourceKeyUIImageShader        "kRNResourceKeyUIImageShader"
#define kRNResourceKeyUITextShader         "kRNResourceKeyUITextShader"

#define kRNResourceKeyLightTileSampleFirstShader "kRNResourceKeyLightTileSampleFirstShader"
#define kRNResourceKeyLightTileSampleShader      "kRNResourceKeyLightTileSampleShader"
#define kRNResourceKeyLightDepthShader           "kRNResourceKeyLightDepthShader"
#define kRNResourceKeyShadowDepthShader          "kRNResourceKeyShadowDepthShader"

#define kRNResourceKeyDefaultFont     "kRNResourceKeyDefaultFont"
#define kRNResourceKeyDefaultFontBold "kRNResourceKeyDefaultFontBold"

namespace RN
{
	class Kernel;
	class ResourcePool : public Singleton<ResourcePool>
	{
	friend class Kernel;
	public:
		void AddResource(Object *resource, const std::string& name);
		void RemoveResource(const std::string& name);
		
		template<typename T>
		T *ResourceWithName(const std::string& name)
		{
			return static_cast<T *>(ObjectWithName(name));
		}
		
	private:
		Object *ObjectWithName(const std::string& name);
		void __RemoveResource(const std::string& name);
		
		void LoadDefaultResources(ThreadPool::Batch& batch);
		void LoadShader(const std::string& name, const std::string& key);
		void LoadFont(const std::string& name, float size, uint32 traits, const std::string& key);
		
		SpinLock _lock;
		std::unordered_map<std::string, Object *> _objects;
	};
}

#endif /* __RAYNE_RESOURCEPOOL_H__ */
