//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		Initialize();
	}
	
	Material::Material(RN::Shader *shader)
	{
		_shader = shader ? shader->Retain() : 0;
		_textures = new Array<Texture>();
		
		Initialize();
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
	
	void Material::SetBlendMode(BlendMode mode)
	{
		switch(mode)
		{
			case BlendMode::Additive:
				blendSource = GL_ONE;
				blendDestination = GL_ONE;
				break;
				
			case BlendMode::Multiplicative:
				blendSource = GL_DST_COLOR;
				blendDestination = GL_ZERO;
				break;
				
			case BlendMode::Interpolative:
				blendSource = GL_ONE;
				blendDestination = GL_ONE_MINUS_SRC_ALPHA;
				break;
				
			case BlendMode::Cutout:
				blendSource = GL_ZERO;
				blendDestination = GL_ONE_MINUS_SRC_ALPHA;
				break;
		}
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
	
	
	void Material::Initialize()
	{
		override = 0;
		lighting = true;
		
		culling  = true;
		cullmode = GL_CCW;
		
		blending = false;
		blendSource = GL_ONE;
		blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		
		polygonOffset = false;
		polygonOffsetFactor = 1.0f;
		polygonOffsetUnits = 1.0f;
		
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
		
		discard = false;
		discardThreshold = 0.3f;
	}
}
