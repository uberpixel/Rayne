//
//  RND3D12ShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12ShaderLibrary.h"
#include "RND3D12Shader.h"

namespace RN
{
	RNDefineMeta(D3D12SpecificShaderLibrary, Object)

	D3D12SpecificShaderLibrary::D3D12SpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type, bool hasInstancing, Dictionary *signatureDescription) :
		_shaders(new Dictionary()),
		_fileName(fileName->Retain()),
		_entryPoint(entryPoint->Retain()),
		_type(type),
		_hasInstancing(hasInstancing),
		_signatureDescription(signatureDescription)
	{
		if(_signatureDescription) _signatureDescription->Retain();
	}

	D3D12SpecificShaderLibrary::~D3D12SpecificShaderLibrary()
	{
		_fileName->Release();
		_entryPoint->Release();
		_shaders->Release();
		_signatureDescription->Release();
	}

	const Shader::Options *D3D12SpecificShaderLibrary::GetCleanedShaderOptions(const Shader::Options *options) const
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

	size_t D3D12SpecificShaderLibrary::GetPermutationIndexForOptions(const Shader::Options *options) const
	{
		if (!options || !_signatureDescription) return 0;

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
					permutationIndex |= (static_cast<unsigned long long>(1) << index);
				}
			}
		});

		return permutationIndex;
	}

	const Array *D3D12SpecificShaderLibrary::GetSamplerSignature(const Shader::Options *options) const
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

	Shader *D3D12SpecificShaderLibrary::GetShaderWithOptions(ShaderLibrary *library, const Shader::Options *options)
	{
		const Shader::Options *newOptions = GetCleanedShaderOptions(options);

		D3D12Shader *shader = _shaders->GetObjectForKey<D3D12Shader>(newOptions);
		if(shader)
			return shader;

		const Array *samplers = GetSamplerSignature(newOptions);
		if(_fileName->HasSuffix(RNCSTR(".cso")))
		{
			size_t permutationIndex = GetPermutationIndexForOptions(newOptions);
			RN::String *permutationFileName = _fileName->StringByDeletingPathExtension();
			permutationFileName->Append(RNSTR("." << permutationIndex));
			permutationFileName->Append(".cso");
			RN::String *filePath = RN::FileManager::GetSharedInstance()->ResolveFullPath(permutationFileName, 0);
			shader = new D3D12Shader(library, filePath, _entryPoint, _type, _hasInstancing, newOptions, samplers);
		}
		else
		{
			shader = new D3D12Shader(library, _fileName, _entryPoint, _type, _hasInstancing, newOptions, samplers);
		}
		
		_shaders->SetObjectForKey(shader, newOptions);

		return shader->Autorelease();
	}

	RNDefineMeta(D3D12ShaderLibrary, ShaderLibrary)

	D3D12ShaderLibrary::D3D12ShaderLibrary(const String *file) : _specificShaderLibraries(new Dictionary())
	{
		Data *data = Data::WithContentsOfFile(file);
		if(!data)
			throw InvalidArgumentException(RNSTR("Could not open file: " << file));

		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);
		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {
			String *fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~d3d12"));
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

				D3D12SpecificShaderLibrary *specificLibrary = new D3D12SpecificShaderLibrary(fileString, entryPointName, type, hasInstancing, signature);
				_specificShaderLibraries->SetObjectForKey(specificLibrary, entryPointName);
			});
		});
	}

	D3D12ShaderLibrary::~D3D12ShaderLibrary()
	{
		_specificShaderLibraries->Release();
	}

	Shader *D3D12ShaderLibrary::GetShaderWithName(const String *name, const Shader::Options *options)
	{
		Lock();
		D3D12SpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<D3D12SpecificShaderLibrary>(name);
		RN_ASSERT(specificLibrary, "Shader with name does not exist in shader library.");

		Shader *shader = nullptr;
		if(options)
			shader = specificLibrary->GetShaderWithOptions(this, options);
		else
			shader = specificLibrary->GetShaderWithOptions(this, Shader::Options::WithNone());

		RN_ASSERT(shader, "Shader with name does not exist in shader library.");

		Unlock();
		return shader;
	}

	Shader *D3D12ShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
	}
}
