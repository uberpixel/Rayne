//
//  RNVulkanShaderLibrary.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_VULKANSHADERLIBRARY_H_
#define __RAYNE_VULKANSHADERLIBRARY_H_

#include "RNVulkan.h"

namespace RN
{
	class VulkanSpecificShaderLibrary : public Object
	{
	public:
		friend class VulkanShaderLibrary;

		~VulkanSpecificShaderLibrary();
		Shader *GetShaderWithOptions(ShaderLibrary *library, const Shader::Options *options);

	private:
		VulkanSpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type, Dictionary *signatureDescription);

		const Shader::Options *GetCleanedShaderOptions(const Shader::Options *options) const;
		const Array *GetSamplerSignature(const Shader::Options *options) const;

		Dictionary *_shaders;

		const String *_entryPoint;
		const String *_fileName;
		Shader::Type _type;
		Dictionary *_signatureDescription;

		RNDeclareMetaAPI(VulkanSpecificShaderLibrary, VKAPI)
	};

	class VulkanShaderLibrary : public ShaderLibrary
	{
	public:
		friend class D3D12Renderer;

		VKAPI ~VulkanShaderLibrary() override;

		VKAPI Shader *GetShaderWithName(const String *name, const Shader::Options *options = nullptr) final;
		VKAPI Shader *GetInstancedShaderForShader(Shader *shader) final;

	private:
		VulkanShaderLibrary(const String *file);

		Dictionary *_specificShaderLibraries;

	RNDeclareMetaAPI(VulkanShaderLibrary, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSHADERLIBRARY_H_ */
