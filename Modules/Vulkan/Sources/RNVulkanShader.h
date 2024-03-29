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
		VulkanShader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, bool hasInstancing, const Shader::Options *options, const Array *samplers);

		VkPipelineShaderStageCreateInfo _shaderStage;
		VkShaderModule _module;
		const String *_name;
		Data *_entryPoint;

		uint32 _hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom) + 1];
		ArgumentBuffer *_instancingAttributes;

		RNDeclareMetaAPI(VulkanShader, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSHADER_H_ */
