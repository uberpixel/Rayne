//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"

namespace RN
{
	RNDeclareMeta(Material)
	
	Material::Material()
	{
		_shader = 0;
		_textures = new Array<Texture>();
		
		SetDefaultProperties();
	}
	
	Material::Material(RN::Shader *shader)
	{
		_shader = shader ? shader->Retain() : 0;
		_textures = new Array<Texture>();
		
		SetDefaultProperties();
	}
	
	Material::~Material()
	{
		if(_shader)
			_shader->Release();
		
		if(_textures)
			_textures->Release();
	}
	
	
	
	void Material::SetShader(RN::Shader *shader)
	{
		if(_shader)
			_shader->Release();
		_shader = shader ? shader->Retain() : 0;
	}
	
	Shader *Material::Shader() const
	{
		return _shader;
	}
	
	
	void Material::AddTexture(Texture *texture)
	{
		_textures->AddObject(texture);
	}
	
	void Material::RemoveTexture(Texture *texture)
	{
		_textures->RemoveObject(texture);
	}
	
	void Material::RemoveTextures()
	{
		_textures->RemoveAllObjects();
	}
	
	
	Array<Texture> *Material::Textures() const
	{
		return _textures;
	}
	
	
	void Material::SetDefaultProperties()
	{
		culling  = true;
		cullmode = GL_CCW;
		
		blending = false;
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
}
