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
	class MetalRenderer;
	class MetalShaderLibrary : public ShaderLibrary
	{
	public:
		friend class MetalRenderer;

		MTLAPI ~MetalShaderLibrary() override;

		MTLAPI Shader *GetShaderWithName(const String *name) final;
		MTLAPI Array *GetShaderNames() const final;

	private:
		MetalShaderLibrary(void *library);

		void *_library;

		RNDeclareMetaAPI(MetalShaderLibrary, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADERLIBRARY_H_ */
