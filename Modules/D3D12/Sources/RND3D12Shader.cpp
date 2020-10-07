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
		if(fileName->HasSuffix(RNCSTR(".cso")))
		{
			Data *shaderData = Data::WithContentsOfFile(fileName);
			if(!shaderData || FAILED(D3DCreateBlob(shaderData->GetLength(), &_shader)))
			{
				RNDebug(RNSTR("Failed to create blobb for shader: " << fileName));
				throw ShaderCompilationException(RNSTR("Failed to create blobb for shader: " << fileName));
			}

			shaderData->GetBytesInRange(_shader->GetBufferPointer(), Range(0, shaderData->GetLength()));
		}
		else
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
					shaderTarget = RNCSTR("vs_5_1");
					break;
				case Type::Fragment:
					shaderTarget = RNCSTR("ps_5_1");
					break;
				case Type::Compute:
					shaderTarget = RNCSTR("cs_5_1");
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
		}

		ID3D12ShaderReflection* pReflector = nullptr;
		D3DReflect(_shader->GetBufferPointer(), _shader->GetBufferSize(), IID_PPV_ARGS(&pReflector));

		//TODO: Support custom uniforms

		Array *buffersArray = new Array();
		Array *samplersArray = new Array();
		Array *texturesArray = new Array();
		
		D3D12_SHADER_DESC shaderDescription;
		pReflector->GetDesc(&shaderDescription);

		for (UINT i = 0; i < shaderDescription.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC resourceBindingDescription;
			pReflector->GetResourceBindingDesc(i, &resourceBindingDescription);

			if(resourceBindingDescription.Type == D3D_SIT_TEXTURE)
			{
				String *name = RNSTR(resourceBindingDescription.Name);
				uint8 materialTextureIndex = 0;

				//TODO: Move this into the shader base class
				if(name->IsEqual(RNCSTR("directionalShadowTexture")))
				{
					materialTextureIndex = ArgumentTexture::IndexDirectionalShadowTexture;
				}
				else if(name->IsEqual(RNCSTR("framebufferTexture")))
				{
					materialTextureIndex = ArgumentTexture::IndexFramebufferTexture;
				}
				else if(name->HasPrefix(RNCSTR("texture")))
				{
					String *indexString = name->GetSubstring(Range(7, name->GetLength() - 7));
					materialTextureIndex = std::stoi(indexString->GetUTF8String());
				}

				ArgumentTexture *argumentTexture = new ArgumentTexture(name, resourceBindingDescription.BindPoint, materialTextureIndex);
				texturesArray->AddObject(argumentTexture->Autorelease());
			}
			else if(resourceBindingDescription.Type == D3D_SIT_SAMPLER)
			{
				//TODO: Move this into the shader base class
				String *name = RNSTR(resourceBindingDescription.Name);
				ArgumentSampler *argumentSampler = nullptr;
				if (name->IsEqual(RNCSTR("directionalShadowSampler")))
				{
					argumentSampler = new ArgumentSampler(name, resourceBindingDescription.BindPoint, ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Less);
				}
				else //TODO: Pre define some special names like linearRepeatSampler
				{
					samplers->Enumerate<ArgumentSampler>([&](ArgumentSampler *sampler, size_t index, bool &stop) {
						if (sampler->GetName()->IsEqual(name))
						{
							argumentSampler = sampler->Copy();
							argumentSampler->SetIndex(resourceBindingDescription.BindPoint);
							stop = true;
						}
					});

					if (!argumentSampler)
					{
						argumentSampler = new ArgumentSampler(name, resourceBindingDescription.BindPoint);
					}
				}

				samplersArray->AddObject(argumentSampler->Autorelease());
			}
		}

		for(UINT i = 0; i < shaderDescription.ConstantBuffers; i++)
		{
			Array *uniformDescriptors = new Array();
			
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

			if (uniformDescriptors->GetCount() > 0)
			{
				D3D12_SHADER_INPUT_BIND_DESC resourceBindingDescription;
				pReflector->GetResourceBindingDescByName(bufferDescription.Name, &resourceBindingDescription);
				
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR(bufferDescription.Name), resourceBindingDescription.BindPoint, uniformDescriptors->Autorelease());
				buffersArray->AddObject(argumentBuffer->Autorelease());
			}
			else
			{
				uniformDescriptors->Release();
			}
		}

		pReflector->Release();

		Signature *signature = new Signature(buffersArray->Autorelease(), samplersArray->Autorelease(), texturesArray->Autorelease());
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
