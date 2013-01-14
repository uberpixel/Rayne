//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNKernel.h"

namespace RN
{
	Shader::Shader()
	{
		_vertexShader = _fragmentShader = _geometryShader = 0;
		program = 0;
	}
	
	Shader::~Shader()
	{
		if(_vertexShader)
			glDeleteShader(_vertexShader);
		
		if(_fragmentShader)
			glDeleteShader(_fragmentShader);
		
		if(_geometryShader)
			glDeleteShader(_geometryShader);
		
		if(program)
			glDeleteProgram(program);
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
	
	
	
	void Shader::SetShaderDataForType(const std::string& data, GLenum type)
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
		
		const GLchar *source = (const GLchar *)data.c_str();
		GLuint shader = glCreateShader(type);
		
		Kernel::CheckOpenGLError("glCreateShader");
		
		if(shader == 0)
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
		
		switch(type)
		{
			case GL_VERTEX_SHADER:
				_vertexShader = shader;
				break;
				
			case GL_FRAGMENT_SHADER:
				_fragmentShader = shader;
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				_geometryShader = shader;
				break;
#endif
				
			default:
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
				break;
		}
	}
	
	void Shader::SetShaderForType(File *file, GLenum type)
	{
		SetShaderDataForType(file->String(), type);
	}
	
	void Shader::SetShaderForType(const std::string& path, GLenum type)
	{
		File *file = new File(path);
		SetShaderForType(file, type);
		file->Release();
	}
	
	void Shader::Link()
	{
		if(program)
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderAlreadyLinked);

		program = glCreateProgram();
		
		if(_vertexShader)
			glAttachShader(program, _vertexShader);
		
		if(_fragmentShader)
			glAttachShader(program, _fragmentShader);
		
		if(_geometryShader)
			glAttachShader(program, _geometryShader);
		
#if RN_PLATFORM_MAC_OS
		glBindFragDataLocation(program, 0, "fragColor0");
		glBindFragDataLocation(program, 1, "fragColor1");
#endif
		
		glLinkProgram(program);
		
		GLint status, length;
		
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if(status == GL_FALSE)
		{
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			
			GLchar *log = new GLchar[length];
			glGetProgramInfoLog(program, length, &length, log);
			
			std::string tlog = std::string((char *)log);
			delete [] log;

			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, tlog);
		}
		else
		{
			// Detach and remove the shader
			if(_vertexShader)
			{
				glDetachShader(program, _vertexShader);
				glDeleteShader(_vertexShader);
				
				_vertexShader = 0;
			}
			
			if(_fragmentShader)
			{
				glDetachShader(program, _fragmentShader);
				glDeleteShader(_fragmentShader);
				
				_fragmentShader = 0;
			}
			
			if(_geometryShader)
			{
				glDetachShader(program, _geometryShader);
				glDeleteShader(_geometryShader);
				
				_geometryShader = 0;
			}
			
			// Get uniforms
			matProj = glGetUniformLocation(program, "matProj");
			matProjInverse = glGetUniformLocation(program, "matProjInverse");
			
			matView = glGetUniformLocation(program, "matView");
			matViewInverse = glGetUniformLocation(program, "matViewInverse");
			
			matModel = glGetUniformLocation(program, "matModel");
			matModelInverse = glGetUniformLocation(program, "matModelInverse");
			
			matProjViewModel = glGetUniformLocation(program, "matProjViewModel");
			matProjViewModelInverse = glGetUniformLocation(program, "matProjViewModelInverse");
			
			targetmap = glGetUniformLocation(program, "targetmap");
			
			// Get attributes
			position  = glGetAttribLocation(program, "position");
			normal    = glGetAttribLocation(program, "normal");
			texcoord0 = glGetAttribLocation(program, "texcoord0");
			texcoord1 = glGetAttribLocation(program, "texcoord1");
			color0    = glGetAttribLocation(program, "color0");
			color1    = glGetAttribLocation(program, "color1");
			
			glFlush();
		}
	}
}
