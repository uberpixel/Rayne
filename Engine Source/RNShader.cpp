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
		
		AddDefines();
	}
	
	Shader::Shader(const std::string& shader, bool link)
	{
		_vertexShader = _fragmentShader = _geometryShader = 0;
		program = 0;
		
		AddDefines();
		
		{
			std::string path = File::PathForName(shader + ".vsh");
			SetVertexShader(path);
		}
		
		{
			std::string path = File::PathForName(shader + ".fsh");
			SetFragmentShader(path);
		}
		
		try
		{
			std::string path = File::PathForName(shader + ".gsh");
			SetGeometryShader(path);
		}
		catch(ErrorException e)
		{
		}
		
		if(link)
			Link();
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
	
	Shader *Shader::WithFile(const std::string& file, bool link)
	{
		Shader *shader = new Shader(file, link);
		return shader->Autorelease<Shader>();
	}
	
	void Shader::AddDefines()
	{
#ifdef DEBUG
		Define("DEBUG", 1);
#endif
#ifdef NDEBUG
		Define("NDEBUG", 1);
#endif
		
#if RN_TARGET_OPENGL
		Define("OPENGL", 1);
#endif
#if RN_TARGET_OPENGL_ES
		Define("OPENGLES", 1);
#endif
	}
	
	
	void Shader::Define(const std::string& define)
	{
		ShaderDefine def;
		def.name = define;
		def.value = "";
		
		_defines.AddObject(def);
	}
	
	void Shader::Define(const std::string& define, const std::string& value)
	{
		ShaderDefine def;
		def.name = define;
		def.value = value;
		
		_defines.AddObject(def);
	}
	
	void Shader::Define(const std::string& define, int32 value)
	{
		char buffer[32];
		sprintf(buffer, "%i", value);
		
		Define(define, buffer);
	}
	
	void Shader::Define(const std::string& define, float value)
	{
		char buffer[32];
		sprintf(buffer, "%f", value);
		
		Define(define, buffer);
	}	
	
	void Shader::Undefine(const std::string& name)
	{
		for(machine_uint i=0; i<_defines.Count(); i++)
		{
			ShaderDefine& define = _defines[(int)i];
			if(define.name == name)
			{
				_defines.RemoveObjectAtIndex(i);
				return;
			}
		}
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
	
	
	
	void Shader::SetShaderForType(File *file, GLenum type)
	{
		switch(type)
		{
			case GL_VERTEX_SHADER:
				if(_vertexShader)
				{
					glDeleteShader(_vertexShader);
					_vertexShader = 0;
				}
				
				Define("VERTEXSHADER", 1);
				break;
				
			case GL_FRAGMENT_SHADER:
				if(_fragmentShader)
				{
					glDeleteShader(_fragmentShader);
					_fragmentShader = 0;
				}
				
				Define("FRAGMENTSHADER", 1);
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				if(_geometryShader)
				{
					glDeleteShader(_geometryShader);
					_geometryShader = 0;
				}
				
				Define("GEOMETRYSHADER", 1);
				break;
#endif
				
			default:
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
				break;
		}
		
		// Preprocess the shader
		std::string data = file->String();
		size_t index = data.find("#version");
		
		if(index != std::string::npos)
		{
			index += 8;
			
			do {
				index ++;
			} while(data[index] != '\n');
			
			index ++;
		}
		else
		{
			index = 0;
		}
		
		for(machine_uint i=0; i<_defines.Count(); i++)
		{
			ShaderDefine& define = _defines[(int)i];
			std::string exploded = "#define " + define.name + " " + define.value + "\n";
			
			data.insert(index, exploded);
			index += exploded.length();
		}
		
		index = 0;
		while((index = data.find("#include \"", index)) != std::string::npos)
		{
			std::string::iterator iterator = data.begin() + (index + 10);
			size_t length = 0;
			while(*iterator != '"')
			{
				length ++;
				iterator ++;
			}
			
			std::string name = data.substr(index + 10, length);
			data.erase(index, length + 11);
			
			File *includeFile = new File(file->Path() + "/" + name);
			data.insert(index, includeFile->String());
			includeFile->Release();
		}
		
		// Compile everything
		const GLchar *source = (const GLchar *)data.c_str();
		GLuint shader = glCreateShader(type);
		
		RN_CHECKOPENGL();
		
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
			
			std::string compilerLog = "Failed to compile " + file->Name() + file->Extension() + ".\n" + tlog;
			
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, compilerLog);
		}
		
		switch(type)
		{
			case GL_VERTEX_SHADER:
				_vertexShader = shader;
				Undefine("VERTEXSHADER");
				break;
				
			case GL_FRAGMENT_SHADER:
				_fragmentShader = shader;
				Undefine("FRAGMENTSHADER");
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				_geometryShader = shader;
				Undefine("GEOMETRYSHADER");
				break;
#endif
				
			default:
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
				break;
		}
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
		
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		do
		{
			GLint maxDrawbuffers;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawbuffers);
			
			for(GLint i=0; i<maxDrawbuffers; i++)
			{
				char buffer[32];
				sprintf(buffer, "fragColor%i", i);
				
				glBindFragDataLocation(program, i, buffer);
			}
		} while(0);
