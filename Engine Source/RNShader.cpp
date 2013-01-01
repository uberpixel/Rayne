//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"

namespace RN
{
	Shader::Shader()
	{
		_vertexShader = _fragmentShader = _fragmentShader = 0;
		_program = 0;
	}
	
	Shader::~Shader()
	{
		if(_vertexShader)
			glDeleteShader(_vertexShader);
		
		if(_fragmentShader)
			glDeleteShader(_fragmentShader);
		
		if(_geometryShader)
			glDeleteShader(_geometryShader);
		
		if(_program)
			glDeleteProgram(_program);
	}
	
	
	void Shader::SetVertexShader(const std::string& path)
	{
		SetShaderForType(path, GL_VERTEX_SHADER);
	}
	
	void Shader::SetVertexShader(File *file)
	{
		SetShaderForType(file, GL_VERTEX_SHADER);
	}
	
	void Shader::SetFragmentShader(const std::string& path)
	{
		SetShaderForType(path, GL_FRAGMENT_SHADER);
	}
	
	void Shader::SetFragmentShader(File *file)
	{
		SetShaderForType(file, GL_FRAGMENT_SHADER);
	}
	
	void Shader::SetGeometryShader(const std::string& path)
	{
#ifdef GL_GEOMETRY_SHADER
		SetShaderForType(path, GL_GEOMETRY_SHADER);
#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
#endif
	}
	
	void Shader::SetGeometryShader(File *file)
	{
#ifdef GL_GEOMETRY_SHADER
		SetShaderForType(file, GL_GEOMETRY_SHADER);
#else
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
#endif
	}
	
	
	
	void Shader::SetShaderForType(const std::vector<uint8>& data, GLenum type)
	{
		switch(type)
		{
			case GL_VERTEX_SHADER:
				if(_vertexShader)
				{
					glDeleteShader(_vertexShader);
					_vertexShader = 0;
				}
				break;
				
			case GL_FRAGMENT_SHADER:
				if(_fragmentShader)
				{
					glDeleteShader(_fragmentShader);
					_fragmentShader = 0;
				}
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				if(_geometryShader)
				{
					glDeleteShader(_geometryShader);
					_geometryShader = 0;
				}
				break;
#endif
				
			default:
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
				break;
		}
		
		
		const GLchar *source = (const GLchar *)&data[0];
		GLuint shader = glCreateShader(type);
		
		if(!shader == 0)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
		
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);
		
		// Get the compilation status
		GLint status, length;
		
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if(status == GL_FALSE)
		{
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			
			GLchar *log = new GLchar[length];
			glGetShaderInfoLog(shader, length, &length, log);
			
			std::string tlog = std::string((char *)log);
			delete [] log;
			
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, tlog);
		}
	}
	
	void Shader::SetShaderForType(File *file, GLenum type)
	{
		std::vector<uint8> bytes = file->Bytes();
		SetShaderForType(bytes, type);
	}
	
	void Shader::SetShaderForType(const std::string& path, GLenum type)
	{
		File *file = new File(path);
		SetShaderForType(file, type);
		file->Release();
	}
	
	void Shader::Link()
	{
		if(_program)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderAlreadyLinked);

		_program = glCreateProgram();
		
		if(_vertexShader)
			glAttachShader(_program, _vertexShader);
		
		if(_fragmentShader)
			glAttachShader(_program, _fragmentShader);
		
		if(_geometryShader)
			glAttachShader(_program, _geometryShader);
		
		
		glLinkProgram(_program);
		
		GLint status, length;
		
		glGetProgramiv(_program, GL_LINK_STATUS, &status);
		if(status == GL_FALSE)
		{
			glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
			
			GLchar *log = new GLchar[length];
			glGetProgramInfoLog(_program, length, &length, log);
			
			std::string tlog = std::string((char *)log);
			delete [] log;

			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, tlog);
		}
		else
		{
			// Detach and remove the shader
			if(_vertexShader)
			{
				glDetachShader(_program, _vertexShader);
				glDeleteShader(_vertexShader);
				
				_vertexShader = 0;
			}
			
			if(_fragmentShader)
			{
				glDetachShader(_program, _fragmentShader);
				glDeleteShader(_fragmentShader);
				
				_fragmentShader = 0;
			}
			
			if(_geometryShader)
			{
				glDetachShader(_program, _geometryShader);
				glDeleteShader(_geometryShader);
				
				_geometryShader = 0;
			}
			
			// Get uniforms
			_matProj = glGetUniformLocation(_program, "matProj");
			_matProjInverse = glGetUniformLocation(_program, "matProjInverse");
			
			_matView = glGetUniformLocation(_program, "matView");
			_matViewInverse = glGetUniformLocation(_program, "matViewInverse");
			
			_matModel = glGetUniformLocation(_program, "matModel");
			_matModelInverse = glGetUniformLocation(_program, "matModelInverse");
			
			_matProjViewModel = glGetUniformLocation(_program, "matProjViewModel");
			_matProjViewModelInverse = glGetUniformLocation(_program, "matProjViewModelInverse");
			
			// Get attributes
			_position = glGetAttribLocation(_program, "position");
			_texcoord0 = glGetAttribLocation(_program, "texcoord0");
			_texcoord1 = glGetAttribLocation(_program, "texcoord1");
		}
	}
}
