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

	const ShaderOptions *D3D12SpecificShaderLibrary::GetCleanedShaderOptions(const ShaderOptions *options) const
	{
		const Dictionary *oldDefines = options->GetDefines();
		ShaderOptions *newOptions = ShaderOptions::WithNothing();
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

	const Shader::Signature *D3D12SpecificShaderLibrary::GetShaderSignature(const ShaderOptions *options) const
	{
		if(!_signatureDescription)
		{
			Array *uniformDescriptors = new Array();
			Shader::Signature *signature = new Shader::Signature(uniformDescriptors->Autorelease(), 0, 0);
			return signature->Autorelease();
		}

		uint8 samplerCount = 0;
		uint8 textureCount = 0;
		uint32 offset = 0;

		Number *textureObject = _signatureDescription->GetObjectForKey<Number>(RNCSTR("textures"));
		if(textureObject) textureCount = textureObject->GetUint8Value();
		Number *samplerObject = _signatureDescription->GetObjectForKey<Number>(RNCSTR("samplers"));
		if(samplerObject) samplerCount = samplerObject->GetUint8Value();

		Array *uniformArray = _signatureDescription->GetObjectForKey<Array>(RNCSTR("uniforms"));
		Array *uniformDescriptors = GetUniformDescriptors(uniformArray, offset)->Retain();

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

					Number *optionTextureObject = dict->GetObjectForKey<Number>(RNCSTR("textures"));
					if(optionTextureObject) textureCount += optionTextureObject->GetUint32Value();
					Number *optionSamplerObject = dict->GetObjectForKey<Number>(RNCSTR("samplers"));
					if(optionSamplerObject) samplerCount += optionSamplerObject->GetUint32Value();

					Array *optionUniformArray = dict->GetObjectForKey<Array>(RNCSTR("uniforms"));
					Array *optionUniformDescriptors = GetUniformDescriptors(optionUniformArray, offset);

					uniformDescriptors->AddObjectsFromArray(optionUniformDescriptors);
				}
			});
		}

		Shader::Signature *signature = new Shader::Signature(uniformDescriptors->Autorelease(), samplerCount, textureCount);
		return signature->Autorelease();
	}

	Shader *D3D12SpecificShaderLibrary::GetShaderWithOptions(ShaderLibrary *library, const ShaderOptions *options)
	{
		const ShaderOptions *newOptions = GetCleanedShaderOptions(options);

		D3D12Shader *shader = _shaders->GetObjectForKey<D3D12Shader>(newOptions);
		if(shader)
			return shader;

		const Shader::Signature *signature = GetShaderSignature(newOptions);
		shader = new D3D12Shader(library, _fileName, _entryPoint, _type, newOptions, signature);
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

	Shader *D3D12ShaderLibrary::GetShaderWithName(const String *name, const ShaderOptions *options)
	{
		D3D12SpecificShaderLibrary *specificLibrary = _specificShaderLibraries->GetObjectForKey<D3D12SpecificShaderLibrary>(name);
		if(!specificLibrary)
			return nullptr;

		if(options)
			return specificLibrary->GetShaderWithOptions(this, options);

		return specificLibrary->GetShaderWithOptions(this, ShaderOptions::WithNothing());
	}

	Shader *D3D12ShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
	}
}
