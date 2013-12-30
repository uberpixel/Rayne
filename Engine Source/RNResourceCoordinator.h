//
//  RNResourceCoordinator.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCECOORDINATOR_H__
#define __RAYNE_RESOURCECOORDINATOR_H__

#include "RNBase.h"
#include "RNFile.h"
#include "RNArray.h"
#include "RNString.h"
#include "RNDictionary.h"
#include "RNResourceLoader.h"
#include "RNMutex.h"

#define kRNResourceCoordinatorBuiltInPriority 100

#define kRNResourceKeyTexture1Shader       RNCSTR("kRNResourceKeyTexture1Shader")
#define kRNResourceKeyWaterShader          RNCSTR("kRNResourceKeyWaterShader")
#define kRNResourceKeyParticleShader       RNCSTR("kRNResourceKeyParticleShader")

#define kRNResourceKeyDrawFramebufferShader RNCSTR("kRNResourceKeyDrawFramebufferShader")

#define kRNResourceKeyUIImageShader        RNCSTR("kRNResourceKeyUIImageShader")
#define kRNResourceKeyUITextShader         RNCSTR("kRNResourceKeyUITextShader")

#define kRNResourceKeyLightTileSampleFirstShader      RNCSTR("kRNResourceKeyLightTileSampleFirstShader")
#define kRNResourceKeyLightTileSampleShader           RNCSTR("kRNResourceKeyLightTileSampleShader")
#define kRNResourceKeyLightDepthShader                RNCSTR("kRNResourceKeyLightDepthShader")
#define kRNResourceKeyDirectionalShadowDepthShader    RNCSTR("kRNResourceKeyDirectionalShadowDepthShader")
#define kRNResourceKeyPointShadowDepthShader          RNCSTR("kRNResourceKeyPointShadowDepthShader")

namespace RN
{
	class ResourceCoordinator : public ISingleton<ResourceCoordinator>
	{
	public:
		friend class Kernel;
		
		RNAPI ResourceCoordinator();
		RNAPI ~ResourceCoordinator() override;
		
		template<class T>
		T *GetResourceWithName(String *name, Dictionary *settings)
		{
			return static_cast<T *>(RequestResourceWithName(T::MetaClass(), name, settings));
		}
		
		template<class T>
		std::shared_future<Object *> GetFutureResourceWithName(String *name, Dictionary *settings)
		{
			return RequestFutureResourceWithName(T::MetaClass(), name, settings);
		}
		
		RNAPI void AddResource(Object *object, String *name);
		RNAPI void AddResourceAlias(Object *object, String *alias);
		
		RNAPI void RemoveResource(Object *object);
		RNAPI void RemoveResourceWithName(String *name);
		
		RNAPI void WaitForResources();
		
		RNAPI void RegisterResourceLoader(ResourceLoader *loader);
		RNAPI void UnregisterResourceLoader(ResourceLoader *loader);
		
	private:
		void __AddResourceAlias(Object *object, String *name);
		
		Object *RequestResourceWithName(MetaClassBase *base, String *name, Dictionary *settings);
		std::shared_future<Object *> RequestFutureResourceWithName(MetaClassBase *base, String *name, Dictionary *settings);
		
		Object *ValidateResource(MetaClassBase *base, Object *object);
		ResourceLoader *PickResourceLoader(MetaClassBase *base, File *file, String *name, bool requiresBackgroundSupport);
		
		void LoadShader(String *name, String *key);
		void LoadEngineResources();
		
		Mutex _lock;
		
		Array _loader;
		Dictionary _resources;
		Dictionary _resourceToAlias;
		
		std::unordered_map<String *, std::shared_future<Object *>> _requests;
		
		RNDefineSingleton(ResourceCoordinator)
	};
}

#endif /* __RAYNE_RESOURCECOORDINATOR_H__ */
