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

	ShaderOptions *ShaderOptions::WithMesh(Mesh *mesh)
	{
		ShaderOptions *options = new ShaderOptions(mesh);
		return options->Autorelease();
	}

	ShaderOptions *ShaderOptions::WithNothing()
	{
		ShaderOptions *options = new ShaderOptions();
		return options->Autorelease();
	}

	ShaderOptions::ShaderOptions() : _defines(new Dictionary())
	{
	}

	ShaderOptions::ShaderOptions(Mesh *mesh) : _defines(new Dictionary())
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

		if(!options->_defines->IsEqual(_defines))
			return false;

		return true;
	}
	size_t ShaderOptions::GetHash() const
	{
		return _defines->GetHash();
	}

	RNDefineMeta(ShaderLibrary, Object)

	ShaderLibrary::ShaderLibrary()
	{

	}

	ShaderLibrary::~ShaderLibrary()
	{

	}
}
