//
//  RNMetalShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalShaderLibrary.h"
#include "RNMetalShader.h"

namespace RN
{
	RNDefineMeta(MetalSpecificShaderLibrary, Object)

	MetalSpecificShaderLibrary::MetalSpecificShaderLibrary(id<MTLDevice> device, const String *fileName, const String *entryPoint, Shader::Type type, Dictionary *signatureDescription) :
	_shaders(new Dictionary()),
	_entryPoint(entryPoint->Retain()),
	_fileName(fileName->Retain()),
	_type(type),
	_signatureDescription(signatureDescription)
	{
		if(_signatureDescription) _signatureDescription->Retain();
	}

	MetalSpecificShaderLibrary::~MetalSpecificShaderLibrary()
	{
		_fileName->Release();
		_entryPoint->Release();
		_shaders->Release();
		_signatureDescription->Release();
	}
	
	const Shader::Options *MetalSpecificShaderLibrary::GetCleanedShaderOptions(const Shader::Options *options) const
	{
		const Dictionary *oldDefines = options->GetDefines();
		Shader::Options *newOptions = Shader::Options::WithNone();
		if(!_signatureDescription)
			return newOptions;
		
		Array *signatureOptions = _signatureDescription->GetObjectForKey<Array>(RNCSTR("options"));
		if(!signatureOptions)
			return newOptions;
		
		signatureOptions->Enumerate([&](Object *option, size_t index, bool &stop) {
			Dictionary *dict = option->Downcast<Dictionary>();
			String *name = nullptr;
			if(!dict)
			{
				name = option->Downcast<String>();
			}
			else
			{
				name = dict->GetObjectForKey<String>(RNCSTR("option"));
			}
			
			if(name)
			{
				String *obj = oldDefines->GetObjectForKey<String>(name);
				if(obj)
				{
					newOptions->AddDefine(name, obj);
				}
			}
		});
		
		return newOptions;
	}
	
	static Array *GetUniformDescriptors(const Array *uniforms, uint32 &offset)
	{
		Array *uniformDescriptors = new Array();
		if(uniforms)
		{
			uniforms->Enumerate([&](Object *uniform, size_t index, bool &stop) {
				String *name = uniform->Downcast<String>();
				if(!name)
				{
					Dictionary *dict = uniform->Downcast<Dictionary>();
					if(dict)
					{
						//TODO: WAHHHHH implement custom uniforms
					}
				}
				else
				{
					Shader::UniformDescriptor *descriptor = new Shader::UniformDescriptor(name, offset);
					offset += descriptor->GetSize();
					uniformDescriptors->AddObject(descriptor->Autorelease());
				}
			});
		}
		
		return uniformDescriptors->Autorelease();
	}
	
	static Array *GetSamplers(const Array *samplers)
	{
		Array *samplerArray = new Array();
		if(samplers)
		{
			samplers->Enumerate([&](Object *sampler, size_t index, bool &stop) {
				String *name = sampler->Downcast<String>();
				if(!name)
				{
					Dictionary *dict = sampler->Downcast<Dictionary>();
					if(dict)
					{
						String *wrap = dict->GetObjectForKey<String>(RNCSTR("wrap"));
						String *filter = dict->GetObjectForKey<String>(RNCSTR("filter"));
						Number *anisotropy = dict->GetObjectForKey<Number>(RNCSTR("anisotropy"));
						
						Shader::Sampler::WrapMode wrapMode = Shader::Sampler::WrapMode::Repeat;
						Shader::Sampler::Filter filterType = Shader::Sampler::Filter::Anisotropic;
						uint8 anisotropyValue = Shader::Sampler::GetDefaultAnisotropy();
						
						if(wrap)
						{
							if(wrap->IsEqual(RNCSTR("clamp")))
							{
								wrapMode = Shader::Sampler::WrapMode::Clamp;
							}
						}
						
						if(filter)
						{
							if(filter->IsEqual(RNCSTR("nearest")))
							{
								filterType = Shader::Sampler::Filter::Nearest;
							}
							else if(filter->IsEqual(RNCSTR("linear")))
							{
								filterType = Shader::Sampler::Filter::Linear;
							}
						}
						
						if(anisotropy)
						{
							anisotropyValue = anisotropy->GetUint32Value();
						}
						
						//TODO: read comparison function from json
						Shader::Sampler *sampler = new Shader::Sampler(wrapMode, filterType, Shader::Sampler::ComparisonFunction::Never, anisotropyValue);
						samplerArray->AddObject(sampler->Autorelease());
					}
				}
				else
				{
					if(name->IsEqual(RNCSTR("default")))
					{
						Shader::Sampler *sampler = new Shader::Sampler();
						samplerArray->AddObject(sampler->Autorelease());
					}
				}
			});
		}
		
		return samplerArray->Autorelease();
	}
	
