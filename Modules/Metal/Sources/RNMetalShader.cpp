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
		_coordinator(coordinator),
		_wantsDirectionalShadowTexture(false)
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
		//TODO: Support more than one uniform buffer

		uint8 textureCount = 0;
		Array *samplers = new Array();
		Array *specificSamplers = new Array();
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
						String *name = RNSTR([[member name] UTF8String]);
						uint32 offset = [member offset];

						Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, offset);
						uniformDescriptors->AddObject(descriptor->Autorelease());
					}
					break;
				}

				case MTLArgumentTypeTexture:
				{
					//TODO: Move this into the shader base class
					String *name = RNSTR([[argument name] UTF8String]);
					if(name->IsEqual(RNCSTR("directionalShadowTexture")))
					{
						//TODO: Store the register, so it doesn't have to be declared last in the shader
						_wantsDirectionalShadowTexture = true;
					}

					textureCount += 1;
					break;
				}

				case MTLArgumentTypeSampler:
				{
					//TODO: Move this into the shader base class
					String *name = RNSTR([[argument name] UTF8String]);
					if(name->IsEqual(RNCSTR("directionalShadowSampler")))
					{
						//TODO: Store the register, so it doesn't have to be declared last in the shader
						Sampler *sampler = new Sampler(Sampler::WrapMode::Clamp, Sampler::Filter::Linear, Sampler::ComparisonFunction::Less);
						specificSamplers->AddObject(sampler->Autorelease());
					}
					else
					{
						//TODO: Allow other than default samplers
						Sampler *sampler = new Sampler();
						samplers->AddObject(sampler->Autorelease());
					}

					break;
				}

				default:
					break;
			}
		}
		
		if(_rnSamplers->GetCount() > 0)
		{
			RN_ASSERT(samplers->GetCount() <= _rnSamplers->GetCount(), "Sampler count missmatch!");
			samplers->RemoveAllObjects();
			samplers->AddObjectsFromArray(_rnSamplers);
		}

		Signature *signature = new Signature(uniformDescriptors->Autorelease(), samplers->Autorelease(), textureCount);
		SetSignature(signature->Autorelease());

		samplers->Enumerate<Sampler>([&](Sampler *sampler, size_t index, bool &stop){
			id<MTLSamplerState> blubb = [_coordinator->GetSamplerStateForSampler(sampler) retain];
			_samplers.push_back(blubb);
		});

		specificSamplers->Enumerate<Sampler>([&](Sampler *sampler, size_t index, bool &stop){
			id<MTLSamplerState> blubb = [_coordinator->GetSamplerStateForSampler(sampler) retain];
			_samplers.push_back(blubb);
		});
	}
}
