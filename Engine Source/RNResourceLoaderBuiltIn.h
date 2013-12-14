//
//  RNResourceLoaderBuiltIn.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RESOURCELOADER_BUILTIN_H__
#define __RAYNE_RESOURCELOADER_BUILTIN_H__

#include "RNBase.h"
#include "RNResourceLoader.h"

namespace RN
{
	class PNGResourceLoader : public ResourceLoader
	{
	public:
		PNGResourceLoader();
		
		Object *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDefineMeta(PNGResourceLoader, ResourceLoader)
	};
	
	class GLSLResourceLoader : public ResourceLoader
	{
	public:
		GLSLResourceLoader();
		
		Object *Load(String *name, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		bool SupportsLoadingName(String *name) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDefineMeta(GLSLResourceLoader, ResourceLoader)
	};
}

#endif /* __RAYNE_RESOURCELOADER_BUILTIN_H__ */
