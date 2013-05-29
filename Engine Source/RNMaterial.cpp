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
	
	Material::Material() :
		_lookup(0)
	{
		_shader = 0;
		Initialize();
	}
	
	Material::Material(RN::Shader *shader) :
		_lookup(0)
	{
		_shader = shader ? shader->Retain() : 0;
		Initialize();
	}
	
	Material::~Material()
	{
		if(_shader)
			_shader->Release();
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
		
		ambient = Color(0.02f, 0.02f, 0.02f, 1.0f);
		diffuse = Color(0.8f, 0.8f, 0.8f, 1.0f);
		specular = Color(1.0f, 1.0f, 1.0f, 4.0f);
		emissive = Color(0.0f, 0.0f, 0.0f, 0.0f);
		
		shininess = 0.0f;
		
		depthtest = true;
		depthtestmode = GL_LEQUAL;
		depthwrite = true;
		
		discard = false;
		discardThreshold = 0.3f;
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
		_textures.AddObject(texture);
	}
	
	void Material::RemoveTexture(Texture *texture)
	{
		_textures.RemoveObject(texture);
	}
	
	void Material::RemoveTextures()
	{
		_textures.RemoveAllObjects();
	}
	
	
	
	void Material::UpdateLookupRequest()
	{
		_lookup = ShaderLookup(_defines);
	}
	
	
	void Material::Define(const std::string& define)
	{
		_defines.emplace_back(ShaderDefine(define, ""));
		UpdateLookupRequest();
	}
	
	void Material::Define(const std::string& define, const std::string& value)
	{
		_defines.emplace_back(ShaderDefine(define, value));
		UpdateLookupRequest();
	}
	
	void Material::Define(const std::string& define, int32 value)
	{
		char buffer[32];
		sprintf(buffer, "%i", value);
		
		Define(define, buffer);
	}
	
	void Material::Define(const std::string& define, float value)
	{
		char buffer[32];
		sprintf(buffer, "%f", value);
		
		_defines.emplace_back(ShaderDefine(define, buffer));
	}
	
	void Material::Undefine(const std::string& name)
	{
		for(auto i=_defines.begin(); i!=_defines.end(); i++)
		{
			if(name == i->name)
			{
				_defines.erase(i);
				UpdateLookupRequest();
				return;
			}
		}
	}
}
