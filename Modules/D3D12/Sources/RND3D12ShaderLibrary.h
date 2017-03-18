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

	class D3D12SpecificShaderLibrary : public Object
	{
	public:
		friend class D3D12ShaderLibrary;

		~D3D12SpecificShaderLibrary();
		Shader *GetShaderWithOptions(ShaderLibrary *library, const ShaderOptions *options);

	private:
		D3D12SpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type);
		Dictionary *_shaders;

		const String *_entryPoint;
		const String *_fileName;
		Shader::Type _type;

		RNDeclareMetaAPI(D3D12SpecificShaderLibrary, D3DAPI)
	};

	class D3D12ShaderLibrary : public ShaderLibrary
	{
	public:
		friend class D3D12Renderer;

		D3DAPI ~D3D12ShaderLibrary() override;

		D3DAPI Shader *GetShaderWithName(const String *name, const ShaderOptions *options) final;
		D3DAPI Shader *GetInstancedShaderForShader(Shader *shader) final;

	private:
		D3D12ShaderLibrary(const String *file);

		Dictionary *_specificShaderLibraries;

		RNDeclareMetaAPI(D3D12ShaderLibrary, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12SHADERLIBRARY_H_ */
