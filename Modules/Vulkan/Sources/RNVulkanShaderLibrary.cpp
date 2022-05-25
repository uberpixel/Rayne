//
//  RNVulkanShaderLibrary.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanShaderLibrary.h"
#include "RNVulkanShader.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanSpecificShaderLibrary, Object)
	RNDefineMeta(VulkanShaderLibrary, ShaderLibrary)


	VulkanSpecificShaderLibrary::VulkanSpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type, bool hasInstancing, Dictionary *signatureDescription) :
		_shaders(new Dictionary()),
		_fileName(fileName->Retain()),
		_entryPoint(entryPoint->Retain()),
		_type(type),
		_hasInstancing(hasInstancing),
		_signatureDescription(signatureDescription)
	{
		if(_signatureDescription) _signatureDescription->Retain();
	}

	VulkanSpecificShaderLibrary::~VulkanSpecificShaderLibrary()
	{
		_fileName->Release();
		_entryPoint->Release();
		_shaders->Release();
		_signatureDescription->Release();
	}

	const Shader::Options *VulkanSpecificShaderLibrary::GetCleanedShaderOptions(const Shader::Options *options) const
	{
		const Dictionary *oldDefines = options->GetDefines();
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
				String *obj = oldDefines->GetObjectForKey<String>(name);
				if(obj)
				{
					newOptions->AddDefine(name, obj);
				}
			}
		});

		return newOptions;
	}

	size_t VulkanSpecificShaderLibrary::GetPermutationIndexForOptions(const Shader::Options *options) const
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

		const Dictionary *oldDefines = options->GetDefines();
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
				String *obj = oldDefines->GetObjectForKey<String>(name);
				if(obj)
				{
					permutationIndex |= (static_cast<size_t>(1) << index);
				}
			}
		});

		return permutationIndex;
	}

	const Array *VulkanSpecificShaderLibrary::GetSamplerSignature(const Shader::Options *options) const
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
					if(!options->GetDefines()->GetObjectForKey(name))
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

	Shader *VulkanSpecificShaderLibrary::GetShaderWithOptions(ShaderLibrary *library, const Shader::Options *options)
	{
		const Shader::Options *newOptions = GetCleanedShaderOptions(options);

		VulkanShader *shader = _shaders->GetObjectForKey<VulkanShader>(newOptions);
		if(shader)
			return shader;

		size_t permutationIndex = GetPermutationIndexForOptions(newOptions);
		RN::String *permutationFileName = _fileName->StringByDeletingPathExtension();
		permutationFileName->Append(RNSTR("." << permutationIndex));
		permutationFileName->Append(".spirv");

		RN::String *filePath = RN::FileManager::GetSharedInstance()->ResolveFullPath(permutationFileName, 0);

		const Array *samplers = GetSamplerSignature(newOptions);
		shader = new VulkanShader(library, filePath, _entryPoint, _type, _hasInstancing, newOptions, samplers);
		_shaders->SetObjectForKey(shader, newOptions);

		return shader->Autorelease();
	}


	VulkanShaderLibrary::VulkanShaderLibrary(const String *file) : _specificShaderLibraries(new Dictionary())
	{
		Data *data = Data::WithContentsOfFile(file);
		if(!data)
			throw InvalidArgumentException(RNSTR("Could not open file: " << file));

		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);
		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {
			String *fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~vulkan"));
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

				VulkanSpecificShaderLibrary *specificLibrary = new VulkanSpecificShaderLibrary(fileString, entryPointName, type, hasInstancing, signature);
				_specificShaderLibraries->SetObjectForKey(specificLibrary, entryPointName);
			});
		});
	}

	VulkanShaderLibrary::~VulkanShaderLibrary()
	{
		_specificShaderLibraries->Release();
	}

	Shader *VulkanShaderLibrary::GetShaderWithName(const String *name, const Shader::Options *options)
	{
		VulkanSpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<VulkanSpecificShaderLibrary>(name);
		if(!specificLibrary)
			return nullptr;

		if(options)
			return specificLibrary->GetShaderWithOptions(this, options);

		return specificLibrary->GetShaderWithOptions(this, Shader::Options::WithNone());
	}

	Shader *VulkanShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
	}
}
