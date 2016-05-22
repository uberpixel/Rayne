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
		friend class VulkanShaderLibrary;
		friend class VulkanRenderer;
		friend class VulkanStateCoordinator;

		VKAPI ~VulkanShader() override;

		VKAPI const String *GetName() const override;
		VKAPI const Array *GetAttributes() const override;

	private:
		VulkanShader(const String *name, Shader::Type type, VkShaderModule module, ShaderLibrary *library);

		Array *_attributes;
		VkPipelineShaderStageCreateInfo _shaderStage;
		VkShaderModule _module;
		String *_name;

		RNDeclareMetaAPI(VulkanShader, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSHADER_H_ */
