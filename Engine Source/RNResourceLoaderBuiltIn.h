//
//  RNResourceLoaderBuiltIn.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		
		RNDeclareMeta(PNGResourceLoader, ResourceLoader)
	};
	
	class Model;
	class SGMResourceLoader : public ResourceLoader
	{
	public:
		SGMResourceLoader();
		
		Object *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
	private:
		void LoadLODStage(File *file, Model *model, size_t stage, bool guessMaterial);
		
		RNDeclareMeta(SGMResourceLoader, ResourceLoader)
	};
	
	class SGAResourceLoader : public ResourceLoader
	{
	public:
		SGAResourceLoader();
		
		Object *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDeclareMeta(SGAResourceLoader, ResourceLoader)
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
		
		RNDeclareMeta(GLSLResourceLoader, ResourceLoader)
	};
}

#endif /* __RAYNE_RESOURCELOADER_BUILTIN_H__ */
