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
		
		for(uint32 i = 0; i < static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom) + 1; i++)
		{
			_hasInputVertexAttribute[i] = -1;
		}
		
		id<MTLFunction> shaderFunction = static_cast<id<MTLFunction>>(_shader);
		[shaderFunction.vertexAttributes enumerateObjectsUsingBlock:^(MTLVertexAttribute * _Nonnull attribute, NSUInteger idx, BOOL * _Nonnull stop) {
		 
			RN::String *name = RNSTR(attribute.name.UTF8String);
			uint32 location = attribute.attributeIndex;
		 
			if(name->IsEqual(RNCSTR("in_var_POSITION")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Vertices)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_NORMAL")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Normals)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_TANGENT")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Tangents)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_COLOR")) || name->IsEqual(RNCSTR("in_var_COLOR0")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color0)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_COLOR1")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Color1)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_TEXCOORD")) || name->IsEqual(RNCSTR("in_var_TEXCOORD0")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords0)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_TEXCOORD1")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::UVCoords1)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_BONEWEIGHTS")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneWeights)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_BONEINDICES")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::BoneIndices)] = location;
			}
			else if (name->IsEqual(RNCSTR("in_var_CUSTOM")))
			{
				_hasInputVertexAttribute[static_cast<uint32>(Mesh::VertexAttribute::Feature::Custom)] = location;
			}
		}];
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
					//RNDebug("buffer: " << [[argument name] UTF8String]);
					MTLStructType *structType = [argument bufferStructType];
					size_t numberOfElements = 0;
					Array *uniformDescriptors = GetBufferStructElements(structType, numberOfElements);
					
					if(uniformDescriptors->GetCount() > 0)
					{
						//numberOfElements will only be > 0 for per instance data. In this case marking it as storage buffer will make the renderer not limit the number of instances per draw call, as metal can handle this just fine (it's different with vulkan on some mobile hardware!)
						ArgumentBuffer *argumentBuffer = new ArgumentBuffer(RNSTR([[argument name] UTF8String]), static_cast<uint32>([argument index]), uniformDescriptors, numberOfElements > 0? ArgumentBuffer::Type::StorageBuffer : ArgumentBuffer::Type::UniformBuffer, numberOfElements > 0? 0 : 1);
						buffersArray->AddObject(argumentBuffer->Autorelease());
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

	Array *MetalShader::GetBufferStructElements(MTLStructType *structType, size_t &numberOfElements)
	{
		Array *uniformDescriptors = new Array();
		uniformDescriptors->Autorelease();
		
		numberOfElements = 0;
		
		for(MTLStructMember *member in [structType members])
		{
			String *name = RNSTR([[member name] UTF8String]);
			uint32 offset = [member offset];
			MTLDataType type = [member dataType];
			
			uint32 arrayElementCount = 1;
			PrimitiveType uniformType = PrimitiveType::Invalid;
			
			//RNDebug("	buffer member: " << name << " type: " << type);
			//If this is an array of structs with unknown name, assume that it is per instance data
			if(type == MTLDataTypeArray && !UniformDescriptor::IsKnownStructName(name))
			{
				MTLArrayType *arrayType = [member arrayType];
				arrayElementCount = arrayType.arrayLength;
				if(arrayType.elementType == MTLDataTypeStruct)
				{
					MTLStructType *otherStructType = [arrayType elementStructType];
					if(otherStructType)
					{
						numberOfElements = arrayElementCount;
						size_t temp = 0;
						return GetBufferStructElements(otherStructType, temp);
					}
				}
			}
			else if(type == MTLDataTypeArray)
			{
				MTLArrayType *arrayType = [member arrayType];
				arrayElementCount = arrayType.arrayLength;
			}
			else
			{
				if(type == MTLDataTypeHalf)
				{
					uniformType = PrimitiveType::Half;
				}
				else if(type == MTLDataTypeHalf2)
				{
					uniformType = PrimitiveType::HalfVector2;
				}
				else if(type == MTLDataTypeHalf3)
				{
					uniformType = PrimitiveType::HalfVector3;
				}
				else if(type == MTLDataTypeHalf4)
				{
					uniformType = PrimitiveType::HalfVector4;
				}
				else if(type == MTLDataTypeFloat)
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
				else if(type == MTLDataTypeFloat2x2)
				{
					uniformType = PrimitiveType::Matrix2x2;
				}
				else if(type == MTLDataTypeFloat3x3)
				{
					uniformType = PrimitiveType::Matrix3x3;
				}
				else if(type == MTLDataTypeFloat4x4)
				{
					uniformType = PrimitiveType::Matrix4x4;
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
			}

			Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, uniformType, offset, arrayElementCount);
			uniformDescriptors->AddObject(descriptor->Autorelease());
		}
		
		return uniformDescriptors;
	}
}
