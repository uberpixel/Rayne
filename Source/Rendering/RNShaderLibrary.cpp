//
//  RNShaderLibrary.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderLibrary.h"
#include "../Math/RNAlgorithm.h"
#include "../Rendering/RNMesh.h"
#include "../Rendering/RNMaterial.h"

namespace RN
{
	RNDefineMeta(ShaderOptions, Object)

	ShaderOptions::ShaderOptions(Shader::Type shaderType) : _defines(new Dictionary()), _type(shaderType)
	{
	}

	ShaderOptions::ShaderOptions(Mesh *mesh, Shader::Type shaderType) : _defines(new Dictionary()), _type(shaderType)
	{
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Normals))
			AddDefine(RNCSTR("RN_NORMALS"), RNCSTR("1"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Tangents))
			AddDefine(RNCSTR("RN_TANGENTS"), RNCSTR("1"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Color0))
			AddDefine(RNCSTR("RN_COLOR"), RNCSTR("1"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::UVCoords0))
			AddDefine(RNCSTR("RN_UV0"), RNCSTR("1"));
	}

	void ShaderOptions::EnableDiscard()
	{
		AddDefine(RNCSTR("RN_DISCARD"), RNCSTR("1"));
	}

	void ShaderOptions::AddDefine(String *name, String *value)
	{
		_defines->SetObjectForKey(value, name);
	}

	bool ShaderOptions::IsEqual(const Object *other) const
	{
		const ShaderOptions *options = other->Downcast<ShaderOptions>();
		if(RN_EXPECT_FALSE(!options))
			return false;

		if(!options->_defines->IsEqual(_defines) || _type != options->_type)
			return false;

		return true;
	}
	size_t ShaderOptions::GetHash() const
	{
		size_t hash = 0;

		HashCombine(hash, _defines->GetHash());
		HashCombine(hash, _type);

		return hash;
	}

	RNDefineMeta(ShaderLibrary, Object)

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{

	}
}
