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

	Material::Properties::Properties() : colorWriteMask(0xf), depthMode(DepthMode::Less), depthWriteEnabled(true), ambientColor(Color(0.5f, 0.5f, 0.5f, 1.0f)), diffuseColor(Color(1.0f, 1.0f, 1.0f, 1.0f)), specularColor(Color(1.0f, 1.0f, 1.0f, 4.0f)), emissiveColor(Color(0.0f, 0.0f, 0.0f, 0.0f)), usePolygonOffset(false), polygonOffsetFactor(1.1f), polygonOffsetUnits(0.1f), useAlphaToCoverage(false), alphaToCoverageClamp(1.0f), textureTileFactor(1.0f), cullMode(CullMode::BackFace), blendOperationRGB(BlendOperation::None), blendOperationAlpha(BlendOperation::None), blendFactorSourceRGB(BlendFactor::SourceAlpha), blendFactorDestinationRGB(BlendFactor::OneMinusSourceAlpha), blendFactorSourceAlpha(BlendFactor::SourceAlpha), blendFactorDestinationAlpha(BlendFactor::OneMinusSourceAlpha), _customShaderUniforms(nullptr)
	{
		
	}

	Material::Properties::Properties(const Properties &properties) : colorWriteMask(properties.colorWriteMask), depthMode(properties.depthMode), depthWriteEnabled(properties.depthWriteEnabled), ambientColor(properties.ambientColor), diffuseColor(properties.diffuseColor), specularColor(properties.specularColor), emissiveColor(properties.emissiveColor), usePolygonOffset(properties.usePolygonOffset), polygonOffsetFactor(properties.polygonOffsetFactor), polygonOffsetUnits(properties.polygonOffsetUnits), useAlphaToCoverage(properties.useAlphaToCoverage), alphaToCoverageClamp(properties.alphaToCoverageClamp), textureTileFactor(properties.textureTileFactor), cullMode(properties.cullMode), blendOperationRGB(properties.blendOperationRGB), blendOperationAlpha(properties.blendOperationAlpha), blendFactorSourceRGB(properties.blendFactorSourceRGB), blendFactorDestinationRGB(properties.blendFactorDestinationRGB), blendFactorSourceAlpha(properties.blendFactorSourceAlpha), blendFactorDestinationAlpha(properties.blendFactorDestinationAlpha), _customShaderUniforms(nullptr)
	{
		if(properties._customShaderUniforms)
		{
			_customShaderUniforms = properties._customShaderUniforms->Copy();
		}
	}

	Material::Properties::~Properties()
	{
		SafeRelease(_customShaderUniforms);
	}

	void Material::Properties::SetCustomShaderUniform(const String *name, Value *value)
	{
		if(!_customShaderUniforms) _customShaderUniforms = new RN::Dictionary();
		_customShaderUniforms->SetObjectForKey(value, name);
	}

	void Material::Properties::SetCustomShaderUniform(const String *name, Number *number)
	{
		if(!_customShaderUniforms) _customShaderUniforms = new RN::Dictionary();
		_customShaderUniforms->SetObjectForKey(number, name);
	}

	Object *Material::Properties::GetCustomShaderUniform(const String *name) const
	{
		if(!_customShaderUniforms) return nullptr;
		return _customShaderUniforms->GetObjectForKey(name);
	}

	Material::Material(Shader *vertexShader, Shader *fragmentShader) :
		_override(0),
		_textures(new Array()),
		_vertexBuffers(nullptr),
		_fragmentBuffers(nullptr),
		_skipRendering(false)
	{
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
		_fragmentBuffers(SafeCopy(other->_fragmentBuffers)),
		_skipRendering(other->_skipRendering),
		_properties(other->_properties)
	{
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
		RN_ASSERT(shader, "A valid fragment shader needs to be assigned!");
		SafeRelease(_fragmentShader[type]);
		_fragmentShader[type] = SafeRetain(shader);
		RN_ASSERT(!_fragmentShader[type] || _fragmentShader[type]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
	}
	
	void Material::SetVertexShader(Shader *shader, Shader::UsageHint type)
	{
		RN_ASSERT(shader, "A valid vertex shader needs to be assigned!");
		SafeRelease(_vertexShader[type]);
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

	void Material::SetCustomShaderUniform(const String *name, Value *value)
	{
		_properties.SetCustomShaderUniform(name, value);
	}

	void Material::SetCustomShaderUniform(const String *name, Number *number)
	{
		_properties.SetCustomShaderUniform(name, number);
	}

	Object *Material::GetCustomShaderUniform(const String *name) const
	{
		return _properties.GetCustomShaderUniform(name);
	}

	void Material::SetSkipRendering(bool skip)
	{
		_skipRendering = skip;
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
		
		if(!(overrideMaterial->GetOverride() & Override::CustomUniforms) && !(_override & Override::CustomUniforms))
		{
			if(overrideMaterial->_properties._customShaderUniforms)
			{
				if(!properties._customShaderUniforms)
				{
					properties._customShaderUniforms = overrideMaterial->_properties._customShaderUniforms->Retain();
				}
				else
				{
					//This seems to override any existing values, so exactly what I want here.
					properties._customShaderUniforms->AddEntriesFromDictionary(overrideMaterial->_properties._customShaderUniforms);
				}
			}
		}

		return properties;
	}
}
