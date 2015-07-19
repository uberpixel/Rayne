//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"

namespace RN
{
	RNDefineMeta(Material, Object)

	Material::Material(Shader *fragmentShader, Shader *vertexShader) :
		_fragmentShader(SafeRetain(fragmentShader)),
		_vertexShader(SafeRetain(vertexShader))
	{
		RN_ASSERT(!_fragmentShader || _fragmentShader->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		RN_ASSERT(_vertexShader->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
	}

	Material::~Material()
	{
		SafeRelease(_fragmentShader);
		SafeRelease(_vertexShader);
	}
}
