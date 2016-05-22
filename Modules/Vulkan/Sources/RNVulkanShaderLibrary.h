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
	class VulkanShaderLibrary : public ShaderLibrary
	{
	public:
		friend class VulkanRenderer;

		VKAPI ~VulkanShaderLibrary() override;

		VKAPI Shader *GetShaderWithName(const String *name) final;
		VKAPI Array *GetShaderNames() const final;

	private:
		VulkanShaderLibrary(class VulkanRenderer *renderer, const String *file, const ShaderCompileOptions *options);

		class VulkanRenderer *_renderer;

		VkShaderModule _shaderModule;
		Dictionary *_shaders;

		RNDeclareMetaAPI(VulkanShaderLibrary, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSHADERLIBRARY_H_ */
