//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNShaderLibrary.h"
#include "RNMesh.h"

namespace RN
{
	RNDefineMeta(Shader, Object)
	RNDefineScopedMeta(Shader, Options, Object)
	RNDefineScopedMeta(Shader, UniformDescriptor, Object)
	RNDefineScopedMeta(Shader, Sampler, Object)
	RNDefineScopedMeta(Shader, Signature, Object)

	static uint8 _defaultAnisotropy = 16;

	Shader::Options *Shader::Options::WithMesh(Mesh *mesh)
	{
		Shader::Options *options = new Shader::Options(mesh);
		return options->Autorelease();
	}

	Shader::Options *Shader::Options::WithNone()
	{
		Shader::Options *options = new Shader::Options();
		return options->Autorelease();
	}

	Shader::Options::Options() : _defines(new Dictionary())
	{
	}

	Shader::Options::Options(Mesh *mesh) : _defines(new Dictionary())
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

	void Shader::Options::EnableDiscard()
	{
		AddDefine(RNCSTR("RN_DISCARD"), RNCSTR("1"));
	}

	void Shader::Options::AddDefine(String *name, String *value)
	{
		_defines->SetObjectForKey(value, name);
	}

	bool Shader::Options::IsEqual(const Object *other) const
	{
		const Shader::Options *options = other->Downcast<Shader::Options>();
		if(RN_EXPECT_FALSE(!options))
			return false;

		if(!options->_defines->IsEqual(_defines))
			return false;

		return true;
	}
	size_t Shader::Options::GetHash() const
	{
		return _defines->GetHash();
	}

	Shader::UniformDescriptor::UniformDescriptor(const String *name, PrimitiveType type, size_t offset) :
		_name(name->Copy()),
		_identifier(Identifier::Custom),
		_type(type),
		_offset(offset)
	{}

	Shader::UniformDescriptor::UniformDescriptor(const String *name, size_t offset) :
		_name(name->Copy()), _offset(offset)
	{
		if(name->IsEqual(RNCSTR("global_time")) || name->IsEqual(RNCSTR("time")))
		{
			_identifier = Time;
			_type = PrimitiveType::Float;
		}

		if(name->IsEqual(RNCSTR("transform_model")) || name->IsEqual(RNCSTR("modelMatrix")))
		{
			_identifier = ModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelview")) || name->IsEqual(RNCSTR("modelViewMatrix")))
		{
			_identifier = ModelViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelviewprojection")) || name->IsEqual(RNCSTR("modelViewProjectionMatrix")))
		{
			_identifier = ModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_view")) || name->IsEqual(RNCSTR("viewMatrix")))
		{
			_identifier = ViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_viewprojection")) || name->IsEqual(RNCSTR("viewProjectionMatrix")))
		{
			_identifier = ViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_projection")) || name->IsEqual(RNCSTR("projectionMatrix")))
		{
			_identifier = ProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodel")) || name->IsEqual(RNCSTR("inverseModelMatrix")))
		{
			_identifier = InverseModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelview")) || name->IsEqual(RNCSTR("inverseModelViewMatrix")))
		{
			_identifier = InverseModelViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelviewprojection")) || name->IsEqual(RNCSTR("inverseModelViewProjectionMatrix")))
		{
			_identifier = InverseModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseview")) || name->IsEqual(RNCSTR("inverseViewMatrix")))
		{
			_identifier = InverseViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseviewprojection")) || name->IsEqual(RNCSTR("inverseViewProjectionMatrix")))
		{
			_identifier = InverseViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseprojection")) || name->IsEqual(RNCSTR("inverseProjectionMatrix")))
		{
			_identifier = InverseProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("material_ambientcolor")) || name->IsEqual(RNCSTR("ambientColor")))
		{
			_identifier = AmbientColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_diffusecolor")) || name->IsEqual(RNCSTR("diffuseColor")))
		{
			_identifier = DiffuseColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_specularcolor")) || name->IsEqual(RNCSTR("specularColor")))
		{
			_identifier = SpecularColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_emissivecolor")) || name->IsEqual(RNCSTR("emissiveColor")))
		{
			_identifier = EmissiveColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_texturetilefactor")) || name->IsEqual(RNCSTR("textureTileFactor")))
		{
			_identifier = TextureTileFactor;
			_type = PrimitiveType::Float;
		}
		else if(name->IsEqual(RNCSTR("material_discardthreshold")) || name->IsEqual(RNCSTR("discardThreshold")))
		{
			_identifier = DiscardThreshold;
			_type = PrimitiveType::Float;
		}
	}

	Shader::UniformDescriptor::~UniformDescriptor()
	{
		_name->Release();
	}

	size_t Shader::UniformDescriptor::GetSize() const
	{
		switch(_type)
		{
		case PrimitiveType::Uint8:
			return 1;
		case PrimitiveType::Uint16:
			return 2;
		case PrimitiveType::Uint32:
			return 4;
		case PrimitiveType::Int8:
			return 1;
		case PrimitiveType::Int16:
			return 2;
		case PrimitiveType::Int32:
			return 4;
		case PrimitiveType::Float:
			return 4;
		case PrimitiveType::Vector2:
			return 8;
		case PrimitiveType::Vector3:
			return 12;
		case PrimitiveType::Vector4:
			return 16;
		case PrimitiveType::Matrix:
			return 64;
		case PrimitiveType::Quaternion:
			return 16;
		case PrimitiveType::Color:
			return 16;
		}

		return 0;
	}


	Shader::Sampler::Sampler(WrapMode wrapMode, Filter filter, uint8 anisotropy) :
		filter(filter),
		wrapMode(wrapMode),
		anisotropy(anisotropy)
	{}

	Shader::Sampler::~Sampler()
	{}

	uint32 Shader::Sampler::GetDefaultAnisotropy()
	{
		return _defaultAnisotropy;
	}

	void Shader::Sampler::SetDefaultAnisotropy(uint32 anisotropy)
	{
		RN_ASSERT(anisotropy >= 1 && anisotropy <= 16, "Anisotropy must be [1, 16]");
		_defaultAnisotropy = anisotropy;
	}


	Shader::Signature::Signature(Array *uniformDescriptors, Array *samplers, uint8 textureCount) :
		_uniformDescriptors(uniformDescriptors->Retain()),
		_samplers(samplers->Retain()),
		_textureCount(textureCount),
		_totalUniformSize(0)
	{
		_uniformDescriptors->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			_totalUniformSize += descriptor->GetSize();
		});
	}

	Shader::Signature::~Signature()
	{
		_uniformDescriptors->Release();
	}


	Shader::Shader(ShaderLibrary *library, Type type, const Shader::Options *options, const Signature *signature) :
		_options(options->Retain()), _library(library), _type(type), _signature(signature->Retain())
	{}

	Shader::Shader(ShaderLibrary *library, Type type, const Shader::Options *options) :
		_options(options->Retain()), _library(library), _type(type), _signature(nullptr)
	{}

	Shader::~Shader()
	{
		_options->Release();
		_signature->Release();
	}

	Shader::Type Shader::GetType() const
	{
		return _type;
	}

	const Shader::Options *Shader::GetOptions() const
	{
		//TODO: maybe retain and autorelease!?
		return _options;
	}

	void Shader::SetSignature(const Signature *signature)
	{
		RN_ASSERT(!_signature, "Shader signature can only be set once!");
		_signature = signature->Retain();
	}

	const Shader::Signature *Shader::GetSignature() const
	{
		//TODO: maybe retain and autorelease!?
		return _signature;
	}

	ShaderLibrary *Shader::GetLibrary() const
	{
		return _library;
	}
}
