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

	MetalShader::MetalShader(ShaderLibrary *library, Type type, const ShaderOptions *options, void *shader) :
		Shader(library, type, options),
		_shader(shader)
	{
		// We don't need to retain the shader because it was created
		// with [newFunctionWithName:] which returns an explicitly
		// owned object
	}

	MetalShader::~MetalShader()
	{
		id<MTLFunction> function = (id<MTLFunction>)_shader;
		[function release];
	}

	const String *MetalShader::GetName() const
	{
		id<MTLFunction> function = (id<MTLFunction>)_shader;
		NSString *name = [function name];

		return RNSTR([name UTF8String]);
	}

	void MetalShader::SetReflectedArguments(NSArray *arguments)
	{
		//TODO: Support custom arguments
		//TODO: Support more than one uniform buffer

		uint8 samplerCount = 0;
		uint8 textureCount = 0;
		Array *uniformDescriptors = new Array();

		for(MTLArgument *argument in arguments)
		{
			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
				{
					MTLStructType *structType = [argument
					bufferStructType];
					for(MTLStructMember *member in [structType members])
					{
						String *name = RNSTR([[member name] UTF8String])->Retain();
						uint32 offset = [member
						offset];

						Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, offset);
						//offset += descriptor->GetSize();
						uniformDescriptors->AddObject(descriptor->Autorelease());
					}
					break;
				}

				case MTLArgumentTypeTexture:
					textureCount += 1;
				break;

				case MTLArgumentTypeSampler:
					samplerCount += 1;
				break;

				default:
					break;
			}
		}

		Signature *signature = new Signature(uniformDescriptors, samplerCount, textureCount);
		SetSignature(signature->Autorelease());
	}
}
