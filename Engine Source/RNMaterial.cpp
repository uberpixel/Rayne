//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"

namespace RN
{
	RNDefineMeta(Material)
	
	Material::ShaderUniform::ShaderUniform(const std::string& name, Type type, void *data, size_t size, bool copy) :
		_name(name)
	{
		StoreData(type, data, size, copy);
	}
	
	Material::ShaderUniform::ShaderUniform(const std::string& name, const Matrix& matrix) :
		ShaderUniform(name, Type::Matrix, const_cast<Matrix *>(&matrix), sizeof(Matrix), true)
	{}
	
	Material::ShaderUniform::ShaderUniform(const std::string& name, const Vector2& vec2) :
		ShaderUniform(name, Type::Float2, const_cast<Vector2 *>(&vec2), sizeof(Vector2), true)
	{}
	Material::ShaderUniform::ShaderUniform(const std::string& name, const Vector3& vec3) :
		ShaderUniform(name, Type::Float3, const_cast<Vector3 *>(&vec3), sizeof(Vector3), true)
	{}
	Material::ShaderUniform::ShaderUniform(const std::string& name, const Vector4& vec4) :
		ShaderUniform(name, Type::Float4, const_cast<Vector4 *>(&vec4), sizeof(Vector4), true)
	{}
	
	Material::ShaderUniform::ShaderUniform(const std::string& name, float fValue) :
		ShaderUniform(name, Type::Float1, &fValue, sizeof(float), true)
	{}
	Material::ShaderUniform::ShaderUniform(const std::string& name, int32 iValue) :
		ShaderUniform(name, Type::Int1, &iValue, sizeof(int32), true)
	{}
	Material::ShaderUniform::ShaderUniform(const std::string& name, uint32 uiValue) :
		ShaderUniform(name, Type::UInt1, &uiValue, sizeof(uint32), true)
	{}
	
	
	void Material::ShaderUniform::SetFloatValue(float value)
	{
		StoreData(Type::Float1, &value, sizeof(float), true);
	}
	
	void Material::ShaderUniform::SetIntegerValue(int32 value)
	{
		StoreData(Type::Int1, &value, sizeof(int32), true);
	}
	
	void Material::ShaderUniform::SetUIntValue(uint32 value)
	{
		StoreData(Type::UInt1, &value, sizeof(uint32), true);
	}
	
	void Material::ShaderUniform::SetMatrix(const Matrix& matrix)
	{
		StoreData(Type::Matrix, const_cast<Matrix *>(&matrix), sizeof(Matrix), true);
	}
	void Material::ShaderUniform::SetVector(const Vector2& vector)
	{
		StoreData(Type::Float2, const_cast<Vector2 *>(&vector), sizeof(Vector2), true);
	}
	void Material::ShaderUniform::SetVector(const Vector3& vector)
	{
		StoreData(Type::Float3, const_cast<Vector3 *>(&vector), sizeof(Vector3), true);
	}
	void Material::ShaderUniform::SetVector(const Vector4& vector)
	{
		StoreData(Type::Float4, const_cast<Vector4 *>(&vector), sizeof(Vector4), true);
	}
	
	void Material::ShaderUniform::SetData(const void *data)
	{
		StoreData(_type, const_cast<void *>(data), _size, _rawStorage);
	}
	
	
	void Material::ShaderUniform::Apply(ShaderProgram *program)
	{
		GLuint location = program->GetCustomLocation(_name);
		if(location != -1)
		{
			switch(_type)
			{
				case Type::Int1:
					gl::Uniform1iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int2:
					gl::Uniform2iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int3:
					gl::Uniform3iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int4:
					gl::Uniform4iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
					
				case Type::UInt1:
					gl::Uniform1uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt2:
					gl::Uniform2uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt3:
					gl::Uniform3uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt4:
					gl::Uniform4uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
					
				case Type::Float1:
					gl::Uniform1fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float2:
					gl::Uniform2fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float3:
					gl::Uniform3fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float4:
					gl::Uniform4fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
					
				case Type::Matrix:
					gl::UniformMatrix4fv(location, 1, GL_FALSE, static_cast<GLfloat *>(GetPointerValue()));
					break;
			}
		}
	}
	
	void Material::ShaderUniform::StoreData(Type type, void *data, size_t size, bool copy)
	{
		uint8 *temp;
		
		if(copy)
		{
			temp = static_cast<uint8 *>(data);
		}
		else
		{
			temp = reinterpret_cast<uint8 *>(&data);
			size = sizeof(void *);
		}
		
		_rawStorage = copy;
		_type = type;
		_size = size;
		
		_storage.resize(size);
		std::copy(temp, temp + size, _storage.data());
	}
	
	void *Material::ShaderUniform::GetPointerValue()
	{
		return (_rawStorage) ? _storage.data() : static_cast<void *>(*(reinterpret_cast<uint32 **>(_storage.data())));
	}
	
	
	
	Material::Material() :
		_lookup(0)
	{
		Initialize();
		_shader = nullptr;
	}
	
	Material::Material(Shader *shader) :
		Material()
	{
		_shader = shader ? shader->Retain() : nullptr;
	}
	
