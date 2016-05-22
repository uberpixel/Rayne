//
//  RNVulkanShader.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanShader.h"

namespace RN
{
	RNDefineMeta(VulkanShader, Shader)

	VulkanShader::VulkanShader(const String *name, Shader::Type type, VkShaderModule module, ShaderLibrary *library) :
		Shader(type, library),
		_attributes(new Array()),
		_name(name->Copy()),
		_module(module)
	{
		VkShaderStageFlagBits stage;

		switch(type)
		{
			case Shader::Type::Vertex:
				stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;

			case Shader::Type::Fragment:
				stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;

			case Shader::Type::Compute:
				stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
		}

		_shaderStage = {};
		_shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		_shaderStage.stage = stage;
		_shaderStage.module = module;
		_shaderStage.pName = "main";
	}
	VulkanShader::~VulkanShader()
	{
		_attributes->Release();
	}

	const String *VulkanShader::GetName() const
	{
		return _name;
	}

	const Array *VulkanShader::GetAttributes() const
	{
		return _attributes;
	}
}
