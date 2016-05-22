//
//  RNShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderLibrary.h"
#include "../Math/RNAlgorithm.h"

namespace RN
{
	RNDefineMeta(ShaderLookupRequest, Object)

	ShaderLookupRequest::ShaderLookupRequest() :
		receiveShadows(true),
		castShadows(true),
		discard(true)
	{}

	RNDefineMeta(ShaderCompileOptions, Object)

	ShaderCompileOptions::ShaderCompileOptions() :
		_defines(new Dictionary()),
		_basePath(nullptr)
	{}

	ShaderCompileOptions::~ShaderCompileOptions()
	{
		SafeRelease(_defines);
		SafeRelease(_basePath);
	}

	void ShaderCompileOptions::SetDefines(const Dictionary *defines)
	{
		SafeRelease(_defines);
		_defines = SafeCopy(defines);

		if(!_defines)
			_defines = new Dictionary();
	}
	void ShaderCompileOptions::SetBasePath(const String *basePath)
	{
		SafeRelease(_basePath);
		_basePath = SafeCopy(basePath);
	}

	bool ShaderCompileOptions::IsEqual(const Object *other) const
	{
		const ShaderCompileOptions *options = other->Downcast<ShaderCompileOptions>();
		if(RN_EXPECT_FALSE(!options))
			return false;

		if(!options->_defines->IsEqual(_defines))
			return false;

		return true;
	}
	size_t ShaderCompileOptions::GetHash() const
	{
		size_t hash = 0;

		HashCombine(hash, _defines->GetHash());

		if(_basePath)
			HashCombine(hash, _basePath->GetHash());

		return hash;
	}

	RNDefineMeta(ShaderProgram, Object)

	ShaderProgram::ShaderProgram(Shader *vertexShader, Shader *fragmentShader) :
		_vertexShader(SafeRetain(vertexShader)),
		_fragmentShader(SafeRetain(fragmentShader))
	{}

	ShaderProgram::~ShaderProgram()
	{
		SafeRelease(_vertexShader);
		SafeRelease(_fragmentShader);
	}

	ShaderProgram *ShaderProgram::WithVertexAndFragmentShaders(Shader *vertex, Shader *fragment)
	{
		ShaderProgram *program = new ShaderProgram(vertex, fragment);
		return program->Autorelease();
	}

	RNDefineMeta(ShaderLibrary, Object)

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{

	}
}
