//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNShaderLibrary.h"

namespace RN
{
	RNDefineMeta(Shader, Object)
	RNDefineScopedMeta(Shader, UniformDescriptor, Object)
	RNDefineScopedMeta(Shader, Signature, Object)

	Shader::UniformDescriptor::UniformDescriptor(const String *name, PrimitiveType type, size_t offset) :
		_name(name->Copy()),
		_type(type),
		_offset(offset),
		_identifier(Identifier::Custom)
	{}

	Shader::UniformDescriptor::UniformDescriptor(const String *name, size_t offset) :
		_name(name->Copy()), _offset(offset)
	{
		if(name->IsEqual(RNCSTR("transform_model")))
		{
			_identifier = ModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelview")))
		{
			_identifier = ModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelviewprojection")))
		{
			_identifier = ModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_view")))
		{
			_identifier = ViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_viewprojection")))
		{
			_identifier = ViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodel")))
		{
			_identifier = InverseModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelview")))
		{
			_identifier = InverseModelViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelviewprojection")))
		{
			_identifier = InverseModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseview")))
		{
			_identifier = InverseViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseviewprojection")))
		{
			_identifier = InverseViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("material_ambientcolor")))
		{
			_identifier = AmbientColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_diffusecolor")))
		{
			_identifier = DiffuseColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_specularcolor")))
		{
			_identifier = SpecularColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_emissivecolor")))
		{
			_identifier = EmissiveColor;
			_type = PrimitiveType::Color;
		}
		else if(name->IsEqual(RNCSTR("material_texturetilefactor")))
		{
			_identifier = TextureTileFactor;
			_type = PrimitiveType::Float;
		}
		else if(name->IsEqual(RNCSTR("material_discardthreshold")))
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


	Shader::Signature::Signature(Array *uniformDescriptors, uint8 samplerCount, uint8 textureCount) :
		_uniformDescriptors(uniformDescriptors->Retain()),
		_samplerCount(samplerCount),
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


	Shader::Shader(ShaderLibrary *library, Type type, const ShaderOptions *options, const Signature *signature) :
		_options(options->Retain()), _library(library), _type(type), _signature(signature->Retain())
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

	const ShaderOptions *Shader::GetShaderOptions() const
	{
		//TODO: maybe retain and autorelease!?
		return _options;
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
