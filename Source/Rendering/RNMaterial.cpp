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

	Material::Properties::Properties() : colorWriteMask(0xf), depthMode(DepthMode::Greater), depthWriteEnabled(true), ambientColor(Color(0.5f, 0.5f, 0.5f, 1.0f)), diffuseColor(Color(1.0f, 1.0f, 1.0f, 1.0f)), specularColor(Color(1.0f, 1.0f, 1.0f, 4.0f)), emissiveColor(Color(0.0f, 0.0f, 0.0f, 0.0f)), usePolygonOffset(false), polygonOffsetFactor(-1.1f), polygonOffsetUnits(-0.1f), useAlphaToCoverage(false), alphaToCoverageClamp(1.0f), textureTileFactor(1.0f), cullMode(CullMode::BackFace), blendOperationRGB(BlendOperation::None), blendOperationAlpha(BlendOperation::None), blendFactorSourceRGB(BlendFactor::SourceAlpha), blendFactorDestinationRGB(BlendFactor::OneMinusSourceAlpha), blendFactorSourceAlpha(BlendFactor::SourceAlpha), blendFactorDestinationAlpha(BlendFactor::OneMinusSourceAlpha)
	{
		
	}

	Material::Properties::Properties(const Properties &properties)
    {
		CopyFromProperties(properties);
	}

	Material::Properties::~Properties()
	{
		for(auto const& data : _customShaderUniforms)
		{
			data.second->Release();
		}
	}

    void Material::Properties::CopyFromProperties(const Properties &properties)
    {
        colorWriteMask = properties.colorWriteMask;
        depthMode = properties.depthMode;
        depthWriteEnabled = properties.depthWriteEnabled;
        ambientColor = properties.ambientColor;
        diffuseColor = properties.diffuseColor;
        specularColor = properties.specularColor;
        emissiveColor = properties.emissiveColor;
        usePolygonOffset = properties.usePolygonOffset;
        polygonOffsetFactor = properties.polygonOffsetFactor;
        polygonOffsetUnits = properties.polygonOffsetUnits;
        useAlphaToCoverage = properties.useAlphaToCoverage;
        alphaToCoverageClamp = properties.alphaToCoverageClamp;
        textureTileFactor = properties.textureTileFactor;
        cullMode = properties.cullMode;
        blendOperationRGB = properties.blendOperationRGB;
        blendOperationAlpha = properties.blendOperationAlpha;
        blendFactorSourceRGB = properties.blendFactorSourceRGB;
        blendFactorDestinationRGB = properties.blendFactorDestinationRGB;
        blendFactorSourceAlpha = properties.blendFactorSourceAlpha;
        blendFactorDestinationAlpha = properties.blendFactorDestinationAlpha;
		
		_customShaderUniforms.clear();
		_customShaderUniforms.insert(properties._customShaderUniforms.begin(), properties._customShaderUniforms.end());
		
		for(auto const& data : _customShaderUniforms)
		{
			data.second->Retain();
		}
    }

	void Material::Properties::SetCustomShaderUniform(const String *name, Value *value)
	{
		_customShaderUniforms[name->GetHash()] = value->Retain();
	}

	void Material::Properties::SetCustomShaderUniform(const String *name, Number *number)
	{
		_customShaderUniforms[name->GetHash()] = number->Retain();
	}

	Object *Material::Properties::GetCustomShaderUniform(const String *name) const
	{
		const auto result = _customShaderUniforms.find(name->GetHash());
		if(result != _customShaderUniforms.end())
		{
			return result->second;
		}
		
		return nullptr;
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

	Material::Properties Material::GetMergedProperties(Material *overrideMaterial)
	{
		if(!overrideMaterial) return _properties;
		
		if(!(overrideMaterial->GetOverride() & Override::ColorWriteMask) && !(_override & Override::ColorWriteMask))
		{
			_mergedProperties.colorWriteMask = overrideMaterial->_properties.colorWriteMask;
		}
		else
		{
			_mergedProperties.colorWriteMask = _properties.colorWriteMask;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupDepth) && !(_override & Override::GroupDepth))
		{
			_mergedProperties.depthMode = overrideMaterial->_properties.depthMode;
			_mergedProperties.depthWriteEnabled = overrideMaterial->_properties.depthWriteEnabled;
		}
		else
		{
			_mergedProperties.depthMode = _properties.depthMode;
			_mergedProperties.depthWriteEnabled = _properties.depthWriteEnabled;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupColors) && !(_override & Override::GroupColors))
		{
			_mergedProperties.ambientColor = overrideMaterial->_properties.ambientColor;
			_mergedProperties.diffuseColor = overrideMaterial->_properties.diffuseColor;
			_mergedProperties.specularColor = overrideMaterial->_properties.specularColor;
			_mergedProperties.emissiveColor = overrideMaterial->_properties.emissiveColor;
		}
		else
		{
			_mergedProperties.ambientColor = _properties.ambientColor;
			_mergedProperties.diffuseColor = _properties.diffuseColor;
			_mergedProperties.specularColor = _properties.specularColor;
			_mergedProperties.emissiveColor = _properties.emissiveColor;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupAlphaToCoverage) && !(_override & Override::GroupAlphaToCoverage))
		{
			_mergedProperties.useAlphaToCoverage = overrideMaterial->_properties.useAlphaToCoverage;
			_mergedProperties.alphaToCoverageClamp = overrideMaterial->_properties.alphaToCoverageClamp;
		}
		else
		{
			_mergedProperties.useAlphaToCoverage = _properties.useAlphaToCoverage;
			_mergedProperties.alphaToCoverageClamp = _properties.alphaToCoverageClamp;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupBlending) && !(_override & Override::GroupBlending))
		{
			_mergedProperties.blendOperationRGB = overrideMaterial->_properties.blendOperationRGB;
			_mergedProperties.blendOperationAlpha = overrideMaterial->_properties.blendOperationAlpha;
			_mergedProperties.blendFactorSourceRGB = overrideMaterial->_properties.blendFactorSourceRGB;
			_mergedProperties.blendFactorSourceAlpha = overrideMaterial->_properties.blendFactorSourceAlpha;
			_mergedProperties.blendFactorDestinationRGB = overrideMaterial->_properties.blendFactorDestinationRGB;
			_mergedProperties.blendFactorDestinationAlpha = overrideMaterial->_properties.blendFactorDestinationAlpha;
		}
		else
		{
			_mergedProperties.blendOperationRGB = _properties.blendOperationRGB;
			_mergedProperties.blendOperationAlpha = _properties.blendOperationAlpha;
			_mergedProperties.blendFactorSourceRGB = _properties.blendFactorSourceRGB;
			_mergedProperties.blendFactorSourceAlpha = _properties.blendFactorSourceAlpha;
			_mergedProperties.blendFactorDestinationRGB = _properties.blendFactorDestinationRGB;
			_mergedProperties.blendFactorDestinationAlpha = _properties.blendFactorDestinationAlpha;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::TextureTileFactor) && !(_override & Override::TextureTileFactor))
		{
			_mergedProperties.textureTileFactor = overrideMaterial->_properties.textureTileFactor;
		}
		else
		{
			_mergedProperties.textureTileFactor = _properties.textureTileFactor;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::GroupPolygonOffset) && !(_override & Override::GroupPolygonOffset))
		{
			_mergedProperties.usePolygonOffset = overrideMaterial->_properties.usePolygonOffset;
			_mergedProperties.polygonOffsetFactor = overrideMaterial->_properties.polygonOffsetFactor;
			_mergedProperties.polygonOffsetUnits = overrideMaterial->_properties.polygonOffsetUnits;
		}
		else
		{
			_mergedProperties.usePolygonOffset = _properties.usePolygonOffset;
			_mergedProperties.polygonOffsetFactor = _properties.polygonOffsetFactor;
			_mergedProperties.polygonOffsetUnits = _properties.polygonOffsetUnits;
		}
		
		if(!(overrideMaterial->GetOverride() & Override::CullMode) && !(_override & Override::CullMode))
		{
			_mergedProperties.cullMode = overrideMaterial->_properties.cullMode;
		}
		else
		{
			_mergedProperties.cullMode = _properties.cullMode;
		}

		_mergedProperties._customShaderUniforms.clear();
		_mergedProperties._customShaderUniforms.insert(_properties._customShaderUniforms.begin(), _properties._customShaderUniforms.end());
		if(!(overrideMaterial->GetOverride() & Override::CustomUniforms) && !(_override & Override::CustomUniforms) && overrideMaterial->_properties._customShaderUniforms.size() > 0)
		{
			for(auto const &data : overrideMaterial->_properties._customShaderUniforms)
			{
				_mergedProperties._customShaderUniforms[data.first] = data.second;
			}
		}
		for(auto const &data : _mergedProperties._customShaderUniforms)
		{
			data.second->Retain();
		}

		return _mergedProperties;
	}
}
