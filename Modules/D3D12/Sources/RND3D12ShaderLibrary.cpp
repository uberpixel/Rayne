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
	RNDefineMeta(D3D12ShaderLibrary, ShaderLibrary)

	D3D12ShaderLibrary::D3D12ShaderLibrary(const String *file) :
		_shaders(new Dictionary())
	{
		Data *data = Data::WithContentsOfFile(file);
		if(!data)
			throw InvalidArgumentException(RNSTR("Could not open file: " << file));

		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);
		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {
			String *fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~d3d12"));
			if(!fileString)
				fileString = libraryDictionary->GetObjectForKey<String>(RNCSTR("file"));

			Array *shadersArray = libraryDictionary->GetObjectForKey<Array>(RNCSTR("shaders"));
			shadersArray->Enumerate<Dictionary>([&](Dictionary *shaderDictionary, size_t index, bool &stop) {
				String *entryPointName = shaderDictionary->GetObjectForKey<String>(RNCSTR("name"));
				String *shaderType = shaderDictionary->GetObjectForKey<String>(RNCSTR("type"));
				D3D12Shader *shader = new D3D12Shader(fileString, entryPointName, shaderType);
				if(shader)
				{
					_shaders->SetObjectForKey(shader, entryPointName);
				}
			});
		});
	}

	D3D12ShaderLibrary::~D3D12ShaderLibrary()
	{
		_shaders->Release();
	}

	Shader *D3D12ShaderLibrary::GetShaderWithName(const String *name)
	{
		D3D12Shader *temp = _shaders->GetObjectForKey<D3D12Shader>(name);
		if(!temp)
			return nullptr;
		
		return temp;
	}

	Array *D3D12ShaderLibrary::GetShaderNames() const
	{
		return _shaders->GetAllKeys();
	}
}
