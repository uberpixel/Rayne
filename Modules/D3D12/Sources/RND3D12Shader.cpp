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

	D3D12Shader::D3D12Shader(String *file, String *entryPointName, String *shaderType) :
		_attributes(new Array()), _shader(nullptr)
	{
#ifdef _DEBUG
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		String *shaderTarget;
		if(shaderType->IsEqual(RNCSTR("vertex")))
		{
			shaderTarget = RNCSTR("vs_5_0");
			SetType(Type::Vertex);
		}
		else if(shaderType->IsEqual(RNCSTR("fragment")))
		{
			shaderTarget = RNCSTR("ps_5_0");
			SetType(Type::Fragment);
		}
		else if(shaderType->IsEqual(RNCSTR("compute")))
		{
			shaderTarget = RNCSTR("cs_5_0");
			SetType(Type::Compute);
		}

		Data *shaderData = Data::WithContentsOfFile(file);
		char *text = file->GetUTF8String();

		ID3DBlob *shader = nullptr;
		ID3DBlob *error = nullptr;
		HRESULT success = D3DCompile(shaderData->GetBytes(), shaderData->GetLength(), text, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPointName->GetUTF8String(), shaderTarget->GetUTF8String(), compileFlags, 0, &shader, &error);
		_shader = shader;

		if(FAILED(success))
		{
			if(shader)
				shader->Release();

			String *errorString = RNCSTR("");
			if(error)
			{
				errorString = RNSTR((char*)error->GetBufferPointer());
				error->Release();
			}

			RNDebug(RNSTR("Failed to compile shader: " << file << " with error: " << errorString));
			throw ShaderCompilationException(RNSTR("Failed to compile shader: " << file << " with error: " << errorString));
		}
		
/*
		NSArray *attributes = [function vertexAttributes];
		size_t count = [attributes count];

		for(size_t i = 0; i < count; i ++)
		{
			MTLVertexAttribute *attribute = [attributes objectAtIndex:i];
			if([attribute isActive])
			{
				PrimitiveType type;
				switch([attribute attributeType])
				{
					case MTLDataTypeFloat:
						type = PrimitiveType::Float;
						break;
					case MTLDataTypeFloat2:
						type = PrimitiveType::Vector2;
						break;
					case MTLDataTypeFloat3:
						type = PrimitiveType::Vector3;
						break;
					case MTLDataTypeFloat4:
						type = PrimitiveType::Vector4;
						break;

					case MTLDataTypeFloat4x4:
						type = PrimitiveType::Matrix;
					break;

					case MTLDataTypeInt:
						type = PrimitiveType::Int32;
						break;
					case MTLDataTypeUInt:
						type = PrimitiveType::Uint32;
						break;

					case MTLDataTypeShort:
						type = PrimitiveType::Int16;
						break;
					case MTLDataTypeUShort:
						type = PrimitiveType::Uint16;
						break;

					case MTLDataTypeChar:
						type = PrimitiveType::Int8;
						break;
					case MTLDataTypeUChar:
						type = PrimitiveType::Uint8;
						break;

					default:
						continue;
				}

				D3D12Attribute *attributeCopy = new D3D12Attribute(RNSTR([[attribute name] UTF8String]), type, [attribute attributeIndex]);
				_attributes->AddObject(attributeCopy);
				attributeCopy->Release();
			}
		}*/
	}

	D3D12Shader::~D3D12Shader()
	{
/*		id<MTLFunction> function = (id<MTLFunction>)_shader;
		[function release];

		_attributes->Release();*/
	}

	const String *D3D12Shader::GetName() const
	{
/*		id<MTLFunction> function = (id<MTLFunction>)_shader;
		NSString *name = [function name];

		return RNSTR([name UTF8String]);*/

		return nullptr;
	}

	const Array *D3D12Shader::GetAttributes() const
	{
		return _attributes;
	}
}