#endif
		
		glLinkProgram(program);
		RN_CHECKOPENGL();
		
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
		
		// Get the linking status
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
			
#define GetUniformLocation(uniform) uniform = glGetUniformLocation(program, #uniform)
#define GetAttributeLocation(attribute) attribute = glGetAttribLocation(program, #attribute)
			
			// Get uniforms
			GetUniformLocation(matProj);
			GetUniformLocation(matProjInverse);
			
			GetUniformLocation(matView);
			GetUniformLocation(matViewInverse);
			
			GetUniformLocation(matModel);
			GetUniformLocation(matModelInverse);
			
			GetUniformLocation(matViewModel);
			GetUniformLocation(matViewModelInverse);
			
			GetUniformLocation(matProjViewModel);
			GetUniformLocation(matProjViewModelInverse);
			
			GetUniformLocation(matBones);
			
			GetUniformLocation(time);
			GetUniformLocation(frameSize);
			GetUniformLocation(clipPlanes);
			
			GetUniformLocation(lightPosition);
			GetUniformLocation(lightColor);
			GetUniformLocation(lightCount);
			GetUniformLocation(lightList);
			GetUniformLocation(lightListOffset);
			GetUniformLocation(lightListPosition);
			GetUniformLocation(lightListColor);
			GetUniformLocation(lightTileSize);
			
			char string[32];
			for(machine_uint i=0; ; i++)
			{
				sprintf(string, "targetmap%i", (int)i);
				GLuint location = glGetUniformLocation(program, string);
				
				if(location == -1)
					break;
				
				targetmaplocations.AddObject(location);
			}
			
			for(machine_uint i=0; ; i++)
			{
				sprintf(string, "mTexture%i", (int)i);
				GLuint location = glGetUniformLocation(program, string);
				
				if(location == -1)
					break;
				
				texlocations.AddObject(location);
			}
			
			GetUniformLocation(depthmap);
			
			// Get attributes
			GetAttributeLocation(imatModel);
			
			GetAttributeLocation(vertPosition);
			GetAttributeLocation(vertNormal);
			GetAttributeLocation(vertTangent);
			
			GetAttributeLocation(vertTexcoord0);
			GetAttributeLocation(vertTexcoord1);
			
			GetAttributeLocation(vertColor0);
			GetAttributeLocation(vertColor1);
			GetAttributeLocation(vertBoneWeights);
			GetAttributeLocation(vertBoneIndices);
			
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
			do
			{
				GLint maxDrawbuffers;
				glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawbuffers);
				
				for(GLint i=0; i<maxDrawbuffers; i++)
				{
					char buffer[32];
					sprintf(buffer, "fragColor%i", i);
					
					GLint location = glGetFragDataLocation(program, buffer);
					
					if(location == -1)
						break;
					
					fraglocations.AddObject(location);
				}
			} while(0);
#endif
			
#undef GetUniformLocation
#undef GetAttributeLocation
			
			RN_CHECKOPENGL();
			glFlush();
		}
	}
	
	bool Shader::IsLinked() const
	{
		return (program != 0);
	}
}