	Material::Material(const Material *other) :
		Material()
	{
		_shader = SafeRetain(other->_shader);
		
		lighting = other->lighting;
		
		cullMode         = other->cullMode;
		polygonMode      = other->polygonMode;
		
		blending         = other->blending;
		blendEquation    = other->blendEquation;
		blendSource      = other->blendSource;
		blendDestination = other->blendDestination;
		
		alphaBlendEquation    = other->alphaBlendEquation;
		alphaBlendSource      = other->alphaBlendSource;
		alphaBlendDestination = other->alphaBlendDestination;
		
		polygonOffset       = other->polygonOffset;
		polygonOffsetFactor = other->polygonOffsetFactor;
		polygonOffsetUnits  = other->polygonOffsetUnits;
		
		ambient  = other->ambient;
		diffuse  = other->diffuse;
		specular = other->specular;
		emissive = other->emissive;
		
		depthTest    = other->depthTest;
		depthWrite   = other->depthWrite;
		depthTestMode = other->depthTestMode;
		
		discard = other->discard;
		discardThreshold = other->discardThreshold;
		
		override = other->override;
		
		_defines = other->_defines;
		_textures.AddObjectsFromArray(&other->_textures);
		
		UpdateLookupRequest();
	}
	
	Material::~Material()
	{
		SafeRelease(_shader);
	}
	
	void Material::Initialize()
	{
		lighting = true;
		
		cullMode = CullMode::BackFace;
		polygonMode = PolygonMode::Fill;
		
		blending = false;
		alphaBlendEquation = blendEquation = BlendEquation::Add;
		alphaBlendSource = blendSource = BlendMode::One;
		alphaBlendDestination = blendDestination = BlendMode::OneMinusSourceAlpha;
		
		polygonOffset = false;
		polygonOffsetFactor = 1.0f;
		polygonOffsetUnits = 1.0f;
		
		ambient = Color(1.0f, 1.0f, 1.0f, 1.0f);
		diffuse = Color(0.8f, 0.8f, 0.8f, 1.0f);
		specular = Color(1.0f, 1.0f, 1.0f, 4.0f);
		emissive = Color(0.0f, 0.0f, 0.0f, 0.0f);
		
		depthTest = true;
		depthTestMode = DepthMode::LessOrEqual;
		depthWrite = true;
		
		discard = false;
		discardThreshold = 0.3f;
	}
	
	
	
	void Material::SetShader(Shader *shader)
	{
		if(_shader)
			_shader->Release();
		
		_shader = shader ? shader->Retain() : 0;
		
		for(ShaderUniform *uniform : _uniforms)
			delete uniform;
	}
	
	Shader *Material::GetShader() const
	{
		return const_cast<Shader *>(static_cast<const Shader *>(_shader));
	}
	
	
	
	void Material::AddTexture(Texture *texture)
	{
		_textures.AddObject(texture);
	}
	
	void Material::InsertTexture(Texture *texture, size_t index)
	{
		if(index >= _textures.GetCount())
			throw Exception(Exception::Type::InvalidArgumentException, "index mustn't be out of bounds!");
		
		if(!texture)
			throw Exception(Exception::Type::InvalidArgumentException, "texture mustn't be null!");
		
		_textures.InsertObjectAtIndex(texture, index);
		
	}
	
	void Material::ReplaceTexture(Texture *texture, size_t index)
	{
		if(index >= _textures.GetCount())
			throw Exception(Exception::Type::InvalidArgumentException, "index mustn't be out of bounds!");
		
		if(!texture)
			throw Exception(Exception::Type::InvalidArgumentException, "texture mustn't be null!");
		
		_textures.ReplaceObjectAtIndex(index, texture);
	}
	
	void Material::RemoveTexture(Texture *texture)
	{
		_textures.RemoveObject(texture);
	}
	
	void Material::RemoveTextures()
	{
		_textures.RemoveAllObjects();
	}
	
	
	
	void Material::UpdateLookupRequest()
	{
		_lookup = ShaderLookup(_defines);
	}
	
	
	void Material::InsertShaderUniform(ShaderUniform *uniform)
	{
		_uniforms.push_back(uniform);
	}
	
	void Material::ApplyUniforms(ShaderProgram *program)
	{
		for(ShaderUniform *uniform : _uniforms)
		{
			uniform->Apply(program);
		}
	}
	
	
	void Material::Define(const std::string& define)
	{
		_defines.emplace_back(ShaderDefine(define, ""));
		UpdateLookupRequest();
	}
	
	void Material::Define(const std::string& define, const std::string& value)
	{
		_defines.emplace_back(ShaderDefine(define, value));
		UpdateLookupRequest();
	}
	
	void Material::Define(const std::string& define, int32 value)
	{
		std::stringstream stream;
		stream << value;
		
		Define(define, stream.str());
	}
	
	void Material::Define(const std::string& define, float value)
	{
		std::stringstream stream;
		stream << value;
		
		Define(define, stream.str());
	}
	
	void Material::Undefine(const std::string& name)
	{
		for(auto i=_defines.begin(); i!=_defines.end(); i++)
		{
			if(name == i->name)
			{
				_defines.erase(i);
				UpdateLookupRequest();
				return;
			}
		}
	}
}
