//
//  RNVulkanShader.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanShader.h"
#include "RNVulkanRenderer.h"
#include "spirv_cross.hpp"

namespace RN
{
	RNDefineMeta(VulkanShader, Shader)

	VulkanShader::VulkanShader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, const Shader::Options *options, const Array *samplers) :
		Shader(library, type, options), _name(entryPoint->Retain())
	{
		VulkanRenderer *renderer = VulkanRenderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();

		Data *shaderData = Data::WithContentsOfFile(fileName);
		_entryPoint = Data::WithBytes(reinterpret_cast<uint8 *>(_name->GetUTF8String()), _name->GetLength()+1);
		_entryPoint->Retain();

		//Create the shader module
		const char *shaderCode = static_cast<const char*>(shaderData->GetBytes());
		size_t size = shaderData->GetLength();

		VkShaderModuleCreateInfo moduleCreateInfo;

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;

		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		moduleCreateInfo.flags = 0;

		RNVulkanValidate(vk::CreateShaderModule(device->GetDevice(), &moduleCreateInfo, renderer->GetAllocatorCallback(), &_module));

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
		_shaderStage.module = _module;
		_shaderStage.pName = _entryPoint->GetBytes<const char>();

		spirv_cross::Compiler reflector(static_cast<uint32_t*>(shaderData->GetBytes()), shaderData->GetLength() / 4);
		spirv_cross::ShaderResources resources = reflector.get_shader_resources();

		uint8 textureCount = 0;
		Array *reflectionSamplers = new Array();
		Array *specificReflectionSamplers = new Array();
		Array *uniformDescriptors = new Array();

		_wantsDirectionalShadowTexture = false;
		for(uint32 i = 0; i < static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom) + 1; i++)
		{
			_hasInputVertexAttribute[i] = false;
		}

		if(stage == VK_SHADER_STAGE_VERTEX_BIT)
		{
			for(auto &resource : resources.stage_inputs)
			{
				String *name = RNSTR(resource.name);
				if(name->IsEqual(RNCSTR("in_var_POSITION")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Vertices)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_NORMAL")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Normals)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_TANGENT")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Tangents)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_COLOR")) || name->IsEqual(RNCSTR("in_var_COLOR0")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color0)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_COLOR1")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color1)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_TEXCOORD")) || name->IsEqual(RNCSTR("in_var_TEXCOORD0")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords0)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_TEXCOORD1")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords1)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_BONEWEIGHTS")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneWeights)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_BONEINDICES")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneIndices)] = true;
				}
				else if (name->IsEqual(RNCSTR("in_var_CUSTOM")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom)] = true;
				}
			}
			
		}

		for(auto &resource : resources.separate_images)
		{
			//TODO: Move this into the shader base class
			String *name = RNSTR(resource.name);
			if(name->IsEqual(RNCSTR("directionalShadowTexture")))
			{
				//TODO: Store the register, so it doesn't have to be declared last in the shader
				_wantsDirectionalShadowTexture = true;
			}

			textureCount += 1;
		}

		for(auto &resource : resources.separate_samplers)
		{
			//TODO: Move this into the shader base class
			String *name = RNSTR(resource.name);
			if(name->IsEqual(RNCSTR("directionalShadowSampler")))
			{
				//TODO: Store the register, so it doesn't have to be declared last in the shader
				Sampler *sampler = new Sampler(Sampler::WrapMode::Clamp, Sampler::Filter::Linear, Sampler::ComparisonFunction::Less);
				specificReflectionSamplers->AddObject(sampler->Autorelease());
			}
			else
			{
				Sampler *sampler = new Sampler();
				reflectionSamplers->AddObject(sampler->Autorelease());
			}
		}

		for(auto &resource : resources.uniform_buffers)
		{
			for(size_t index = 0; index < 10; index++)
			{
				uint32_t offset = reflector.get_member_decoration(resource.base_type_id, index, spv::DecorationOffset);

				std::string membername = reflector.get_member_name(resource.base_type_id, index);
				String *name = RNSTR(membername);

				if(name->GetLength() == 0) break;

				UniformDescriptor *descriptor = new UniformDescriptor(name, offset);
				uniformDescriptors->AddObject(descriptor->Autorelease());
			}
		}

		if(samplers->GetCount() > 0)
		{
			RN_ASSERT(reflectionSamplers->GetCount() == samplers->GetCount(), "Sampler count missmatch!");
			reflectionSamplers->RemoveAllObjects();
			reflectionSamplers->AddObjectsFromArray(samplers);
		}

		reflectionSamplers->AddObjectsFromArray(specificReflectionSamplers->Autorelease());

		Signature *signature = new Signature(uniformDescriptors->Autorelease(), reflectionSamplers->Autorelease(), textureCount);
		Shader::SetSignature(signature->Autorelease());
	}

	VulkanShader::~VulkanShader()
	{
		VulkanRenderer *renderer = VulkanRenderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();
		vk::DestroyShaderModule(device->GetDevice(), _module, renderer->GetAllocatorCallback());

		_name->Release();
		_entryPoint->Release();
	}

	const String *VulkanShader::GetName() const
	{
		return _name;
	}
}
