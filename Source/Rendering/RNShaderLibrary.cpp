//
//  RNShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderLibrary.h"
#include "../Objects/RNNumber.h"
#include "../Math/RNAlgorithm.h"
#include "../Rendering/RNMesh.h"
#include "../Rendering/RNMaterial.h"

namespace RN
{
	RNDefineMeta(ShaderLibrary, Object)

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{

	}

	Array *ShaderLibrary::GetSamplers(const Array *samplers)
	{
		Array *samplerArray = new Array();
		if(samplers)
		{
			samplers->Enumerate([&](Object *sampler, size_t index, bool &stop) {
				Dictionary *dict = sampler->Downcast<Dictionary>();
				if(dict)
				{
					String *name = dict->GetObjectForKey<String>(RNCSTR("name"));
					String *wrap = dict->GetObjectForKey<String>(RNCSTR("wrap"));
					String *filter = dict->GetObjectForKey<String>(RNCSTR("filter"));
					String *comparison = dict->GetObjectForKey<String>(RNCSTR("comparison"));
					Number *anisotropy = dict->GetObjectForKey<Number>(RNCSTR("anisotropy"));
					
					Shader::ArgumentSampler::WrapMode wrapMode = Shader::ArgumentSampler::WrapMode::Repeat;
					Shader::ArgumentSampler::Filter filterType = Shader::ArgumentSampler::Filter::Anisotropic;
					Shader::ArgumentSampler::ComparisonFunction comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Never;
					uint8 anisotropyValue = Shader::ArgumentSampler::GetDefaultAnisotropy();
					
					if(wrap)
					{
						if(wrap->IsEqual(RNCSTR("clamp")))
						{
							wrapMode = Shader::ArgumentSampler::WrapMode::Clamp;
						}
					}
					
					if(filter)
					{
						if(filter->IsEqual(RNCSTR("nearest")))
						{
							filterType = Shader::ArgumentSampler::Filter::Nearest;
						}
						else if(filter->IsEqual(RNCSTR("linear")))
						{
							filterType = Shader::ArgumentSampler::Filter::Linear;
						}
					}
					
					if(comparison)
					{
						if(comparison->IsEqual(RNCSTR("never")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Never;
						}
						else if(comparison->IsEqual(RNCSTR("less")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Less;
						}
						else if(comparison->IsEqual(RNCSTR("lessequal")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::LessEqual;
						}
						else if(comparison->IsEqual(RNCSTR("equal")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Equal;
						}
						else if(comparison->IsEqual(RNCSTR("notequal")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::NotEqual;
						}
						else if(comparison->IsEqual(RNCSTR("greaterequal")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::GreaterEqual;
						}
						else if(comparison->IsEqual(RNCSTR("greater")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Greater;
						}
						else if(comparison->IsEqual(RNCSTR("always")))
						{
							comparisonFunction = Shader::ArgumentSampler::ComparisonFunction::Always;
						}
					}
					
					if(anisotropy)
					{
						anisotropyValue = anisotropy->GetUint32Value();
					}
					
					Shader::ArgumentSampler *sampler = new Shader::ArgumentSampler(name, -1, wrapMode, filterType, comparisonFunction, anisotropyValue);
					samplerArray->AddObject(sampler->Autorelease());
				}
			});
		}
		
		return samplerArray->Autorelease();
	}
}
