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
	class MetalStateCoordinator;
	class MetalSpecializedShaderLibrary : public Object
	{
	public:
		friend class MetalShaderLibrary;

		~MetalSpecializedShaderLibrary();

		Shader *GetShaderWithName(const String *name, ShaderLibrary *library, id<MTLDevice> device, MetalStateCoordinator *coordinator);

		RNDeclareMetaAPI(MetalSpecializedShaderLibrary, MTLAPI)

	private:
		MetalSpecializedShaderLibrary(id<MTLDevice> device, const String *source, const Shader::Options *options);
		void *_metalLibrary;
		const Shader::Options *_options;
		Dictionary *_shaders;
	};

	class MetalShaderLibrary : public ShaderLibrary
	{
	public:
		friend class MetalRenderer;

		MTLAPI ~MetalShaderLibrary() override;

		MTLAPI Shader *GetShaderWithName(const String *name, const Shader::Options *options) final;
		MTLAPI Shader *GetInstancedShaderForShader(Shader *shader) final;

	private:
		MetalShaderLibrary(id<MTLDevice> device, const String *source, MetalStateCoordinator *coordinator);

		id<MTLDevice> _device;
		const String *_source;
		Dictionary *_specializedLibraries;
		MetalStateCoordinator *_coordinator;

		RNDeclareMetaAPI(MetalShaderLibrary, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADERLIBRARY_H_ */
