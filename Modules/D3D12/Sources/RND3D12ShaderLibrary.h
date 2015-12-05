//
//  RND3D12ShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12SHADERLIBRARY_H_
#define __RAYNE_D3D12SHADERLIBRARY_H_

#include <Rayne.h>

namespace RN
{
	class D3D12Renderer;
	class D3D12ShaderLibrary : public ShaderLibrary
	{
	public:
		friend class D3D12Renderer;

		~D3D12ShaderLibrary() override;

		Shader *GetShaderWithName(const String *name) final;
		Array *GetShaderNames() const final;

	private:
		D3D12ShaderLibrary(void *library);

		void *_library;

		RNDeclareMeta(D3D12ShaderLibrary)
	};
}


#endif /* __RAYNE_D3D12SHADERLIBRARY_H_ */
