//
//  RND3D12Shader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Shader.h"

namespace RN
{
	RNDefineMeta(D3D12Shader, Shader)

	D3D12Shader::D3D12Shader(String *file, String *entryPointName, String *shaderType) :
		_attributes(new Array())
	{
		if(shaderType->IsEqual(RNCSTR("vertex")))
		{
			SetType(Type::Vertex);
		}
		else if(shaderType->IsEqual(RNCSTR("fragment")))
		{
			SetType(Type::Fragment);
		}
		else if(shaderType->IsEqual(RNCSTR("compute")))
		{
			SetType(Type::Compute);
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
