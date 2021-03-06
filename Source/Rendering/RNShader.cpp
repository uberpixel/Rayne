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
	RNDefineScopedMeta(Shader, Argument, Object)
	RNDefineScopedMeta(Shader, ArgumentBuffer, Shader::Argument)
	RNDefineScopedMeta(Shader, ArgumentSampler, Shader::Argument)
	RNDefineScopedMeta(Shader, ArgumentTexture, Shader::Argument)
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

	Shader::Options::Options(const Options *options) : _defines(options->_defines->Copy())
	{
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
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::UVCoords1))
			AddDefine(RNCSTR("RN_UV1"), RNCSTR("1"));
		
		//TODO: This should be based on the model having a skeleton...
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::BoneWeights))
			AddDefine(RNCSTR("RN_ANIMATIONS"), RNCSTR("1"));
	}

    Shader::Options::~Options()
    {
	    SafeRelease(_defines);
    }

	Shader::Options *Shader::Options::EnableAlpha()
	{
		return AddDefine(RNCSTR("RN_ALPHA"), RNCSTR("1"));
	}

	Shader::Options *Shader::Options::EnablePointLights()
	{
		return AddDefine(RNCSTR("RN_LIGHTS_POINT"), RNCSTR("1"));
	}

	Shader::Options *Shader::Options::EnableDirectionalLights()
	{
		return AddDefine(RNCSTR("RN_LIGHTS_DIRECTIONAL"), RNCSTR("1"));
	}

	Shader::Options *Shader::Options::EnableDirectionalShadows()
	{
		return AddDefine(RNCSTR("RN_SHADOWS_DIRECTIONAL"), RNCSTR("1"));
	}

	Shader::Options *Shader::Options::EnableMultiview()
	{
		return AddDefine(RNCSTR("RN_USE_MULTIVIEW"), RNCSTR("1"));
	}

	Shader::Options *Shader::Options::AddDefine(String *name, String *value)
	{
		_defines->SetObjectForKey(value, name);
		return this;
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
	{
		if(name->IsEqual(RNCSTR("global_time")) || name->IsEqual(RNCSTR("time")))
		{
			_identifier = Time;
			_type = PrimitiveType::Float;
		}
		else if(name->IsEqual(RNCSTR("transform_model")) || name->IsEqual(RNCSTR("modelMatrix")))
		{
			_identifier = ModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodel")) || name->IsEqual(RNCSTR("inverseModelMatrix")))
		{
			_identifier = InverseModelMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelview")) || name->IsEqual(RNCSTR("modelViewMatrix")))
		{
			_identifier = ModelViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelview_multiview")) || name->IsEqual(RNCSTR("modelViewMatrix_multiview")))
		{
			_identifier = ModelViewMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelviewprojection")) || name->IsEqual(RNCSTR("modelViewProjectionMatrix")))
		{
			_identifier = ModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_modelviewprojection_multiview")) || name->IsEqual(RNCSTR("modelViewProjectionMatrix_multiview")))
		{
			_identifier = ModelViewProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_view")) || name->IsEqual(RNCSTR("viewMatrix")))
		{
			_identifier = ViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_view_multiview")) || name->IsEqual(RNCSTR("viewMatrix_multiview")))
		{
			_identifier = ViewMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_viewprojection")) || name->IsEqual(RNCSTR("viewProjectionMatrix")))
		{
			_identifier = ViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_viewprojection_multiview")) || name->IsEqual(RNCSTR("viewProjectionMatrix_multiview")))
		{
			_identifier = ViewProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_projection")) || name->IsEqual(RNCSTR("projectionMatrix")))
		{
			_identifier = ProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_projection_multiview")) || name->IsEqual(RNCSTR("projectionMatrix_multiview")))
		{
			_identifier = ProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelview")) || name->IsEqual(RNCSTR("inverseModelViewMatrix")))
		{
			_identifier = InverseModelViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelview_multiview")) || name->IsEqual(RNCSTR("inverseModelViewMatrix_multiview")))
		{
			_identifier = InverseModelViewMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelviewprojection")) || name->IsEqual(RNCSTR("inverseModelViewProjectionMatrix")))
		{
			_identifier = InverseModelViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inversemodelviewprojection_multiview")) || name->IsEqual(RNCSTR("inverseModelViewProjectionMatrix_multiview")))
		{
			_identifier = InverseModelViewProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseview")) || name->IsEqual(RNCSTR("inverseViewMatrix")))
		{
			_identifier = InverseViewMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseview_multiview")) || name->IsEqual(RNCSTR("inverseViewMatrix_multiview")))
		{
			_identifier = InverseViewMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseviewprojection")) || name->IsEqual(RNCSTR("inverseViewProjectionMatrix")))
		{
			_identifier = InverseViewProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseviewprojection_multiview")) || name->IsEqual(RNCSTR("inverseViewProjectionMatrix_multiview")))
		{
			_identifier = InverseViewProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseprojection")) || name->IsEqual(RNCSTR("inverseProjectionMatrix")))
		{
			_identifier = InverseProjectionMatrix;
			_type = PrimitiveType::Matrix;
		}
		else if(name->IsEqual(RNCSTR("transform_inverseprojection_multiview")) || name->IsEqual(RNCSTR("inverseProjectionMatrix_multiview")))
		{
			_identifier = InverseProjectionMatrixMultiview;
			_type = PrimitiveType::Matrix;
		}
		else if (name->IsEqual(RNCSTR("camera_position_multiview")) || name->IsEqual(RNCSTR("cameraPosition_multiview")))
		{
			_identifier = CameraPositionMultiview;
			_type = PrimitiveType::Vector4;
		}
		else if (name->IsEqual(RNCSTR("camera_position")) || name->IsEqual(RNCSTR("cameraPosition")))
		{
			_identifier = CameraPosition;
			_type = PrimitiveType::Vector4;
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
		else if (name->IsEqual(RNCSTR("material_alphatocoverageclamp")) || name->IsEqual(RNCSTR("alphaToCoverageClamp")))
		{
			_identifier = AlphaToCoverageClamp;
			_type = PrimitiveType::Vector2;
		}
		else if (name->IsEqual(RNCSTR("camera_ambientcolor")) || name->IsEqual(RNCSTR("cameraAmbientColor")))
		{
			_identifier = CameraAmbientColor;
			_type = PrimitiveType::Color;
		}
		else if (name->IsEqual(RNCSTR("lights_directionalcount")) || name->IsEqual(RNCSTR("directionalLightsCount")))
		{
			_identifier = DirectionalLightsCount;
			_type = PrimitiveType::Uint32;
		}
		else if (name->IsEqual(RNCSTR("lights_directional")) || name->IsEqual(RNCSTR("directionalLights")))
		{
			_identifier = DirectionalLights;
			_type = PrimitiveType::Vector4;
		}
		else if (name->IsEqual(RNCSTR("shadows_directional_matrices")) || name->IsEqual(RNCSTR("directionalShadowMatrices")))
		{
			_identifier = DirectionalShadowMatrices;
			_type = PrimitiveType::Matrix;
		}
		else if (name->IsEqual(RNCSTR("shadows_directional_matrices_count")) || name->IsEqual(RNCSTR("directionalShadowMatricesCount")))
		{
			_identifier = DirectionalShadowMatricesCount;
			_type = PrimitiveType::Uint32;
		}
		else if (name->IsEqual(RNCSTR("shadows_directional_info")) || name->IsEqual(RNCSTR("directionalShadowInfo")))
		{
			_identifier = DirectionalShadowInfo;
			_type = PrimitiveType::Vector2;
		}
		else if (name->IsEqual(RNCSTR("lights_point")) || name->IsEqual(RNCSTR("pointLights")))
		{
			_identifier = PointLights;
			_type = PrimitiveType::Vector4;
		}
		else if (name->IsEqual(RNCSTR("lights_spot")) || name->IsEqual(RNCSTR("spotLights")))
		{
			_identifier = SpotLights;
			_type = PrimitiveType::Vector4;
		}
		else if (name->IsEqual(RNCSTR("bone_matrices")) || name->IsEqual(RNCSTR("boneMatrices")))
		{
			_identifier = BoneMatrices;
			_type = PrimitiveType::Matrix;
		}
	}

	Shader::UniformDescriptor::~UniformDescriptor()
	{
		_name->Release();
	}

	size_t Shader::UniformDescriptor::GetSize() const
	{
		//TODO: Handle alignment!
		switch(_identifier)
		{
			case DirectionalLights:
				return (16 + 16) * 5;	//TODO: use define or something for the 5

			case DirectionalShadowMatrices:
				return 64 * 4;	//TODO: use define or something for the 4
				
			case PointLights:
				return (16 + 16) * 8;	//TODO: use define or something for the 8
				
			case SpotLights:
				return (16 + 16 + 16) * 8;	//TODO: use define or something for the 8
				
			case BoneMatrices:
				return 64 * 100; //TODO: Handle the 100 bones limit in some better way

			case ModelViewMatrixMultiview:
				return 64 * 6;

			case ModelViewProjectionMatrixMultiview:
				return 64 * 6;

			case ViewMatrixMultiview:
				return 64 * 6;

			case ViewProjectionMatrixMultiview:
				return 64 * 6;

			case ProjectionMatrixMultiview:
				return 64 * 6;

			case InverseModelViewMatrixMultiview:
				return 64 * 6;

			case InverseModelViewProjectionMatrixMultiview:
				return 64 * 6;

			case InverseViewMatrixMultiview:
				return 64 * 6;

			case InverseViewProjectionMatrixMultiview:
				return 64 * 6;

			case InverseProjectionMatrixMultiview:
				return 64 * 6;

			case CameraPositionMultiview:
				return 16 * 6;

			default:
				break;
		}

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

	Shader::Argument::Argument(String *name, uint32 index) : _index(index)
	{
		_name = SafeRetain(name);
	}

	Shader::Argument::Argument(const Argument *other) : _index(other->_index)
	{
		_name = SafeRetain(other->_name);
	}

	Shader::Argument::~Argument()
	{
		SafeRelease(_name);
	}

	Shader::ArgumentBuffer::ArgumentBuffer(String *name, uint32 index, Array *uniformDescriptors) :
		Argument(name, index), _totalUniformSize(0)
	{
		_uniformDescriptors = SafeRetain(uniformDescriptors);
		
		if(_uniformDescriptors && _uniformDescriptors->GetCount() > 0)
		{
			Shader::UniformDescriptor *lastDescriptor = _uniformDescriptors->GetLastObject<Shader::UniformDescriptor>();
			_totalUniformSize = lastDescriptor->GetOffset() + lastDescriptor->GetSize();
		}
	}

	Shader::ArgumentBuffer::ArgumentBuffer(const ArgumentBuffer *other) : Argument(other)
	{
		_uniformDescriptors = SafeRetain(other->_uniformDescriptors);
	}

	Shader::ArgumentBuffer::~ArgumentBuffer()
	{
		SafeRelease(_uniformDescriptors);
	}

	Shader::ArgumentSampler::ArgumentSampler(String *name, uint32 index, WrapMode wrapMode, Filter filter, ComparisonFunction comparisonFunction, uint8 anisotropy) :
		Argument(name, index),
		_wrapMode(wrapMode),
		_filter(filter),
		_comparisonFunction(comparisonFunction),
		_anisotropy(anisotropy)
	{}

	Shader::ArgumentSampler::ArgumentSampler(const ArgumentSampler *other) : Argument(other), _wrapMode(other->_wrapMode), _filter(other->_filter), _comparisonFunction(other->_comparisonFunction), _anisotropy(other->_anisotropy)
	{}

	Shader::ArgumentSampler::~ArgumentSampler()
	{}

	uint32 Shader::ArgumentSampler::GetDefaultAnisotropy()
	{
		return _defaultAnisotropy;
	}

	void Shader::ArgumentSampler::SetDefaultAnisotropy(uint32 anisotropy)
	{
		RN_ASSERT(anisotropy >= 1 && anisotropy <= 16, "Anisotropy must be [1, 16]");
		_defaultAnisotropy = anisotropy;
	}

	Shader::ArgumentTexture::ArgumentTexture(String *name, uint32 index, uint8 materialTextureIndex) :
		Argument(name, index), _materialTextureIndex(materialTextureIndex)
	{
		
	}

	Shader::ArgumentTexture::ArgumentTexture(const ArgumentTexture *other) : Argument(other), _materialTextureIndex(other->_materialTextureIndex)
	{}

	Shader::ArgumentTexture::~ArgumentTexture()
	{
		
	}


	Shader::Signature::Signature(Array *buffers, Array *samplers, Array *textures)
	{
		_buffers = SafeRetain(buffers);
		_samplers = SafeRetain(samplers);
		_textures = SafeRetain(textures);
	}

	Shader::Signature::~Signature()
	{
		SafeRelease(_buffers);
		SafeRelease(_samplers);
		SafeRelease(_textures);
	}


	Shader::Shader(ShaderLibrary *library, Type type, const Shader::Options *options, const Signature *signature) :
		_options(options->Retain()), _library(library), _type(type), _signature(signature->Retain())
	{}

	Shader::Shader(ShaderLibrary *library, Type type, const Shader::Options *options) :
		_options(options->Retain()), _library(library), _type(type), _signature(nullptr)
	{}

	Shader::~Shader()
	{
		SafeRelease(_options);
		SafeRelease(_signature);
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
