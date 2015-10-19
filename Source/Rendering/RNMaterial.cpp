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
		fragmentShader(nullptr),
		vertexShader(nullptr),
		_textures(new Array())
	{}

	MaterialDescriptor::~MaterialDescriptor()
	{
		SafeRelease(_textures);
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
		_fragmentShader(SafeRetain(descriptor.fragmentShader)),
		_vertexShader(SafeRetain(descriptor.vertexShader)),
		_depthMode(DepthMode::Less),
		_depthWriteEnabled(true),
		_ambientColor(1.0f, 1.0f, 1.0f, 1.0f),
		_diffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
		_specularColor(1.0f, 1.0f, 1.0f, 4.0f),
		_emissiveColor(0.0f, 0.0f, 0.0f, 0.0f)
	{
		RN_ASSERT(!_fragmentShader || _fragmentShader->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		RN_ASSERT(_vertexShader->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
	}

	Material::~Material()
	{
		SafeRelease(_fragmentShader);
		SafeRelease(_vertexShader);
	}

	void Material::SetDepthWriteEnabled(bool depthWrite)
	{
		_depthWriteEnabled = depthWrite;
	}
	void Material::SetDepthMode(DepthMode mode)
	{
		_depthMode = mode;
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
}
