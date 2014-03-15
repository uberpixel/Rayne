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
		
		Asset *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDeclareMeta(PNGResourceLoader)
	};
	
	class Model;
	class SGMResourceLoader : public ResourceLoader
	{
	public:
		SGMResourceLoader();
		
		Asset *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
	private:
		void LoadLODStage(File *file, Model *model, size_t stage, bool guessMaterial);
		
		RNDeclareMeta(SGMResourceLoader)
	};
	
	class SGAResourceLoader : public ResourceLoader
	{
	public:
		SGAResourceLoader();
		
		Asset *Load(File *file, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDeclareMeta(SGAResourceLoader)
	};
	
	class GLSLResourceLoader : public ResourceLoader
	{
	public:
		GLSLResourceLoader();
		
		Asset *Load(String *name, Dictionary *settings) override;
		
		bool SupportsBackgroundLoading() override;
		bool SupportsLoadingFile(File *file) override;
		bool SupportsLoadingName(String *name) override;
		
		uint32 GetPriority() const override;
		
		static void InitialWakeUp(MetaClassBase *meta);
		
		RNDeclareMeta(GLSLResourceLoader)
	};
}

#endif /* __RAYNE_RESOURCELOADER_BUILTIN_H__ */
