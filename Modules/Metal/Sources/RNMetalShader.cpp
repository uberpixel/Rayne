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

	MetalShader::MetalShader(ShaderLibrary *library, Type type, bool hasInstancing, const Array *samplers, const Shader::Options *options, void *shader, MetalStateCoordinator *coordinator) :
		Shader(library, type, hasInstancing, options),
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
		//TODO: Support custom uniformsdirectionalLights
		
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
					
					//RNDebug("buffer: " << [[argument name] UTF8String]);
					MTLStructType *structType = [argument bufferStructType];
					bool isInstanceBuffer = false;
					AddBufferStructElements(uniformDescriptors, structType, isInstanceBuffer);
					
					if(uniformDescriptors->GetCount() > 0)
					{
						ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR([[argument name] UTF8String]), static_cast<uint32>([argument index]), uniformDescriptors->Autorelease(), isInstanceBuffer? ArgumentBuffer::Type::StorageBuffer : ArgumentBuffer::Type::UniformBuffer);
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
						argumentSampler = new ArgumentSampler(name, static_cast<uint32>([argument index]), ArgumentSampler::WrapMode::Clamp, ArgumentSampler::Filter::Linear, ArgumentSampler::ComparisonFunction::Greater);
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

	void MetalShader::AddBufferStructElements(Array *uniformDescriptors, MTLStructType *structType, bool &isInstanceBuffer)
	{
		for(MTLStructMember *member in [structType members])
		{
			String *name = RNSTR([[member name] UTF8String]);
			uint32 offset = [member offset];
			MTLDataType type = [member dataType];
			
			//RNDebug("	buffer member: " << name << " type: " << type);
			//If this is an array of structs with unknown name, assume that it is per instance data
			if(type == MTLDataTypeArray && !UniformDescriptor::IsKnownStructName(name))
			{
				MTLArrayType *arrayType = [member arrayType];
				if(arrayType.elementType == MTLDataTypeStruct)
				{
					MTLStructType *otherStructType = [arrayType elementStructType];
					if(otherStructType)
					{
						isInstanceBuffer = true;
						AddBufferStructElements(uniformDescriptors, otherStructType, isInstanceBuffer);
					}
				}
				return;
			}
			
			PrimitiveType uniformType = PrimitiveType::Invalid;
			if(type == MTLDataTypeFloat)
			{
				uniformType = PrimitiveType::Float;
			}
			else if(type == MTLDataTypeFloat2)
			{
				uniformType = PrimitiveType::Vector2;
			}
			else if(type == MTLDataTypeFloat3)
			{
				uniformType = PrimitiveType::Vector3;
			}
			else if(type == MTLDataTypeFloat4)
			{
				uniformType = PrimitiveType::Vector4;
			}
			else if(type == MTLDataTypeFloat4x4)
			{
				uniformType = PrimitiveType::Matrix;
			}
			else if(type == MTLDataTypeInt)
			{
				uniformType = PrimitiveType::Int32;
			}
			else if(type == MTLDataTypeUInt)
			{
				uniformType = PrimitiveType::Uint32;
			}
			else if(type == MTLDataTypeShort)
			{
				uniformType = PrimitiveType::Int16;
			}
			else if(type == MTLDataTypeUShort)
			{
				uniformType = PrimitiveType::Uint16;
			}
			else if(type == MTLDataTypeChar)
			{
				uniformType = PrimitiveType::Int8;
			}
			else if(type == MTLDataTypeUChar)
			{
				uniformType = PrimitiveType::Uint8;
			}

			Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, uniformType, offset);
			uniformDescriptors->AddObject(descriptor->Autorelease());
		}
	}
}
