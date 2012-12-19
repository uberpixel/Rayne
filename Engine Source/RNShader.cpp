//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
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
	
	
	bool Shader::SetVertexShader(const std::string& path, Error *error)
	{
		return SetShaderForType(path, GL_VERTEX_SHADER, error);
	}
	
	bool Shader::SetFragmentShader(const std::string& path, Error *error)
	{
		return SetShaderForType(path, GL_FRAGMENT_SHADER, error);
	}
	
	bool Shader::SetGeometryShader(const std::string& path, Error *error)
	{
#ifdef GL_GEOMETRY_SHADER
		return SetShaderForType(path, GL_GEOMETRY_SHADER, error);
#else
		if(error)
			*error = Error(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported, "");
		
		return false;
#endif
	}
	
	bool Shader::SetShaderForType(const std::string& path, GLenum type, Error *error)
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
				if(error)
					*error = Error(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported, "");
				
				return false;
		}
		
		
		const GLchar *source = 0;
		GLuint shader = glCreateShader(type);
				
		if(shader == 0)
		{
			if(error)
				*error = Error(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported, "");
			
			return false;
		}
		
		glShaderSource(shader, 1, &source, NULL);
		glCompileShader(shader);
		
		
		GLint status, length;
		
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if(status == GL_FALSE && error)
		{
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		
			GLchar *log = new GLchar[length];
			glGetShaderInfoLog(shader, length, &length, log);
			
			*error = Error(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, std::string((char *)log));
			delete [] log;
		}
		
		return (status == GL_TRUE);
	}
	
	bool Shader::Link(Error *error)
	{
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
		if(status == GL_FALSE && error)
		{
			glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
			
			GLchar *log = new GLchar[length];
			glGetProgramInfoLog(_program, length, &length, log);
			
			*error = Error(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, std::string((char *)log));
			delete [] log;
		}
		else if(status == GL_TRUE)
		{
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
		
		return (status == GL_TRUE);
	}
}
