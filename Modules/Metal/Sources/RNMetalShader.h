//
//  RNMetalShader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALSHADER_H_
#define __RAYNE_METALSHADER_H_

#include "RNMetal.h"

namespace RN
{
	class MetalStateCoordinator;
	class MetalShader : public Shader
	{
	public:
		friend class MetalSpecificShaderLibrary;
		friend class MetalStateCoordinator;
		friend class MetalRenderer;

		MTLAPI ~MetalShader() override;
		MTLAPI const String *GetName() const override;

	private:
		MetalShader(ShaderLibrary *library, Type type, bool hasInstancing, const Array *samplers, const Shader::Options *options, void *shader, MetalStateCoordinator *coordinator);
		void SetReflectedArguments(NSArray *arguments);
		
		void AddBufferStructElements(Array *uniformDescriptors, MTLStructType *structType, bool &isInstanceBuffer);

		void *_shader;
		std::vector<void*> _samplers;
		std::vector<uint16> _samplerToIndexMapping;
		const Array *_rnSamplers; //TODO: Fix naming...
		MetalStateCoordinator *_coordinator;

		RNDeclareMetaAPI(MetalShader, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADER_H_ */