	const Array *MetalSpecificShaderLibrary::GetSamplerSignature(const Shader::Options *options) const
	{
		if(!_signatureDescription)
		{
			Array *samplers = new Array();
			return samplers->Autorelease();
		}
		
		Array *samplerDataArray = _signatureDescription->GetObjectForKey<Array>(RNCSTR("samplers"));
		Array *samplerArray = GetSamplers(samplerDataArray);
		
		Array *signatureOptions = _signatureDescription->GetObjectForKey<Array>(RNCSTR("options"));
		if(signatureOptions)
		{
			signatureOptions->Enumerate([&](Object *option, size_t index, bool &stop) {
				Dictionary *dict = option->Downcast<Dictionary>();
				if(dict)
				{
					String *name = dict->GetObjectForKey<String>(RNCSTR("option"));
					if(!options->GetDefines()->GetObjectForKey(name))
					{
						return;
					}
					Array *optionSamplerdataArray = dict->GetObjectForKey<Array>(RNCSTR("samplers"));
					Array *optionSamplerArray = GetSamplers(optionSamplerdataArray);
					samplerArray->AddObjectsFromArray(optionSamplerArray);
				}
			});
		}
		
		return samplerArray;
	}
	
	Shader *MetalSpecificShaderLibrary::GetShaderWithOptions(id<MTLDevice> device, MetalStateCoordinator *coordinator, ShaderLibrary *library, const Shader::Options *options)
	{
		const Shader::Options *newOptions = GetCleanedShaderOptions(options);
		
		MetalShader *shader = _shaders->GetObjectForKey<MetalShader>(newOptions);
		if(shader)
			return shader;
		
		MTLCompileOptions *metalOptions = [[MTLCompileOptions alloc] init];
		
		if(newOptions)
		{
			const Dictionary *defines = newOptions->GetDefines();
			if(defines)
			{
				NSMutableDictionary *metalDefines = [[NSMutableDictionary alloc] init];
				
				defines->Enumerate<Object, Object>([&](Object *value, const Object *key, bool &stop) {
					
					if(key->IsKindOfClass(String::GetMetaClass()))
					{
						NSString *keyString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(key)->GetUTF8String()];
						
						if(value->IsKindOfClass(Number::GetMetaClass()))
						{
							NSNumber *valueNumber = [[NSNumber alloc] initWithLongLong:static_cast<Number *>(value)->GetInt64Value()];
							[metalDefines setObject:valueNumber forKey:keyString];
							[valueNumber release];
						}
						else if(value->IsKindOfClass(String::GetMetaClass()))
						{
							NSString *valueString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(value)->GetUTF8String()];
							[metalDefines setObject:valueString forKey:keyString];
							[valueString release];
						}
						
						[keyString release];
					}
					
				});
				
				[metalOptions setPreprocessorMacros:metalDefines];
				[metalDefines release];
			}
		}
		
		String *content = String::WithContentsOfFile(_fileName, Encoding::UTF8);
		
		NSError *error = nil;
		id<MTLLibrary> metalLibrary = [device newLibraryWithSource:[NSString stringWithUTF8String:content->GetUTF8String()] options:metalOptions error:&error];
		[metalOptions release];
		
		if(!metalLibrary)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);
		
		id<MTLFunction> function = [metalLibrary newFunctionWithName:[NSString stringWithUTF8String:_entryPoint->GetUTF8String()]];
		
		//TODO: release once shader is not needed anymore
		//[metalLibrary release];
		
		if(!function)
			return nullptr;
		
		//TODO: verify shader type in debug builds?
