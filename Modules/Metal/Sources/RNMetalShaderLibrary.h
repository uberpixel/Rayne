//
//  RNMetalShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALSHADERLIBRARY_H_
#define __RAYNE_METALSHADERLIBRARY_H_

#include <Rayne.h>

namespace RN
{
	class MetalRenderer;
	class MetalShaderLibrary : public ShaderLibrary
	{
	public:
		friend class MetalRenderer;

		~MetalShaderLibrary() override;

		Shader *GetShaderWithName(const String *name) final;
		Array *GetShaderNames() const final;

	private:
		MetalShaderLibrary(void *library);

		void *_library;

		RNDeclareMetaAPI(MetalShaderLibrary, MTLAPI)
	};
}


#endif /* __RAYNE_METALSHADERLIBRARY_H_ */
