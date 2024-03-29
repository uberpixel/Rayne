//
//  RND3D12Shader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12Shader.h"

#include <Rendering/RNShader.h>

namespace RN
{
	RNDefineMeta(D3D12Shader, Shader)

	D3D12Shader::D3D12Shader(ShaderLibrary *library, const String *fileName, const String *entryPoint, Type type, bool hasInstancing, const Shader::Options *options, const Array *samplers) :
		Shader(library, type, hasInstancing, options), _shader(nullptr), _name(entryPoint->Retain())
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
			options->Enumerate([&](const std::string &value, const std::string &key, bool &stop) {
				shaderDefines.push_back({key.c_str(), value.c_str()});
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
					argumentSampler = new ArgumentSampler(name, resourceBindingDescription.BindPoint, ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Greater);
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
			size_t maxInstanceCount = 1;
			
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

				//TODO: This assumes that any single uinknown array of structs inside a uniform buffer contains per instance data, should make this better...
				if(bufferDescription.Variables == 1 && variableTypeDescription.Class == D3D_SVC_STRUCT && !UniformDescriptor::IsKnownStructName(name))
				{
					maxInstanceCount = variableTypeDescription.Elements;

					for(UINT member = 0; member < variableTypeDescription.Members; member++)
					{
						ID3D12ShaderReflectionType *memberType = variableType->GetMemberTypeByIndex(member);
						D3D12_SHADER_TYPE_DESC memberTypeDescription;
						memberType->GetDesc(&memberTypeDescription);

						String *memberName = RNSTR(variableType->GetMemberTypeName(member))->Retain();

						UniformDescriptor *descriptor = GetUniformDescriptorForReflectionInfo(memberName, memberTypeDescription, offset + memberTypeDescription.Offset);
						uniformDescriptors->AddObject(descriptor);
					}

					break;
				}

				UniformDescriptor *descriptor = GetUniformDescriptorForReflectionInfo(name, variableTypeDescription, offset);
				uniformDescriptors->AddObject(descriptor);
			}

			if (uniformDescriptors->GetCount() > 0)
			{
				D3D12_SHADER_INPUT_BIND_DESC resourceBindingDescription;
				pReflector->GetResourceBindingDescByName(bufferDescription.Name, &resourceBindingDescription);
				
				ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR(bufferDescription.Name), resourceBindingDescription.BindPoint, uniformDescriptors->Autorelease(), ArgumentBuffer::Type::UniformBuffer, maxInstanceCount);
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

	Shader::UniformDescriptor *D3D12Shader::GetUniformDescriptorForReflectionInfo(const String *name, const D3D12_SHADER_TYPE_DESC &variableTypeDescription, uint32 offset) const
	{
		PrimitiveType uniformType = PrimitiveType::Invalid;
		if (variableTypeDescription.Type == D3D_SVT_FLOAT)
		{
			if (variableTypeDescription.Rows == 1)
			{
				if (variableTypeDescription.Columns == 1) uniformType = PrimitiveType::Float;
				else if (variableTypeDescription.Columns == 2) uniformType = PrimitiveType::Vector2;
				else if (variableTypeDescription.Columns == 3) uniformType = PrimitiveType::Vector3;
				else if (variableTypeDescription.Columns == 4) uniformType = PrimitiveType::Vector4;
			}
			else
			{
				if(variableTypeDescription.Columns == 2) uniformType = PrimitiveType::Matrix2x2;
				if(variableTypeDescription.Columns == 3) uniformType = PrimitiveType::Matrix3x3;
				if(variableTypeDescription.Columns == 4) uniformType = PrimitiveType::Matrix4x4;
			}
		}
		else if (variableTypeDescription.Type == D3D_SVT_MIN16FLOAT)
		{
			if (variableTypeDescription.Rows == 1)
			{
				if (variableTypeDescription.Columns == 1) uniformType = PrimitiveType::Half;
				else if (variableTypeDescription.Columns == 2) uniformType = PrimitiveType::HalfVector2;
				else if (variableTypeDescription.Columns == 3) uniformType = PrimitiveType::HalfVector3;
				else if (variableTypeDescription.Columns == 4) uniformType = PrimitiveType::HalfVector4;
			}
		}
		else if (variableTypeDescription.Columns == 1 && variableTypeDescription.Rows == 1)
		{
			if (variableTypeDescription.Type == D3D_SVT_INT)
			{
				uniformType = PrimitiveType::Int32;
			}
			else if (variableTypeDescription.Type == D3D_SVT_UINT)
			{
				uniformType = PrimitiveType::Uint32;
			}
			else if (variableTypeDescription.Type == D3D_SVT_MIN16INT)
			{
				uniformType = PrimitiveType::Int16;
			}
			else if (variableTypeDescription.Type == D3D_SVT_MIN16UINT)
			{
				uniformType = PrimitiveType::Uint16;
			}
/*			else if(variableTypeDescription.Type == D3D_SVT_INT8) //Not supported!?
			{
				uniformType = PrimitiveType::Int8;
			}*/
			else if (variableTypeDescription.Type == D3D_SVT_UINT8)
			{
				uniformType = PrimitiveType::Uint8;
			}
		}

		size_t arrayElementCount = variableTypeDescription.Elements;
		if(arrayElementCount == 0) arrayElementCount = 1; //It will be 0 if not an array, force it to 1 in that case
		UniformDescriptor *descriptor = new UniformDescriptor(name, uniformType, offset, arrayElementCount);
		return descriptor->Autorelease();
	}
}
