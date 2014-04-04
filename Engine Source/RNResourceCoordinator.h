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
#include "RNAsset.h"
#include "RNFile.h"
#include "RNArray.h"
#include "RNString.h"
#include "RNDictionary.h"
#include "RNResourceLoader.h"
#include "RNMutex.h"

#define kRNResourceCoordinatorBuiltInPriority 100

#define kRNResourceKeyDefaultShader       RNCSTR("kRNResourceKeyDefaultShader")
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
		friend class Asset;
		
		RNAPI ResourceCoordinator();
		RNAPI ~ResourceCoordinator() override;
		
		template<class T>
		T *GetResourceWithName(String *name, Dictionary *settings)
		{
			return static_cast<T *>(RequestResourceWithName(T::MetaClass(), name, settings));
		}
		
		template<class T>
		std::shared_future<Asset *> GetFutureResourceWithName(String *name, Dictionary *settings)
		{
			return RequestFutureResourceWithName(T::MetaClass(), name, settings);
		}
		
		RNAPI void WaitForResources();
		
		RNAPI void RegisterResourceLoader(ResourceLoader *loader);
		RNAPI void UnregisterResourceLoader(ResourceLoader *loader);
		
	private:
		RNAPI Asset *RequestResourceWithName(MetaClassBase *base, String *name, Dictionary *settings);
		RNAPI std::shared_future<Asset *> RequestFutureResourceWithName(MetaClassBase *base, String *name, Dictionary *settings);
		
		void __AddResource(Asset *object);
		void RemoveResource(Asset *object);
		
		Asset *ValidateResource(MetaClassBase *base, Asset *object);
		void PrepareResource(Asset *object, String *name, Dictionary *settings);
		
		ResourceLoader *PickResourceLoader(MetaClassBase *base, File *file, String *name, bool requiresBackgroundSupport);
		
		void LoadShader(String *name, String *key);
		void LoadEngineResources();
		
		Mutex _lock;
		Array _loader;
		
		std::unordered_map<String *, Asset *, std::hash<Object>, std::equal_to<Object>> _resources;
		std::unordered_map<String *, std::shared_future<Asset *>, std::hash<Object>, std::equal_to<Object>> _requests;
		
		RNDeclareSingleton(ResourceCoordinator)
	};
}

#endif /* __RAYNE_RESOURCECOORDINATOR_H__ */
