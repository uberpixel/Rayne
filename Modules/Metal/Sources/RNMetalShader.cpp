//
//  RNMetalShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalShader.h"

namespace RN
{
	RNDefineMeta(MetalShader, Shader)

	MetalShader::MetalShader(void *shader) :
		_shader(shader),
		_attributes(new Array())
	{
		// We don't need to retain the shader because it was created
		// with [newFunctionWithName:] which returns an explicitly
		// owned object

		id<MTLFunction> function = (id<MTLFunction>)_shader;
		switch([function functionType])
		{
			case MTLFunctionTypeFragment:
				SetType(Type::Fragment);
				break;
			case MTLFunctionTypeVertex:
				SetType(Type::Vertex);
				break;
			case MTLFunctionTypeKernel:
				SetType(Type::Compute);
				break;
		}

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

				MetalAttribute *attributeCopy = new MetalAttribute(RNSTR([[attribute name] UTF8String]), type, [attribute attributeIndex]);
				_attributes->AddObject(attributeCopy);
				attributeCopy->Release();
			}
		}

	}
	MetalShader::~MetalShader()
	{
		id<MTLFunction> function = (id<MTLFunction>)_shader;
		[function release];

		_attributes->Release();
	}

	const String *MetalShader::GetName() const
	{
		id<MTLFunction> function = (id<MTLFunction>)_shader;
		NSString *name = [function name];

		return RNSTR([name UTF8String]);
	}

	const Array *MetalShader::GetAttributes() const
	{
		return _attributes;
	}
}
