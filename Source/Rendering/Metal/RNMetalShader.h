//
//  RNMetalShader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALSHADER_H_
#define __RAYNE_METALSHADER_H_

#include "../../Base/RNBase.h"
#include "../RNShader.h"

namespace RN
{
	class MetalShaderLibrary;
	class MetalShader : public Shader
	{
	public:
		friend class MetalShaderLibrary;

		RNAPI ~MetalShader() override;

		RNAPI const String *GetName() const override;

	private:
		MetalShader(void *shader);

		void *_shader;

		RNDeclareMeta(MetalShader)
	};
}


#endif /* __RAYNE_METALSHADER_H_ */
