//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"

namespace RN
{
	Material::Material(RN::Shader *shader)
	{
		_shader = shader;
		_shader->Retain();
		
		culling  = true;
		cullmode = GL_CCW;
		
		blending = true;
		blendSource = GL_ONE;
		blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		
		ambient = Color(0.2f, 0.2f, 0.2f, 1.0f);
		diffuse = Color(0.8f, 0.8f, 0.8f, 1.0f);
		specular = Color(0.0f, 0.0f, 0.0f, 0.0f);
		emissive = Color(0.0f, 0.0f, 0.0f, 0.0f);
		
		shininess = 0.0f;
		
		alphatest = false;
		alphatestvalue = 0.75;
		
		depthtest = true;
		depthtestmode = GL_LEQUAL;
		depthwrite = true;
	}
	
	Material::~Material()
	{
		_shader->Release();
	}
	
	void Material::SetShader(RN::Shader *shader)
	{
		_shader->Release();
		_shader = shader;
		_shader->Retain();
	}
	
	Shader *Material::Shader() const
	{
		return _shader;
	}
}
