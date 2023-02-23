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

	VulkanShader::VulkanShader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, bool hasInstancing, const Shader::Options *options, const Array *samplers) :
		Shader(library, type, hasInstancing, options), _name(entryPoint->Retain()), _instancingAttributes(nullptr)
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

		Array *buffersArray = new Array();
		Array *samplersArray = new Array();
		Array *texturesArray = new Array();

		for(uint32 i = 0; i < static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom) + 1; i++)
		{
			_hasInputVertexAttribute[i] = -1;
		}

		if(stage == VK_SHADER_STAGE_VERTEX_BIT)
		{
			Array *instanceAttributes = new Array();
			uint32 instanceAttributeOffset = 0;

			for(auto &resource : resources.stage_inputs)
			{
				String *name = RNSTR(resource.name);
				uint32 location = reflector.get_decoration(resource.id, spv::DecorationLocation);

				if(name->IsEqual(RNCSTR("in_var_POSITION")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Vertices)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_NORMAL")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Normals)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_TANGENT")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Tangents)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_COLOR")) || name->IsEqual(RNCSTR("in_var_COLOR0")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color0)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_COLOR1")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color1)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_TEXCOORD")) || name->IsEqual(RNCSTR("in_var_TEXCOORD0")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords0)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_TEXCOORD1")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords1)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_BONEWEIGHTS")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneWeights)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_BONEINDICES")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneIndices)] = location;
				}
				else if (name->IsEqual(RNCSTR("in_var_CUSTOM")))
				{
					_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom)] = location;
				}

				else if(name->HasPrefix(RNCSTR("in_var_INSTANCEDATA")))
				{
					auto type = reflector.get_type(resource.base_type_id);
					unsigned memberCount = type.member_types.size();

					RNDebug(memberCount);

					//Set array element count to the real number, but default to 1 for all cases
					size_t arrayElementCount = 1;
					if(type.array.size() == 1) //There are more than one, if it is multidimensional, but only support 1 dimension for now
					{
						arrayElementCount = type.array[0];
					}

					PrimitiveType uniformType = PrimitiveType::Invalid;
					if(type.basetype == spirv_cross::SPIRType::BaseType::Float)
					{
						if(type.columns == 1)
						{
							if(type.vecsize == 1) uniformType = PrimitiveType::Float;
							else if(type.vecsize == 2) uniformType = PrimitiveType::Vector2;
							else if(type.vecsize == 3) uniformType = PrimitiveType::Vector3;
							else if(type.vecsize == 4) uniformType = PrimitiveType::Vector4;
						}
						else
						{
							if(type.vecsize == 2) uniformType = PrimitiveType::Matrix2x2;
							if(type.vecsize == 3) uniformType = PrimitiveType::Matrix3x3;
							if(type.vecsize == 4) uniformType = PrimitiveType::Matrix4x4;
						}
					}
					else if(type.basetype == spirv_cross::SPIRType::BaseType::Half)
					{
						if(type.columns == 1)
						{
							if(type.vecsize == 1) uniformType = PrimitiveType::Half;
							else if(type.vecsize == 2) uniformType = PrimitiveType::HalfVector2;
							else if(type.vecsize == 3) uniformType = PrimitiveType::HalfVector3;
							else if(type.vecsize == 4) uniformType = PrimitiveType::HalfVector4;
						}
					}
					else if(type.columns == 1 && type.vecsize == 1)
					{
						if(type.basetype == spirv_cross::SPIRType::BaseType::Int)
						{
							uniformType = PrimitiveType::Int32;
						}
						else if(type.basetype == spirv_cross::SPIRType::BaseType::UInt)
						{
							uniformType = PrimitiveType::Uint32;
						}
						else if(type.basetype == spirv_cross::SPIRType::BaseType::Short)
						{
							uniformType = PrimitiveType::Int16;
						}
						else if(type.basetype == spirv_cross::SPIRType::BaseType::UShort)
						{
							uniformType = PrimitiveType::Uint16;
						}
						else if(type.basetype == spirv_cross::SPIRType::BaseType::SByte)
						{
							uniformType = PrimitiveType::Int8;
						}
						else if(type.basetype == spirv_cross::SPIRType::BaseType::UByte)
						{
							uniformType = PrimitiveType::Uint8;
						}
					}

					UniformDescriptor *descriptor = new UniformDescriptor(name, uniformType, instanceAttributeOffset, arrayElementCount, location);
					instanceAttributeOffset += descriptor->GetSize(); //Might need some alignment rules? I guess I'll find out when I use it...
					instanceAttributes->AddObject(descriptor->Autorelease());
				}
			}

			if(instanceAttributes->GetCount() > 0)
			{
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR("_InstanceAttributes_"), 0, instanceAttributes->Autorelease(), ArgumentBuffer::Type::InstanceAttributesBuffer, 0);
				_instancingAttributes = argumentBuffer;
			}
			else
			{
				instanceAttributes->Release();
			}
		}

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
				argumentSampler = new ArgumentSampler(name, binding, ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Greater);
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

		for(auto &resource : resources.storage_buffers)
		{
			Array *uniformDescriptors = new Array();

			auto type = reflector.get_type(resource.base_type_id);
			unsigned memberCount = type.member_types.size();

			//TODO: This is a bit of a hack to get the struct inside an instanced uniform buffer / structuredbuffer in the hlsl shader, should change to work more generally
            if(memberCount != 1) continue;
			type = reflector.get_type(type.member_types[0]);
            memberCount = type.member_types.size();

			for(size_t index = 0; index < memberCount; index++)
			{
				size_t offset = reflector.type_struct_member_offset(type, index);

                const std::string &memberName = reflector.get_member_name(type.self, index);
                String *name = RNSTR(memberName);

                if(name->GetLength() == 0) break;

                spirv_cross::SPIRType spirvUniformType = reflector.get_type(type.member_types[index]);

				//Set array element count to the real number, but default to 1 for all cases
				size_t arrayElementCount = 1;
				if(spirvUniformType.array.size() == 1) //There are more than one, if it is multidimensional, but only support 1 dimension for now
				{
					arrayElementCount = spirvUniformType.array[0];
				}

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
						if(spirvUniformType.vecsize == 2) uniformType = PrimitiveType::Matrix2x2;
						if(spirvUniformType.vecsize == 3) uniformType = PrimitiveType::Matrix3x3;
						if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::Matrix4x4;
					}
				}
				else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Half)
				{
					if(spirvUniformType.columns == 1)
					{
						if(spirvUniformType.vecsize == 1) uniformType = PrimitiveType::Half;
						else if(spirvUniformType.vecsize == 2) uniformType = PrimitiveType::HalfVector2;
						else if(spirvUniformType.vecsize == 3) uniformType = PrimitiveType::HalfVector3;
						else if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::HalfVector4;
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

				UniformDescriptor *descriptor = new UniformDescriptor(name, uniformType, offset, arrayElementCount);
				uniformDescriptors->AddObject(descriptor->Autorelease());
			}

			if(uniformDescriptors->GetCount() > 0)
			{
				uint32 binding = reflector.get_decoration(resource.id, spv::DecorationBinding);
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR(resource.name), binding, uniformDescriptors->Autorelease(), ArgumentBuffer::Type::StorageBuffer, 0);
				buffersArray->AddObject(argumentBuffer->Autorelease());
			}
			else
			{
				uniformDescriptors->Release();
			}
		}

		for(auto &resource : resources.uniform_buffers)
		{
			Array *uniformDescriptors = new Array();
			size_t maxInstanceCount = 1;

			spirv_cross::SPIRType type = reflector.get_type(resource.base_type_id);
			unsigned memberCount = type.member_types.size();
            size_t index = 0;
			while(index < memberCount)
			{
				size_t offset = reflector.type_struct_member_offset(type, index);

				const std::string &memberName = reflector.get_member_name(type.self, index);
				String *name = RNSTR(memberName);

				if(name->GetLength() == 0)
                {
                    index++;
                    break;
                }

				spirv_cross::SPIRType spirvUniformType = reflector.get_type(type.member_types[index]);

				//Set array element count to the real number, but default to 1 for all cases
				size_t arrayElementCount = 1;
				if(spirvUniformType.array.size() == 1) //There are more than one, if it is multidimensional, but only support 1 dimension for now
				{
					arrayElementCount = spirvUniformType.array[0];
				}

				//TODO: This assumes that any single unknown array of structs inside a uniform buffer contains per instance data, should make this better...
				if(memberCount == 1 && spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Struct && !UniformDescriptor::IsKnownStructName(name))
				{
					index = 0;
					type = spirvUniformType;
					memberCount = type.member_types.size();
					maxInstanceCount = arrayElementCount;
					continue;
				}

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
						if(spirvUniformType.vecsize == 2) uniformType = PrimitiveType::Matrix2x2;
						if(spirvUniformType.vecsize == 3) uniformType = PrimitiveType::Matrix3x3;
						if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::Matrix4x4;
					}
				}
				else if(spirvUniformType.basetype == spirv_cross::SPIRType::BaseType::Half)
				{
					if(spirvUniformType.columns == 1)
					{
						if(spirvUniformType.vecsize == 1) uniformType = PrimitiveType::Half;
						else if(spirvUniformType.vecsize == 2) uniformType = PrimitiveType::HalfVector2;
						else if(spirvUniformType.vecsize == 3) uniformType = PrimitiveType::HalfVector3;
						else if(spirvUniformType.vecsize == 4) uniformType = PrimitiveType::HalfVector4;
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

				UniformDescriptor *descriptor = new UniformDescriptor(name, uniformType, offset, arrayElementCount);
				uniformDescriptors->AddObject(descriptor->Autorelease());

                index++;
			}

			if(uniformDescriptors->GetCount() > 0)
			{
				uint32 binding = reflector.get_decoration(resource.id, spv::DecorationBinding);
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR(resource.name), binding, uniformDescriptors->Autorelease(), ArgumentBuffer::Type::UniformBuffer, maxInstanceCount);
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
		SafeRelease(_instancingAttributes);
	}

	const String *VulkanShader::GetName() const
	{
		return _name;
	}
}
