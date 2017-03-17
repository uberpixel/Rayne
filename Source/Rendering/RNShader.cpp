//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNShaderLibrary.h"

namespace RN
{
	RNDefineMeta(Shader, Object)
	RNDefineScopedMeta(Shader, Attribute, Object)

	Shader::Attribute::Attribute(const String *name, PrimitiveType type) :
		_name(name->Copy()),
		_type(type)
	{}

	Shader::Attribute::~Attribute()
	{
		_name->Release();
	}


	Shader::Shader(ShaderLibrary *library, const ShaderOptions *options) :
		_options(options->Retain()), _library(library)
	{}

	Shader::~Shader()
	{
		_options->Release();
	}

	Shader::Type Shader::GetType() const
	{
		return _options->GetType();
	}

	const ShaderOptions *Shader::GetShaderOptions() const
	{
		//TODO: maybe retain!?
		return _options;
	}

	ShaderLibrary *Shader::GetLibrary() const
	{
		return _library;
	}
}
