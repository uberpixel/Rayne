//
//  RNShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderLibrary.h"

namespace RN
{
	RNDefineMeta(ShaderLibrary, Object)
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
}
