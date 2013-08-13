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
#include "RNString.h"
#include "RNDictionary.h"

#define kRNResourceKeyTexture1Shader       RNCSTR("kRNResourceKeyTexture1Shader")
#define kRNResourceKeyWaterShader          RNCSTR("kRNResourceKeyWaterShader")
#define kRNResourceKeyParticleShader       RNCSTR("kRNResourceKeyParticleShader")

#define kRNResourceKeyDrawFramebufferShader RNCSTR("kRNResourceKeyDrawFramebufferShader")

#define kRNResourceKeyUIImageShader        RNCSTR("kRNResourceKeyUIImageShader")
#define kRNResourceKeyUITextShader         RNCSTR("kRNResourceKeyUITextShader")

#define kRNResourceKeyLightTileSampleFirstShader RNCSTR("kRNResourceKeyLightTileSampleFirstShader")
#define kRNResourceKeyLightTileSampleShader      RNCSTR("kRNResourceKeyLightTileSampleShader")
#define kRNResourceKeyLightDepthShader           RNCSTR("kRNResourceKeyLightDepthShader")
#define kRNResourceKeyShadowDepthShader          RNCSTR("kRNResourceKeyShadowDepthShader")

#define kRNResourceKeyDefaultFont     RNCSTR("kRNResourceKeyDefaultFont")
#define kRNResourceKeyDefaultFontBold RNCSTR("kRNResourceKeyDefaultFontBold")

namespace RN
{
	class Kernel;
	class ResourcePool : public Singleton<ResourcePool>
	{
	public:
		friend class Kernel;
		
		void AddResource(Object *resource, String *key);
		void RemoveResource(String *key);
		
		template<typename T>
		T *ResourceWithName(String *key)
		{
			T *object;
			
			_lock.Lock();
			object = _objects.ObjectForKey<T>(key);
			_lock.Unlock();
			
			return object;
		}
		
	private:
		void __RemoveResource(String *key);
		
		void LoadDefaultResources(ThreadPool::Batch *batch);
		void LoadShader(const std::string& name, String *key);
		void LoadFont(const std::string& name, float size, uint32 traits, String *key);
		
		SpinLock _lock;
		Dictionary _objects;
	};
}

#endif /* __RAYNE_RESOURCEPOOL_H__ */
