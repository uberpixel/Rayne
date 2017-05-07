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

	MaterialDescriptor::MaterialDescriptor() :
		depthMode(DepthMode::Less),
		depthWriteEnabled(true),
		ambientColor(0.5f, 0.5f, 0.5f, 1.0f),
		diffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
		specularColor(1.0f, 1.0f, 1.0f, 4.0f),
		emissiveColor(0.0f, 0.0f, 0.0f, 0.0f),
		discardThreshold(0.3f),
		alphaToCoverageClamp(1.0f),
		useAlphaToCoverage(false),
		textureTileFactor(1.0),
		usePolygonOffset(false),
		polygonOffsetFactor(1.1f),
		polygonOffsetUnits(0.1f),
		cullMode(CullMode::BackFace),
		_textures(new Array())
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			vertexShader[i] = nullptr;
			fragmentShader[i] = nullptr;
		}
	}

	MaterialDescriptor::MaterialDescriptor(const MaterialDescriptor &other) :
		depthMode(other.depthMode),
		depthWriteEnabled(other.depthWriteEnabled),
		ambientColor(other.ambientColor),
		diffuseColor(other.diffuseColor),
		specularColor(other.specularColor),
		emissiveColor(other.emissiveColor),
		discardThreshold(other.discardThreshold),
		alphaToCoverageClamp(other.alphaToCoverageClamp),
		useAlphaToCoverage(other.useAlphaToCoverage),
		textureTileFactor(other.textureTileFactor),
		usePolygonOffset(other.usePolygonOffset),
		polygonOffsetFactor(other.polygonOffsetFactor),
		polygonOffsetUnits(other.polygonOffsetUnits),
		cullMode(other.cullMode),
		_textures(SafeCopy(other._textures))
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			vertexShader[i] = other.vertexShader[i];
			fragmentShader[i] = other.fragmentShader[i];
		}
	}

	MaterialDescriptor::~MaterialDescriptor()
	{
		SafeRelease(_textures);
	}


	void MaterialDescriptor::SetTextures(const Array *textures)
	{
		_textures->Release();
		_textures = textures->Copy();
	}

	void MaterialDescriptor::AddTexture(Texture *texture)
	{
		_textures->AddObject(texture);
	}
	void MaterialDescriptor::RemoveAllTextures()
	{
		_textures->RemoveAllObjects();
	}
	const Array *MaterialDescriptor::GetTextures() const
	{
		return _textures;
	}



	Material::Material(const MaterialDescriptor &descriptor) :
		_override(0),
		_textures(SafeCopy(descriptor.GetTextures())),
		_vertexBuffers(nullptr),
		_fragmentBuffers(nullptr),
		_depthMode(descriptor.depthMode),
		_depthWriteEnabled(descriptor.depthWriteEnabled),
		_ambientColor(descriptor.ambientColor),
		_diffuseColor(descriptor.diffuseColor),
		_specularColor(descriptor.specularColor),
		_emissiveColor(descriptor.emissiveColor),
		_discardThreshold(descriptor.discardThreshold),
		_alphaToCoverageClamp(descriptor.alphaToCoverageClamp),
		_useAlphaToCoverage(descriptor.useAlphaToCoverage),
		_textureTileFactor(descriptor.textureTileFactor),
		_usePolygonOffset(descriptor.usePolygonOffset),
		_polygonOffsetFactor(descriptor.polygonOffsetFactor),
		_polygonOffsetUnits(descriptor.polygonOffsetUnits),
		_cullMode(descriptor.cullMode)
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			_vertexShader[i] = SafeRetain(descriptor.vertexShader[i]);
			_fragmentShader[i] = SafeRetain(descriptor.fragmentShader[i]);

			RN_ASSERT(!_vertexShader[i] || _vertexShader[i]->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
			RN_ASSERT(!_fragmentShader[i] || _fragmentShader[i]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		}

		RN_ASSERT(_vertexShader[0], "Default vertex shader must be set");
	}

	Material::Material(const Material *other) :
		_override(0),
		_textures(SafeRetain(other->_textures)),
		_vertexBuffers(SafeCopy(other->_vertexBuffers)),
		_fragmentBuffers(SafeCopy(other->_fragmentBuffers)),
		_depthMode(other->_depthMode),
		_depthWriteEnabled(other->_depthWriteEnabled),
		_ambientColor(other->_ambientColor),
		_diffuseColor(other->_diffuseColor),
		_specularColor(other->_specularColor),
		_emissiveColor(other->_emissiveColor),
		_discardThreshold(other->_discardThreshold),
		_useAlphaToCoverage(other->_useAlphaToCoverage),
		_alphaToCoverageClamp(other->_alphaToCoverageClamp),
		_textureTileFactor(other->_textureTileFactor),
		_usePolygonOffset(other->_usePolygonOffset),
		_polygonOffsetFactor(other->_polygonOffsetFactor),
		_polygonOffsetUnits(other->_polygonOffsetUnits),
		_cullMode(other->_cullMode)
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			_vertexShader[i] = SafeRetain(other->_vertexShader[i]);
			_fragmentShader[i] = SafeRetain(other->_fragmentShader[i]);
		}
	}

	Material *Material::WithDescriptor(const MaterialDescriptor &descriptor)
	{
		Material *material = new Material(descriptor);
		return material->Autorelease();
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

	void Material::SetOverride(Override override)
	{
		_override = override;
	}

	void Material::SetDepthWriteEnabled(bool depthWrite)
	{
		_depthWriteEnabled = depthWrite;
	}
	void Material::SetDepthMode(DepthMode mode)
	{
		_depthMode = mode;
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
		_discardThreshold = threshold;
	}

	void Material::SetTextureTileFactor(float factor)
	{
		_textureTileFactor = factor;
	}

	void Material::SetAmbientColor(const Color &color)
	{
		_ambientColor = color;
	}
	void Material::SetDiffuseColor(const Color &color)
	{
		_diffuseColor = color;
	}
	void Material::SetSpecularColor(const Color &color)
	{
		_specularColor = color;
	}
	void Material::SetEmissiveColor(const Color &color)
	{
		_emissiveColor = color;
	}

	void Material::SetCullMode(CullMode mode)
	{
		_cullMode = mode;
	}

	void Material::SetPolygonOffset(bool enable, float factor, float units)
	{
		_usePolygonOffset = enable;
		_polygonOffsetFactor = factor;
		_polygonOffsetUnits = units;
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

	MaterialDescriptor Material::GetDescriptor() const
	{
		MaterialDescriptor descriptor;
		descriptor.SetTextures(GetTextures());
		descriptor.depthMode = _depthMode;
		descriptor.depthWriteEnabled = _depthWriteEnabled;
		descriptor.ambientColor = _ambientColor;
		descriptor.diffuseColor = _diffuseColor;
		descriptor.specularColor = _specularColor;
		descriptor.emissiveColor = _emissiveColor;
		descriptor.discardThreshold = _discardThreshold;
		descriptor.useAlphaToCoverage = _useAlphaToCoverage;
		descriptor.alphaToCoverageClamp = _alphaToCoverageClamp;
		descriptor.textureTileFactor = _textureTileFactor;
		descriptor.usePolygonOffset = _usePolygonOffset;
		descriptor.polygonOffsetFactor = _polygonOffsetFactor;
		descriptor.polygonOffsetUnits = _polygonOffsetUnits;
		descriptor.cullMode = _cullMode;

		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			descriptor.vertexShader[i] = _vertexShader[i];
			descriptor.fragmentShader[i] = _fragmentShader[i];
		}

		return descriptor;
	}
}
