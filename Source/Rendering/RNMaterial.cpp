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
		_properties.colorWriteMask = 0xf;
		_properties.depthMode = DepthMode::Less;
		_properties.depthWriteEnabled = true;
	  	_properties.ambientColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
		_properties.diffuseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
		_properties.specularColor = Color(1.0f, 1.0f, 1.0f, 4.0f);
		_properties.emissiveColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
		_properties.alphaToCoverageClamp = 1.0f;
		_properties.useAlphaToCoverage = false;
		_properties.textureTileFactor = 1.0f;
		_properties.usePolygonOffset = false;
		_properties.polygonOffsetFactor = 1.1f;
		_properties.polygonOffsetUnits = 0.1f;
		_properties.cullMode = CullMode::BackFace;
		_properties.blendOperationRGB = BlendOperation::None;
		_properties.blendOperationAlpha = BlendOperation::None;
		_properties.blendFactorSourceRGB = BlendFactor::SourceAlpha;
		_properties.blendFactorSourceAlpha = BlendFactor::SourceAlpha;
		_properties.blendFactorDestinationRGB = BlendFactor::OneMinusSourceAlpha;
		_properties.blendFactorDestinationAlpha = BlendFactor::OneMinusSourceAlpha;
		
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
		_properties.colorWriteMask = other->_properties.colorWriteMask;
		_properties.depthMode = other->_properties.depthMode;
		_properties.depthWriteEnabled = other->_properties.depthWriteEnabled;
		_properties.ambientColor = other->_properties.ambientColor;
		_properties.diffuseColor = other->_properties.diffuseColor;
		_properties.specularColor = other->_properties.specularColor;
		_properties.emissiveColor = other->_properties.emissiveColor;
		_properties.alphaToCoverageClamp = other->_properties.alphaToCoverageClamp;
		_properties.useAlphaToCoverage = other->_properties.useAlphaToCoverage;
		_properties.textureTileFactor = other->_properties.textureTileFactor;
		_properties.usePolygonOffset = other->_properties.usePolygonOffset;
		_properties.polygonOffsetFactor = other->_properties.polygonOffsetFactor;
		_properties.polygonOffsetUnits = other->_properties.polygonOffsetUnits;
		_properties.cullMode = other->_properties.cullMode;
		_properties.blendOperationRGB = other->_properties.blendOperationRGB;
		_properties.blendOperationAlpha = other->_properties.blendOperationAlpha;
		_properties.blendFactorSourceRGB = other->_properties.blendFactorSourceRGB;
		_properties.blendFactorSourceAlpha = other->_properties.blendFactorSourceAlpha;
		_properties.blendFactorDestinationRGB = other->_properties.blendFactorDestinationRGB;
		_properties.blendFactorDestinationAlpha = other->_properties.blendFactorDestinationAlpha;
		
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
		SafeRelease(_textures);
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
	
	void Material::SetColorWriteMask(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha)
	{
		_properties.colorWriteMask = 0;
		
		if(writeRed)
			_properties.colorWriteMask |= (1 << 0);
		if(writeGreen)
			_properties.colorWriteMask |= (1 << 1);
		if(writeBlue)
			_properties.colorWriteMask |= (1 << 2);
		if(writeAlpha)
			_properties.colorWriteMask |= (1 << 3);
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
	
	void Material::SetAlphaToCoverage(bool enabled, float min, float max)
	{
		_properties.useAlphaToCoverage = enabled;
		_properties.alphaToCoverageClamp.x = min;
		_properties.alphaToCoverageClamp.y = max;
	}
	
	void Material::SetBlendOperation(BlendOperation blendOperationRGB, BlendOperation blendOperationAlpha)
	{
		RN_ASSERT((blendOperationRGB != BlendOperation::None && blendOperationAlpha != BlendOperation::None) || blendOperationAlpha == blendOperationRGB, "Blend operation None can not be mixed with any of the others.");
		_properties.blendOperationRGB = blendOperationRGB;
		_properties.blendOperationAlpha = blendOperationAlpha;
	}
	
	void Material::SetBlendFactorSource(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha)
	{
		_properties.blendFactorSourceRGB = blendFactorRGB;
		_properties.blendFactorSourceAlpha = blendFactorAlpha;
	}
	
	void Material::SetBlendFactorDestination(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha)
	{
		_properties.blendFactorDestinationRGB = blendFactorRGB;
		_properties.blendFactorDestinationAlpha = blendFactorAlpha;
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

	Material::Properties Material::GetMergedProperties(Material *overrideMaterial) const
	{
		Properties properties = _properties;

		if(!overrideMaterial) return properties;
		
		if(!(overrideMaterial->GetOverride() & Override::ColorWriteMask) && !(_override & Override::ColorWriteMask))
		{
			properties.colorWriteMask = overrideMaterial->_properties.colorWriteMask;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupDepth) && !(_override & Override::GroupDepth))
		{
			properties.depthMode = overrideMaterial->_properties.depthMode;
			properties.depthWriteEnabled = overrideMaterial->_properties.depthWriteEnabled;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupColors) && !(_override & Override::GroupColors))
		{
			properties.ambientColor = overrideMaterial->_properties.ambientColor;
			properties.diffuseColor = overrideMaterial->_properties.diffuseColor;
			properties.specularColor = overrideMaterial->_properties.specularColor;
			properties.emissiveColor = overrideMaterial->_properties.emissiveColor;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupAlphaToCoverage) && !(_override & Override::GroupAlphaToCoverage))
		{
			properties.useAlphaToCoverage = overrideMaterial->_properties.useAlphaToCoverage;
			properties.alphaToCoverageClamp = overrideMaterial->_properties.alphaToCoverageClamp;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupBlending) && !(_override & Override::GroupBlending))
		{
			properties.blendOperationRGB = overrideMaterial->_properties.blendOperationRGB;
			properties.blendOperationAlpha = overrideMaterial->_properties.blendOperationAlpha;
			properties.blendFactorSourceRGB = overrideMaterial->_properties.blendFactorSourceRGB;
			properties.blendFactorSourceAlpha = overrideMaterial->_properties.blendFactorSourceAlpha;
			properties.blendFactorDestinationRGB = overrideMaterial->_properties.blendFactorDestinationRGB;
			properties.blendFactorDestinationAlpha = overrideMaterial->_properties.blendFactorDestinationAlpha;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::TextureTileFactor) && !(_override & Override::TextureTileFactor))
		{
			properties.textureTileFactor = overrideMaterial->_properties.textureTileFactor;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupPolygonOffset) && !(_override & Override::GroupPolygonOffset))
		{
			properties.usePolygonOffset = overrideMaterial->_properties.usePolygonOffset;
			properties.polygonOffsetFactor = overrideMaterial->_properties.polygonOffsetFactor;
			properties.polygonOffsetUnits = overrideMaterial->_properties.polygonOffsetUnits;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::CullMode) && !(_override & Override::CullMode))
		{
			properties.cullMode = overrideMaterial->_properties.cullMode;
		}

		return properties;
	}
}