/*		Shader::Type type = Shader::Type::Vertex;
		switch([function functionType])
		{
			case MTLFunctionTypeFragment:
			{
				type = Shader::Type::Fragment;
				break;
			}
			case MTLFunctionTypeVertex:
			{
				type = Shader::Type::Vertex;
				break;
			}
			case MTLFunctionTypeKernel:
			{
				type = Shader::Type::Compute;
				break;
			}
		}*/
		
		const Array *samplers = GetSamplerSignature(newOptions);
		shader = new MetalShader(library, _type, samplers, newOptions, function, coordinator);
		_shaders->SetObjectForKey(shader, newOptions);
		return shader->Autorelease();
	}

	RNDefineMeta(MetalShaderLibrary, ShaderLibrary)

	MetalShaderLibrary::MetalShaderLibrary(id<MTLDevice> device, const String *file, MetalStateCoordinator *coordinator) :
		_device(device), _specificShaderLibraries(new Dictionary()), _coordinator(coordinator)
	{
		Data *data = Data::WithContentsOfFile(file);
		if(!data)
			throw InvalidArgumentException(RNSTR("Could not open file: " << file));
		
		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);
		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {
			String *fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~metal"));
			if(!fileString)
				fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file"));
			
			if(!fileString)
				return;
			
			Array *shadersArray = libraryDictionary->GetObjectForKey<Array>(RNCSTR("shaders"));
			shadersArray->Enumerate<Dictionary>([&](Dictionary *shaderDictionary, size_t index, bool &stop) {
				String *entryPointName = shaderDictionary->GetObjectForKey<String>(RNCSTR("name"));
				String *shaderType = shaderDictionary->GetObjectForKey<String>(RNCSTR("type"));
				Dictionary *signature = shaderDictionary->GetObjectForKey<Dictionary>(RNCSTR("signature"));
				
				Shader::Type type = Shader::Type::Vertex;
				if(shaderType->IsEqual(RNCSTR("vertex")))
				{
					type = Shader::Type::Vertex;
				}
				else if(shaderType->IsEqual(RNCSTR("fragment")))
				{
					type = Shader::Type::Fragment;
				}
				else if(shaderType->IsEqual(RNCSTR("compute")))
				{
					type = Shader::Type::Compute;
				}
				else
				{
					RN_ASSERT(false, "Unknown shader type %s for %s in library %s.", shaderType, entryPointName, file);
				}
				
				MetalSpecificShaderLibrary *specificLibrary = new MetalSpecificShaderLibrary(_device, fileString, entryPointName, type, signature);
				_specificShaderLibraries->SetObjectForKey(specificLibrary, entryPointName);
			});
		});
	}
	MetalShaderLibrary::~MetalShaderLibrary()
	{
		_specificShaderLibraries->Release();
	}

	Shader *MetalShaderLibrary::GetShaderWithName(const String *name, const Shader::Options *options)
	{
		MetalSpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<MetalSpecificShaderLibrary>(name);
		if(!specificLibrary)
			return nullptr;
		
		if(!options)
			options = Shader::Options::WithNone();
		
		return specificLibrary->GetShaderWithOptions(_device, _coordinator, this, options);
	}

	Shader *MetalShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
		
/*		MetalSpecializedShaderLibrary *specializedLibrary = _specificShaderLibraries->GetObjectForKey<MetalSpecificShaderLibrary>(shader->GetOptions());

		if(!specializedLibrary)
		{
			return nullptr;
		}

		return specializedLibrary->GetShaderWithName(shader->GetName()->StringByAppendingString(RNCSTR("_instanced")), this, _device, _coordinator);*/
	}
}
