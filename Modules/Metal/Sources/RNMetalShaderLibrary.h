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
	class MetalSpecificShaderLibrary : public Object
	{
	public:
		friend class MetalShaderLibrary;

		~MetalSpecificShaderLibrary();

		Shader *GetShaderWithName(const String *name, ShaderLibrary *library, MetalStateCoordinator *coordinator);
		
		Shader *GetShaderWithOptions(id<MTLDevice> device, MetalStateCoordinator *coordinator, ShaderLibrary *library, const Shader::Options *options);

	private:
		MetalSpecificShaderLibrary(id<MTLDevice> device, const String *fileName, const String *entryPoint, Shader::Type type, Dictionary *signatureDescription);
		
		const Shader::Options *GetCleanedShaderOptions(const Shader::Options *options) const;
		const Array *GetSamplerSignature(const Shader::Options *options) const;
		
		Dictionary *_shaders;
		
		const String *_entryPoint;
		const String *_fileName;
		Shader::Type _type;
		Dictionary *_signatureDescription;
		
		RNDeclareMetaAPI(MetalSpecificShaderLibrary, MTLAPI)
	};

	class MetalShaderLibrary : public ShaderLibrary
	{
	public:
		friend class MetalRenderer;

		MTLAPI ~MetalShaderLibrary() override;

		MTLAPI Shader *GetShaderWithName(const String *name, const Shader::Options *options) final;
		MTLAPI Shader *GetInstancedShaderForShader(Shader *shader) final;

	private:
		MetalShaderLibrary(id<MTLDevice> device, const String *file, MetalStateCoordinator *coordinator);

		id<MTLDevice> _device;
		Dictionary *_specificShaderLibraries;
		MetalStateCoordinator *_coordinator;

		RNDeclareMetaAPI(MetalShaderLibrary, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADERLIBRARY_H_ */
