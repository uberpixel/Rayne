//
//  RND3D12Shader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12Shader.h"

namespace RN
{
	RNDefineMeta(D3D12Shader, Shader)

	D3D12Shader::D3D12Shader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, const Shader::Options *options, const Array *samplers) :
		Shader(library, type, options), _shader(nullptr), _name(entryPoint->Retain())
	{
#ifdef _DEBUG
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		String *shaderTarget;

		switch(GetType())
		{
			case Type::Vertex:
				shaderTarget = RNCSTR("vs_5_0");
				break;
			case Type::Fragment:
				shaderTarget = RNCSTR("ps_5_0");
				break;
			case Type::Compute:
				shaderTarget = RNCSTR("cs_5_0");
				break;
		}

		Data *shaderData = Data::WithContentsOfFile(fileName);
		char *text = fileName->GetUTF8String();

		std::vector<D3D_SHADER_MACRO> shaderDefines;
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
		Shader::SetSignature(signature->Autorelease());
	}

	D3D12Shader::~D3D12Shader()
	{
		_shader->Release();
		_name->Release();
	}

	const String *D3D12Shader::GetName() const
	{
		return _name;
	}
}
