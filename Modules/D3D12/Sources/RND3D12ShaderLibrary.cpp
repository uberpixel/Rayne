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

	D3D12SpecificShaderLibrary::D3D12SpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type, Dictionary *signatureDescription) :
		_shaders(new Dictionary()),
		_fileName(fileName->Retain()),
		_entryPoint(entryPoint->Retain()),
		_type(type),
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

						Shader::Sampler *sampler = new Shader::Sampler(wrapMode, filterType, anisotropyValue);
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

	const Array *D3D12SpecificShaderLibrary::GetSamplerSignature(const Shader::Options *options) const
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

		return samplerArray->Autorelease();
	}

	Shader *D3D12SpecificShaderLibrary::GetShaderWithOptions(ShaderLibrary *library, const Shader::Options *options)
	{
		const Shader::Options *newOptions = GetCleanedShaderOptions(options);

		D3D12Shader *shader = _shaders->GetObjectForKey<D3D12Shader>(newOptions);
		if(shader)
			return shader;

		const Array *samplers = GetSamplerSignature(newOptions);
		shader = new D3D12Shader(library, _fileName, _entryPoint, _type, newOptions, samplers);
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

				D3D12SpecificShaderLibrary *specificLibrary = new D3D12SpecificShaderLibrary(fileString, entryPointName, type, signature);
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
		D3D12SpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<D3D12SpecificShaderLibrary>(name);
		if(!specificLibrary)
			return nullptr;

		if(options)
			return specificLibrary->GetShaderWithOptions(this, options);

		return specificLibrary->GetShaderWithOptions(this, Shader::Options::WithNone());
	}

	Shader *D3D12ShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
	}
}
