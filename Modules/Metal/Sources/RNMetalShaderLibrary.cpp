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

	MetalSpecificShaderLibrary::MetalSpecificShaderLibrary(id<MTLDevice> device, const String *fileName, const String *entryPoint, Shader::Type type, bool hasInstancing, Dictionary *signatureDescription) :
	_shaders(new Dictionary()),
	_entryPoint(entryPoint->Retain()),
	_fileName(fileName->Retain()),
	_type(type),
	_hasInstancing(hasInstancing),
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
		Shader::Options *newOptions = Shader::Options::WithNone();
		if(!_signatureDescription)
			return newOptions;
		
		Array *signatureOptions = _signatureDescription->GetObjectForKey<Array>(RNCSTR("options"));
		if(!signatureOptions)
		{
			Dictionary *signatureOptionsDictionary = _signatureDescription->GetObjectForKey<Dictionary>(RNCSTR("options"));
			if(signatureOptionsDictionary)
			{
				signatureOptions = signatureOptionsDictionary->GetObjectForKey<Array>(RNCSTR("defines"));
			}
		}
		
		if(!signatureOptions) return newOptions;
		
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
				const String *obj = options->GetValue(name->GetUTF8String());
				if(obj)
				{
					newOptions->AddDefine(name, obj);
				}
			}
		});
		
		return newOptions;
	}
	
	size_t MetalSpecificShaderLibrary::GetPermutationIndexForOptions(const Shader::Options *options) const
	{
		if(!options || !_signatureDescription) return 0;
		
		Array *signatureOptions = _signatureDescription->GetObjectForKey<Array>(RNCSTR("options"));
		if(!signatureOptions)
		{
			Dictionary *signatureOptionsDictionary = _signatureDescription->GetObjectForKey<Dictionary>(RNCSTR("options"));
			if(signatureOptionsDictionary)
			{
				signatureOptions = signatureOptionsDictionary->GetObjectForKey<Array>(RNCSTR("defines"));
			}
		}
		
		if(!signatureOptions) return 0;
		
		size_t permutationIndex = 0;
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
				const String *obj = options->GetValue(name->GetUTF8String());
				if(obj)
				{
					permutationIndex |= (1 << index);
				}
			}
		});
		
		return permutationIndex;
	}
	
	const Array *MetalSpecificShaderLibrary::GetSamplerSignature(const Shader::Options *options) const
	{
		if(!_signatureDescription)
		{
			Array *samplers = new Array();
			return samplers->Autorelease();
		}
		
		Array *samplerDataArray = _signatureDescription->GetObjectForKey<Array>(RNCSTR("samplers"));
		Array *samplerArray = ShaderLibrary::GetSamplers(samplerDataArray);
		
		Array *signatureOptions = _signatureDescription->GetObjectForKey<Array>(RNCSTR("options"));
		if(!signatureOptions)
		{
			Dictionary *signatureOptionsDictionary = _signatureDescription->GetObjectForKey<Dictionary>(RNCSTR("options"));
			if(signatureOptionsDictionary)
			{
				signatureOptions = signatureOptionsDictionary->GetObjectForKey<Array>(RNCSTR("defines"));
			}
		}
		if(signatureOptions)
		{
			signatureOptions->Enumerate([&](Object *option, size_t index, bool &stop) {
				Dictionary *dict = option->Downcast<Dictionary>();
				if(dict)
				{
					String *name = dict->GetObjectForKey<String>(RNCSTR("option"));
					if(!options->GetValue(name->GetUTF8String()))
					{
						return;
					}
					Array *optionSamplerdataArray = dict->GetObjectForKey<Array>(RNCSTR("samplers"));
					Array *optionSamplerArray = ShaderLibrary::GetSamplers(optionSamplerdataArray);
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
		
		bool isShaderBinary = _fileName->HasSuffix(RNCSTR(".metallib"));
		
		NSError *error = nil;
		id<MTLLibrary> metalLibrary = nil;
		if(!isShaderBinary)
		{
			MTLCompileOptions *metalOptions = [[MTLCompileOptions alloc] init];
			if(newOptions && newOptions->GetCount() > 0)
			{
				NSMutableDictionary *metalDefines = [[NSMutableDictionary alloc] init];
				
				newOptions->Enumerate([&](const std::string &value, const std::string &key, bool &stop) {
					NSString *keyString = [[NSString alloc] initWithUTF8String:key.c_str()];
					NSString *valueString = [[NSString alloc] initWithUTF8String:value.c_str()];
					[metalDefines setObject:valueString forKey:keyString];
					[valueString release];
					[keyString release];
				});
				
				[metalOptions setPreprocessorMacros:metalDefines];
				[metalDefines release];
			}
			
			String *content = String::WithContentsOfFile(_fileName, Encoding::UTF8);
			metalLibrary = [device newLibraryWithSource:[NSString stringWithUTF8String:content->GetUTF8String()] options:metalOptions error:&error];
			
			[metalOptions release];
		}
		else
		{
			size_t permutationIndex = GetPermutationIndexForOptions(newOptions);
			RN::String *permutationFileName = _fileName->StringByDeletingPathExtension();
			permutationFileName->Append(RNSTR("." << permutationIndex));
			permutationFileName->Append(".metallib");
			RN::String *filePath = RN::FileManager::GetSharedInstance()->ResolveFullPath(permutationFileName, 0);
			RN_ASSERT(filePath, "Missing shader permutation! It might be excluded in the shader library? Is a wrong combination of shader defines used?");
			metalLibrary = [device newLibraryWithFile:[NSString stringWithUTF8String:filePath->GetUTF8String()] error:&error];
		}
		
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
		shader = new MetalShader(library, _type, _hasInstancing, samplers, newOptions, function, coordinator);
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
				Number *hasInstancingNumber = shaderDictionary->GetObjectForKey<Number>(RNCSTR("has_instancing"));
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
				
				bool hasInstancing = false;
				if(hasInstancingNumber)
				{
					hasInstancing = hasInstancingNumber->GetBoolValue();
				}
				
				MetalSpecificShaderLibrary *specificLibrary = new MetalSpecificShaderLibrary(_device, fileString, entryPointName, type, hasInstancing, signature);
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
		Lock();
		MetalSpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<MetalSpecificShaderLibrary>(name);
		if(!specificLibrary)
		{
			Unlock();
			return nullptr;
		}
		
		if(!options)
			options = Shader::Options::WithNone();
		
		Shader *shader = specificLibrary->GetShaderWithOptions(_device, _coordinator, this, options);;
		Unlock();
		
		return shader;
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
