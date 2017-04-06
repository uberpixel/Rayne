//
//  RND3D12Shader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12SHADER_H_
#define __RAYNE_D3D12SHADER_H_

#include "RND3D12.h"

namespace RN
{
	class D3D12ShaderLibrary;
	class D3D12Shader : public Shader
	{
	public:
		friend class D3D12SpecificShaderLibrary;
		friend class D3D12StateCoordinator;
		friend class D3D12Renderer;

		D3DAPI ~D3D12Shader() final;

		D3DAPI const String *GetName() const final;

	private:
		D3D12Shader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, const Shader::Options *options, const Signature *signature);

		Array *_uniformDescriptors;
		ID3DBlob *_shader;
		const String *_name;

		RNDeclareMetaAPI(D3D12Shader, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12SHADER_H_ */
