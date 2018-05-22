//
//  RNVulkanShader.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_VULKANSHADER_H_
#define __RAYNE_VULKANSHADER_H_

#include "RNVulkan.h"

namespace RN
{
	class VulkanShader : public Shader
	{
	public:
		friend class VulkanSpecificShaderLibrary;
		friend class VulkanStateCoordinator;
		friend class VulkanRenderer;

		VKAPI ~VulkanShader() override;

		VKAPI const String *GetName() const final;

	private:
		VulkanShader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, const Shader::Options *options, const Array *samplers);

		VkPipelineShaderStageCreateInfo _shaderStage;
		VkShaderModule _module;
		const String *_name;

		bool _wantsDirectionalShadowTexture; //TODO: Solve better...

		RNDeclareMetaAPI(VulkanShader, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSHADER_H_ */
