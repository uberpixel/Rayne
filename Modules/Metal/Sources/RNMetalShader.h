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
	class MetalShader : public Shader
	{
	public:
		friend class MetalSpecializedShaderLibrary;
		friend class MetalStateCoordinator;

		MTLAPI ~MetalShader() override;
		MTLAPI const String *GetName() const override;

	private:
		MetalShader(ShaderLibrary *library, Type type, const ShaderOptions *options, void *shader);
		void SetReflectedArguments(NSArray *arguments);

		void *_shader;

		RNDeclareMetaAPI(MetalShader, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADER_H_ */
