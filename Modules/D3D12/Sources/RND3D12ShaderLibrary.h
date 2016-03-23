//
//  RND3D12ShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12SHADERLIBRARY_H_
#define __RAYNE_D3D12SHADERLIBRARY_H_

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12ShaderLibrary : public ShaderLibrary
	{
	public:
		friend class D3D12Renderer;

		D3DAPI ~D3D12ShaderLibrary() override;

		D3DAPI Shader *GetShaderWithName(const String *name) final;
		D3DAPI Array *GetShaderNames() const final;

	private:
		D3D12ShaderLibrary(const String *file);

		Dictionary *_shaders;

		RNDeclareMetaAPI(D3D12ShaderLibrary, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12SHADERLIBRARY_H_ */
