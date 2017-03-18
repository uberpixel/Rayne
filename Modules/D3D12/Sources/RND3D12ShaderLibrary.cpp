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

	D3D12SpecificShaderLibrary::D3D12SpecificShaderLibrary(const String *fileName, const String *entryPoint, Shader::Type type) : _shaders(new Dictionary()), _fileName(fileName->Retain()), _entryPoint(entryPoint->Retain()), _type(type)
	{

	}

	D3D12SpecificShaderLibrary::~D3D12SpecificShaderLibrary()
	{
		_fileName->Release();
		_entryPoint->Release();
		_shaders->Release();
	}

	Shader *D3D12SpecificShaderLibrary::GetShaderWithOptions(ShaderLibrary *library, const ShaderOptions *options)
	{
		D3D12Shader *shader = _shaders->GetObjectForKey<D3D12Shader>(options);
		if(shader)
			return shader;

		shader = new D3D12Shader(library, _fileName, _entryPoint, _type, options);
		_shaders->SetObjectForKey(shader, options);

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

				D3D12SpecificShaderLibrary *specificLibrary = new D3D12SpecificShaderLibrary(fileString, entryPointName, type);
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

		return specificLibrary->GetShaderWithOptions(this, options);
	}

	Shader *D3D12ShaderLibrary::GetInstancedShaderForShader(Shader *shader)
	{
		return nullptr;
	}
}
