//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"

namespace RN
{
	RNDeclareMeta(Material)
	
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
					glUniform1iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int2:
					glUniform2iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int3:
					glUniform3iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
				case Type::Int4:
					glUniform4iv(location, 1, static_cast<GLint *>(GetPointerValue()));
					break;
					
				case Type::UInt1:
					glUniform1uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt2:
					glUniform2uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt3:
					glUniform3uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
				case Type::UInt4:
					glUniform4uiv(location, 1, static_cast<GLuint *>(GetPointerValue()));
					break;
					
				case Type::Float1:
					glUniform1fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float2:
					glUniform2fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float3:
					glUniform3fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
				case Type::Float4:
					glUniform4fv(location, 1, static_cast<GLfloat *>(GetPointerValue()));
					break;
					
				case Type::Matrix:
					glUniformMatrix4fv(location, 1, GL_FALSE, static_cast<GLfloat *>(GetPointerValue()));
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
		_shader = 0;
		Initialize();
	}
	
	Material::Material(Shader *shader) :
		_lookup(0)
	{
		_shader = shader ? shader->Retain() : 0;
		Initialize();
	}
	
	Material::~Material()
	{
		if(_shader)
			_shader->Release();
	}
	
	void Material::Initialize()
	{
		override = 0;
		lighting = true;
		
		culling  = true;
		cullmode = GL_CCW;
		
		blending = false;
		blendSource = GL_ONE;
		blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		
		polygonOffset = false;
		polygonOffsetFactor = 1.0f;
		polygonOffsetUnits = 1.0f;
		
		ambient = Color(0.02f, 0.02f, 0.02f, 1.0f);
		diffuse = Color(0.8f, 0.8f, 0.8f, 1.0f);
		specular = Color(1.0f, 1.0f, 1.0f, 4.0f);
		emissive = Color(0.0f, 0.0f, 0.0f, 0.0f);
		
		shininess = 0.0f;
		
		depthtest = true;
		depthtestmode = GL_LEQUAL;
		depthwrite = true;
		
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
	
	void Material::SetBlendMode(BlendMode mode)
	{
		switch(mode)
		{
			case BlendMode::Additive:
				blendSource = GL_ONE;
				blendDestination = GL_ONE;
				break;
				
			case BlendMode::Multiplicative:
				blendSource = GL_DST_COLOR;
				blendDestination = GL_ZERO;
				break;
				
			case BlendMode::Interpolative:
				blendSource = GL_ONE;
				blendDestination = GL_ONE_MINUS_SRC_ALPHA;
				break;
				
			case BlendMode::Cutout:
				blendSource = GL_ZERO;
				blendDestination = GL_ONE_MINUS_SRC_ALPHA;
				break;
		}
	}
	
	Shader *Material::GetShader() const
	{
		return _shader;
	}
	
	
	
	void Material::AddTexture(Texture *texture)
	{
		_textures.AddObject(texture);
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
		char buffer[32];
		sprintf(buffer, "%i", value);
		
		Define(define, buffer);
	}
	
	void Material::Define(const std::string& define, float value)
	{
		char buffer[32];
		sprintf(buffer, "%f", value);
		
		_defines.emplace_back(ShaderDefine(define, buffer));
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
