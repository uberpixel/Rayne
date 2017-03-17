//
//  RNMetalShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALSHADERLIBRARY_H_
#define __RAYNE_METALSHADERLIBRARY_H_

#include "RNMetal.h"

namespace RN
{
	class MetalSpecializedShaderLibrary : public Object
	{
	public:
		friend class MetalShaderLibrary;

		~MetalSpecializedShaderLibrary();

		Shader *GetShaderWithName(const String *name);
		//Array *GetShaderNames() const;

		RNDeclareMetaAPI(MetalSpecializedShaderLibrary, MTLAPI)

	private:
		MetalSpecializedShaderLibrary(id<MTLDevice> device, const String *source, const ShaderOptions *options);
		void *_library;
	};

	class MetalShaderLibrary : public ShaderLibrary
	{
	public:
		friend class MetalRenderer;

		MTLAPI ~MetalShaderLibrary() override;

		MTLAPI Shader *GetShaderWithName(const String *name, const ShaderOptions *options) final;

	private:
		MetalShaderLibrary(id<MTLDevice> device, const String *source);

		id<MTLDevice> _device;
		const String *_source;
		Dictionary *_specializedLibraries;

		RNDeclareMetaAPI(MetalShaderLibrary, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADERLIBRARY_H_ */
