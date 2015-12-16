//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"

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


	Shader::Shader(Type type, ShaderLibrary *library) :
		_type(type),
		_library(library)
	{}
}
