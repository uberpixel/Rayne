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

	Material::Material(Shader *vertexShader, Shader *fragmentShader) :
		_override(0),
		_textures(new Array()),
		_vertexBuffers(nullptr),
		_fragmentBuffers(nullptr)
	{
		_properties.depthMode = DepthMode::Less;
		_properties.depthWriteEnabled = true;
	  	_properties.ambientColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
		_properties.diffuseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		_properties.specularColor = Color(1.0f, 1.0f, 1.0f, 4.0f);
		_properties.emissiveColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
		_properties.discardThreshold = 0.3f;
		_properties.alphaToCoverageClamp = 1.0f;
		_properties.useAlphaToCoverage = false;
		_properties.textureTileFactor = 1.0f;
		_properties.usePolygonOffset = false;
		_properties.polygonOffsetFactor = 1.1f;
		_properties.polygonOffsetUnits = 0.1f;
		_properties.cullMode = CullMode::BackFace;
		
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			if(i == 0)
			{
				_vertexShader[i] = SafeRetain(vertexShader);
				_fragmentShader[i] = SafeRetain(fragmentShader);
			}
			else
			{
				_vertexShader[i] = nullptr;
				_fragmentShader[i] = nullptr;
			}

			RN_ASSERT(!_vertexShader[i] || _vertexShader[i]->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
			RN_ASSERT(!_fragmentShader[i] || _fragmentShader[i]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		}
	}

	Material::Material(const Material *other) :
		_override(other->_override),
		_textures(SafeCopy(other->_textures)),
		_vertexBuffers(SafeCopy(other->_vertexBuffers)),
		_fragmentBuffers(SafeCopy(other->_fragmentBuffers))
	{
		_properties.depthMode = other->_properties.depthMode;
		_properties.depthWriteEnabled = other->_properties.depthWriteEnabled;
		_properties.ambientColor = other->_properties.ambientColor;
		_properties.diffuseColor = other->_properties.diffuseColor;
		_properties.specularColor = other->_properties.specularColor;
		_properties.emissiveColor = other->_properties.emissiveColor;
		_properties.discardThreshold = other->_properties.discardThreshold;
		_properties.alphaToCoverageClamp = other->_properties.alphaToCoverageClamp;
		_properties.useAlphaToCoverage = other->_properties.useAlphaToCoverage;
		_properties.textureTileFactor = other->_properties.textureTileFactor;
		_properties.usePolygonOffset = other->_properties.usePolygonOffset;
		_properties.polygonOffsetFactor = other->_properties.polygonOffsetFactor;
		_properties.polygonOffsetUnits = other->_properties.polygonOffsetUnits;
		_properties.cullMode = other->_properties.cullMode;
		
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			_vertexShader[i] = SafeRetain(other->_vertexShader[i]);
			_fragmentShader[i] = SafeRetain(other->_fragmentShader[i]);
		}
	}

	Material::~Material()
	{
		SafeRelease(_vertexBuffers);
		SafeRelease(_fragmentBuffers);
		SafeRelease(_textures);

		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			SafeRelease(_fragmentShader[i]);
			SafeRelease(_vertexShader[i]);
		}
	}
	
 	Material *Material::WithShaders(Shader *vertexShader, Shader *fragmentShader)
	{
		Material *material = new Material(vertexShader, fragmentShader);
		return material->Autorelease();
	}
	
 	Material *Material::WithMaterial(const Material *material)
	{
		Material *copyMaterial = new Material(material);
		return copyMaterial->Autorelease();
	}
	
 	void Material::SetTextures(const Array *textures)
	{
		SafeRelease(textures);
		_textures = textures->Copy();
	}
	
 	void Material::AddTexture(Texture *texture)
	{
		_textures->AddObject(texture);
	}
	
	void Material::RemoveAllTextures()
	{
		_textures->RemoveAllObjects();
	}
	
	void Material::SetFragmentShader(Shader *shader, Shader::UsageHint type)
	{
		_fragmentShader[type] = SafeRetain(shader);
		RN_ASSERT(!_fragmentShader[type] || _fragmentShader[type]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
	}
	
	void Material::SetVertexShader(Shader *shader, Shader::UsageHint type)
	{
		_vertexShader[type] = SafeRetain(shader);
		RN_ASSERT(!_vertexShader[type] || _vertexShader[type]->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
	}

	void Material::SetOverride(Override override)
	{
		_override = override;
	}

	void Material::SetDepthWriteEnabled(bool depthWrite)
	{
		_properties.depthWriteEnabled = depthWrite;
	}
	void Material::SetDepthMode(DepthMode mode)
	{
		_properties.depthMode = mode;
	}

	void Material::SetFragmentBuffers(const Array *buffers)
	{
		SafeRelease(_fragmentBuffers);
		_fragmentBuffers = SafeCopy(buffers);
	}
	void Material::SetVertexBuffers(const Array *buffers)
	{
		SafeRelease(_vertexBuffers);
		_vertexBuffers = SafeCopy(buffers);
	}

	void Material::SetDiscardThreshold(float threshold)
	{
		_properties.discardThreshold = threshold;
	}

	void Material::SetTextureTileFactor(float factor)
	{
		_properties.textureTileFactor = factor;
	}

	void Material::SetAmbientColor(const Color &color)
	{
		_properties.ambientColor = color;
	}
	void Material::SetDiffuseColor(const Color &color)
	{
		_properties.diffuseColor = color;
	}
	void Material::SetSpecularColor(const Color &color)
	{
		_properties.specularColor = color;
	}
	void Material::SetEmissiveColor(const Color &color)
	{
		_properties.emissiveColor = color;
	}

	void Material::SetCullMode(CullMode mode)
	{
		_properties.cullMode = mode;
	}

	void Material::SetPolygonOffset(bool enable, float factor, float units)
	{
		_properties.usePolygonOffset = enable;
		_properties.polygonOffsetFactor = factor;
		_properties.polygonOffsetUnits = units;
	}
	
	void Material::SetAlphaToCoverage(bool enabled, float clamp)
	{
		_properties.useAlphaToCoverage = enabled;
		_properties.alphaToCoverageClamp = clamp;
	}

	Shader *Material::GetFragmentShader(Shader::UsageHint type) const
	{
		if(!_fragmentShader[static_cast<uint8>(type)] && !_vertexShader[static_cast<uint8>(type)])
			return _fragmentShader[static_cast<uint8>(Shader::UsageHint::Default)];

		return _fragmentShader[static_cast<uint8>(type)];
	}
	Shader *Material::GetVertexShader(Shader::UsageHint type) const
	{
		if(!_vertexShader[static_cast<uint8>(type)])
			return _vertexShader[static_cast<uint8>(Shader::UsageHint::Default)];

		return _vertexShader[static_cast<uint8>(type)];
	}

	const Material::Properties Material::GetMergedProperties(Material *overrideMaterial) const
	{
		if(!overrideMaterial)
			return _properties;
		
		Properties properties = _properties;
		if(overrideMaterial->GetOverride() & Override::GroupDepth && !(_override & Override::GroupDepth))
		{
			properties.depthMode = overrideMaterial->_properties.depthMode;
			properties.depthWriteEnabled = overrideMaterial->_properties.depthWriteEnabled;
		}
		
		if(overrideMaterial->GetOverride() & Override::GroupColors && !(_override & Override::GroupColors))
		{
			properties.ambientColor = overrideMaterial->_properties.ambientColor;
			properties.diffuseColor = overrideMaterial->_properties.diffuseColor;
			properties.specularColor = overrideMaterial->_properties.specularColor;
			properties.emissiveColor = overrideMaterial->_properties.emissiveColor;
		}
		
		if(overrideMaterial->GetOverride() & Override::DiscardThreshold && !(_override & Override::DiscardThreshold))
		{
			properties.discardThreshold = overrideMaterial->_properties.discardThreshold;
		}
		
		if(overrideMaterial->GetOverride() & Override::GroupAlphaToCoverage && !(_override & Override::GroupAlphaToCoverage))
		{
			properties.useAlphaToCoverage = overrideMaterial->_properties.useAlphaToCoverage;
			properties.alphaToCoverageClamp = overrideMaterial->_properties.alphaToCoverageClamp;
		}
		
		if(overrideMaterial->GetOverride() & Override::TextureTileFactor && !(_override & Override::TextureTileFactor))
		{
			properties.textureTileFactor = overrideMaterial->_properties.textureTileFactor;
		}
		
		if(overrideMaterial->GetOverride() & Override::GroupPolygonOffset && !(_override & Override::GroupPolygonOffset))
		{
			properties.usePolygonOffset = overrideMaterial->_properties.usePolygonOffset;
			properties.polygonOffsetFactor = overrideMaterial->_properties.polygonOffsetFactor;
			properties.polygonOffsetUnits = overrideMaterial->_properties.polygonOffsetUnits;
		}
		
		if(overrideMaterial->GetOverride() & Override::CullMode && !(_override & Override::CullMode))
		{
			properties.cullMode = overrideMaterial->_properties.cullMode;
		}

		return properties;
	}
}
