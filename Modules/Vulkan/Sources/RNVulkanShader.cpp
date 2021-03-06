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

		Array *buffersArray = new Array();
		Array *samplersArray = new Array();
		Array *texturesArray = new Array();

		for(auto &resource : resources.separate_images)
		{
			String *name = RNSTR(resource.name);
			uint8 materialTextureIndex = 0;

			//TODO: Move this into the shader base class
			if(name->IsEqual(RNCSTR("directionalShadowTexture")))
			{
				materialTextureIndex = ArgumentTexture::IndexDirectionalShadowTexture;
			}
			else if (name->IsEqual(RNCSTR("framebufferTexture")))
			{
				materialTextureIndex = ArgumentTexture::IndexFramebufferTexture;
			}
			else if(name->HasPrefix(RNCSTR("texture")))
			{
				String *indexString = name->GetSubstring(Range(7, name->GetLength() - 7));
				materialTextureIndex = std::stoi(indexString->GetUTF8String());
			}

			uint32 binding = reflector.get_decoration(resource.id, spv::DecorationBinding);
			ArgumentTexture *argumentTexture = new ArgumentTexture(name, binding, materialTextureIndex);
			texturesArray->AddObject(argumentTexture->Autorelease());
		}

		for(auto &resource : resources.separate_samplers)
		{
			//TODO: Move this into the shader base class
			String *name = RNSTR(resource.name);
			uint32 binding = reflector.get_decoration(resource.id, spv::DecorationBinding);
			ArgumentSampler *argumentSampler = nullptr;
			if(name->IsEqual(RNCSTR("directionalShadowSampler")))
			{
				argumentSampler = new ArgumentSampler(name, binding, ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Less);
			}
			else //TODO: Pre define some special names like linearRepeatSampler
			{
				samplers->Enumerate<ArgumentSampler>([&](ArgumentSampler *sampler, size_t index, bool &stop){
					if(sampler->GetName()->IsEqual(name))
					{
						argumentSampler = sampler->Copy();
						argumentSampler->SetIndex(binding);
						stop = true;
					}
				});

				if(!argumentSampler)
				{
					argumentSampler = new ArgumentSampler(name, binding);
				}
			}

			samplersArray->AddObject(argumentSampler->Autorelease());
		}

		for(auto &resource : resources.uniform_buffers)
		{
			Array *uniformDescriptors = new Array();

			auto &type = reflector.get_type(resource.base_type_id);
			unsigned memberCount = type.member_types.size();
			for(size_t index = 0; index < memberCount; index++)
			{
				size_t offset = reflector.type_struct_member_offset(type, index);

				const std::string &memberName = reflector.get_member_name(type.self, index);
				String *name = RNSTR(memberName);

				if(name->GetLength() == 0) break;

				spirv_cross::SPIRType spirvUniformType = reflector.get_type(type.member_types[index]);

				PrimitiveType uniformType = PrimitiveType::Invalid;
				if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Float)
				{
					if(spirvUniformType.columns == 1)
					{
						if(spirvUniformType.vecsize == 1) uniformType = PrimitiveType::Float;
						else if(spirvUniformType.vecsize == 2) uniformType = PrimitiveType::Vector2;
						else if(spirvUniformType.vecsize == 3) uniformType = PrimitiveType::Vector3;
						else if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::Vector4;
					}
					else
					{
						if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::Matrix;
					}
				}
				else if(spirvUniformType.columns == 1 && spirvUniformType.vecsize == 1)
				{
					if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Int)
					{
						uniformType = PrimitiveType::Int32;
					}
					else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::UInt)
					{
						uniformType = PrimitiveType::Uint32;
					}
					else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Short)
					{
						uniformType = PrimitiveType::Int16;
					}
					else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::UShort)
					{
						uniformType = PrimitiveType::Uint16;
					}
					else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::SByte)
					{
						uniformType = PrimitiveType::Int8;
					}
					else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::UByte)
					{
						uniformType = PrimitiveType::Uint8;
					}
				}

				UniformDescriptor *descriptor = new UniformDescriptor(name, uniformType, offset);
				uniformDescriptors->AddObject(descriptor->Autorelease());
			}

			if(uniformDescriptors->GetCount() > 0)
			{
				uint32 binding = reflector.get_decoration(resource.id, spv::DecorationBinding);
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR(resource.name), binding, uniformDescriptors->Autorelease());
				buffersArray->AddObject(argumentBuffer->Autorelease());
			}
			else
			{
				uniformDescriptors->Release();
			}
		}

		Signature *signature = new Signature(buffersArray->Autorelease(), samplersArray->Autorelease(), texturesArray->Autorelease());
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
