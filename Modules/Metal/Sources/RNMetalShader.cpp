//
//  RNMetalShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalShader.h"
#include "RNMetalStateCoordinator.h"

namespace RN
{
	RNDefineMeta(MetalShader, Shader)

	MetalShader::MetalShader(ShaderLibrary *library, Type type, const Array *samplers, const Shader::Options *options, void *shader, MetalStateCoordinator *coordinator) :
		Shader(library, type, options),
		_shader(shader),
		_coordinator(coordinator)
	{
		// We don't need to retain the shader because it was created
		// with [newFunctionWithName:] which returns an explicitly
		// owned object
		
		_rnSamplers = samplers->Retain();
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
		Array *buffersArray = new Array();
		Array *samplersArray = new Array();
		Array *texturesArray = new Array();

		for(MTLArgument *argument in arguments)
		{
			switch([argument type])
			{
				case MTLArgumentTypeBuffer:
				{
					Array *uniformDescriptors = new Array();
					
					MTLStructType *structType = [argument
					bufferStructType];
					for(MTLStructMember *member in [structType members])
					{
						String *name = RNSTR([[member name] UTF8String]);
						uint32 offset = [member offset];

						Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, offset);
						uniformDescriptors->AddObject(descriptor->Autorelease());
					}
					
					if(uniformDescriptors->GetCount() > 0)
					{
						ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR([[argument name] UTF8String]), static_cast<uint32>([argument index]), uniformDescriptors->Autorelease());
						buffersArray->AddObject(argumentBuffer->Autorelease());
					}
					else
					{
						uniformDescriptors->Release();
					}
					
					break;
				}

				case MTLArgumentTypeTexture:
				{
					String *name = RNSTR([[argument name] UTF8String]);
					uint8 materialTextureIndex = 0;
					
					//TODO: Move this into the shader base class
					if(name->IsEqual(RNCSTR("directionalShadowTexture")))
					{
						materialTextureIndex = ArgumentTexture::IndexDirectionalShadowTexture;
					}
					else if(name->HasPrefix(RNCSTR("texture")))
					{
						String *indexString = name->GetSubstring(Range(7, name->GetLength() - 7));
						materialTextureIndex = std::stoi(indexString->GetUTF8String());
					}
					
					ArgumentTexture *argumentTexture = new ArgumentTexture(name, static_cast<uint32>([argument index]), materialTextureIndex);
					texturesArray->AddObject(argumentTexture->Autorelease());

					break;
				}

				case MTLArgumentTypeSampler:
				{
					//TODO: Move this into the shader base class
					String *name = RNSTR([[argument name] UTF8String]);
					ArgumentSampler *argumentSampler = nullptr;
					if(name->IsEqual(RNCSTR("directionalShadowSampler")))
					{
						argumentSampler = new ArgumentSampler(RNSTR([[argument name] UTF8String]), static_cast<uint32>([argument index]), ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Less);
						argumentSampler->Autorelease();
					}
					else //TODO: Pre define some special names like linearRepeatSampler
					{
						_rnSamplers->Enumerate<ArgumentSampler>([&](ArgumentSampler *sampler, size_t index, bool &stop){
							if(sampler->GetName()->IsEqual(name))
							{
								argumentSampler = sampler->Copy();
								argumentSampler->SetIndex(static_cast<uint32>([argument index]));
								stop = true;
							}
						});
						
						if(!argumentSampler)
						{
							argumentSampler = new ArgumentSampler(name, static_cast<uint32>([argument index]));
						}
					}
					
					samplersArray->AddObject(argumentSampler->Autorelease());
					
					id<MTLSamplerState> samplerState = [_coordinator->GetSamplerStateForSampler(argumentSampler) retain];
					_samplers.push_back(samplerState);
					_samplerToIndexMapping.push_back([argument index]);

					break;
				}

				default:
					break;
			}
		}

		Signature *signature = new Signature(buffersArray->Autorelease(), samplersArray->Autorelease(), texturesArray->Autorelease());
		SetSignature(signature->Autorelease());
	}
}
