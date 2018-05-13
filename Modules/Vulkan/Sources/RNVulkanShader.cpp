//
//  RNVulkanShader.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanShader.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanShader, Shader)

	VulkanShader::VulkanShader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, const Shader::Options *options, const Array *samplers) :
		Shader(library, type, options), _name(entryPoint->Retain())
	{
		VulkanRenderer *renderer = VulkanRenderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();

		Data *shaderData = Data::WithContentsOfFile(fileName);

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
		_shaderStage.pName = entryPoint->GetUTF8String();


/*		std::vector<D3D_SHADER_MACRO> shaderDefines;
		options->GetDefines()->Enumerate<String, String>([&](String *value, const String *key, bool &stop) {
			shaderDefines.push_back({key->GetUTF8String(), value->GetUTF8String()});
		});

		shaderDefines.push_back({0, 0});

		_shader = nullptr;
		ID3DBlob *error = nullptr;
		HRESULT success = D3DCompile(shaderData->GetBytes(), shaderData->GetLength(), text, shaderDefines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint->GetUTF8String(), shaderTarget->GetUTF8String(), compileFlags, 0, &_shader, &error);

		if(FAILED(success))
		{
			if(_shader)
				_shader->Release();

			_shader = nullptr;

			String *errorString = RNCSTR("");
			if(error)
			{
				errorString = RNSTR((char*)error->GetBufferPointer());
				error->Release();
			}

			RNDebug(RNSTR("Failed to compile shader: " << fileName << " with error: " << errorString));
			throw ShaderCompilationException(RNSTR("Failed to compile shader: " << fileName << " with error: " << errorString));
		}

		ID3D12ShaderReflection* pReflector = nullptr;
		D3DReflect(_shader->GetBufferPointer(), _shader->GetBufferSize(), IID_PPV_ARGS(&pReflector));

		uint8 textureCount = 0;
		Array *reflectionSamplers = new Array();
		Array *specificReflectionSamplers = new Array();
		Array *uniformDescriptors = new Array();

		D3D12_SHADER_DESC shaderDescription;
		pReflector->GetDesc(&shaderDescription);

		_wantsDirectionalShadowTexture = false;

		for (UINT i = 0; i < shaderDescription.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC resourceBindingDescription;
			pReflector->GetResourceBindingDesc(i, &resourceBindingDescription);

			if(resourceBindingDescription.Type == D3D_SIT_TEXTURE)
			{
				//TODO: Move this into the shader base class
				String *name = RNSTR(resourceBindingDescription.Name);
				if(name->IsEqual(RNCSTR("directionalShadowTexture")))
				{
					//TODO: Store the register, so it doesn't have to be declared last in the shader
					_wantsDirectionalShadowTexture = true;
				}

				textureCount += 1;
			}
			else if(resourceBindingDescription.Type == D3D_SIT_SAMPLER)
			{
				//TODO: Move this into the shader base class
				String *name = RNSTR(resourceBindingDescription.Name);
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
		}

		for(UINT i = 0; i < shaderDescription.ConstantBuffers; i++)
		{
			ID3D12ShaderReflectionConstantBuffer* pConstBuffer = pReflector->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC bufferDescription;
			pConstBuffer->GetDesc(&bufferDescription);

			// Load the description of each variable for use later on when binding a buffer
			for(UINT j = 0; j < bufferDescription.Variables; j++)
			{
				// Get the variable description
				ID3D12ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC variableDescription;
				pVariable->GetDesc(&variableDescription);

				// Get the variable type description
				ID3D12ShaderReflectionType* variableType = pVariable->GetType();
				D3D12_SHADER_TYPE_DESC variableTypeDescription;
				variableType->GetDesc(&variableTypeDescription);

				String *name = RNSTR(variableDescription.Name)->Retain();
				uint32 offset = variableDescription.StartOffset;
				UniformDescriptor *descriptor = new UniformDescriptor(name, offset);
				uniformDescriptors->AddObject(descriptor->Autorelease());
			}
		}

		pReflector->Release();


		if(samplers->GetCount() > 0)
		{
			RN_ASSERT(reflectionSamplers->GetCount() == samplers->GetCount(), "Sampler count missmatch!");
			reflectionSamplers->RemoveAllObjects();
			reflectionSamplers->AddObjectsFromArray(samplers);
		}

		reflectionSamplers->AddObjectsFromArray(specificReflectionSamplers);

		Signature *signature = new Signature(uniformDescriptors->Autorelease(), reflectionSamplers->Autorelease(), textureCount);
		Shader::SetSignature(signature->Autorelease());*/
	}

	VulkanShader::~VulkanShader()
	{
		VulkanRenderer *renderer = VulkanRenderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		VulkanDevice *device = renderer->GetVulkanDevice();
		vk::DestroyShaderModule(device->GetDevice(), _module, renderer->GetAllocatorCallback());

		_name->Release();
	}

	const String *VulkanShader::GetName() const
	{
		return _name;
	}
}
