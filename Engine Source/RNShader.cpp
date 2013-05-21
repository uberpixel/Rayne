//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNKernel.h"
#include "RNPathManager.h"
#include "RNScopeGuard.h"

namespace RN
{
	RNDeclareMeta(Shader)
	
	Shader::Shader()
	{
		_supportedPrograms = 0;
		AddDefines();
	}
	
	Shader::Shader(const std::string& shader)
	{
		_supportedPrograms = 0;
		AddDefines();
		
		SetVertexShader(PathManager::PathForName(shader + ".vsh"));
		SetFragmentShader(PathManager::PathForName(shader + ".fsh"));
		
#if GL_GEOMETRY_SHADER
		std::string path = "";
		
		try
		{
			path = PathManager::PathForName(shader + ".gsh");
		}
		catch(ErrorException e)
		{
		}
		
		if(path.length() > 0)
			SetGeometryShader(path);
#endif
		
		ProgramOfType(ShaderProgram::TypeNormal);
	}
	
	Shader::~Shader()
	{
		for(auto i=_programs.begin(); i!=_programs.end(); i++)
		{
			ShaderProgram *program = i->second;
			
			glDeleteProgram(program->program);
			delete program;
		}
	}
	
	Shader *Shader::WithFile(const std::string& file)
	{
		Shader *shader = new Shader(file);
		return shader->Autorelease();
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Defines
	// ---------------------
	
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
		_defines.emplace_back(ShaderDefine(define, ""));
	}
	
	void Shader::Define(const std::string& define, const std::string& value)
	{
		_defines.emplace_back(ShaderDefine(define, value));
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
		
		_defines.emplace_back(ShaderDefine(define, buffer));
	}	
	
	void Shader::Undefine(const std::string& name)
	{
		for(auto i=_defines.begin(); i!=_defines.end(); i++)
		{
			if(name == i->name)
			{
				_defines.erase(i);
				return;
			}
		}
	}
	
	// ---------------------
	// MARK: -
	// MARK: Program creation
	// ---------------------
	
	std::string Shader::PreProcessedShaderSource(const std::string& source)
	{
		std::string data = source;
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
		
		for(auto i=_defines.begin(); i!=_defines.end(); i++)
		{
			std::string exploded = "#define " + i->name + " " + i->value + "\n";
			
			data.insert(index, exploded);
			index += exploded.length();
		}
		
		for(auto i=_temporaryDefines.begin(); i!=_temporaryDefines.end(); i++)
		{
			std::string exploded = "#define " + i->name + " " + i->value + "\n";
			
			data.insert(index, exploded);
			index += exploded.length();
		}
		
		return data;
	}
	
	void Shader::CompileShader(GLenum type, GLuint *outShader)
	{
		std::string source;
		
		switch(type)
		{
			case GL_VERTEX_SHADER:
				_temporaryDefines.emplace_back(ShaderDefine("RN_VERTEX_SHADER", "1"));
				source = PreProcessedShaderSource(_vertexShader);
				_temporaryDefines.pop_back();
				break;
				
			case GL_FRAGMENT_SHADER:
				_temporaryDefines.emplace_back(ShaderDefine("RN_FRAGMENT_SHADER", "1"));
				source = PreProcessedShaderSource(_fragmentShader);
				_temporaryDefines.pop_back();
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				_temporaryDefines.emplace_back(ShaderDefine("RN_GEOMETRY_SHADER", "1"));
				source = PreProcessedShaderSource(_geometryShader);
				_temporaryDefines.pop_back();
				break;
#endif
		}
		
		const GLchar *data = source.c_str();
		GLuint shader = glCreateShader(type);
		
		glShaderSource(shader, 1, &data, NULL);
		glCompileShader(shader);
		
		*outShader = shader;
		
		// Check the compilation status of the shader
		GLint status, length;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		
		if(!status)
		{
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			
			GLchar *log = new GLchar[length];
			glGetShaderInfoLog(shader, length, &length, log);
			glDeleteShader(shader);
			
			*outShader = 0;
			
			std::string compilerLog = std::string((char *)log) + "\nShader source:\n" + source;
			delete [] log;
			
			std::string tlog;
			
			switch(type)
			{
				case GL_VERTEX_SHADER:
					tlog = "Failed to compile " + _vertexFile + ".\n";
					break;
					
				case GL_FRAGMENT_SHADER:
					tlog = "Failed to compile " + _fragmentFile + ".\n";
					break;
					
#ifdef GL_GEOMETRY_SHADER
				case GL_GEOMETRY_SHADER:
					tlog = "Failed to compile " + _geometryFile + ".\n";
					break;
#endif
			}
			
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, tlog, compilerLog);
		}
	}
	
	void Shader::DumpLinkStatusAndDie(ShaderProgram *program)
	{
		GLint length;
		GLchar *log;
		
		glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &length);
		
		log = new GLchar[length];
		
		glGetProgramInfoLog(program->program, length, &length, log);
		
		std::string tlog = std::string((char *)log);
		delete [] log;
		
		throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderLinkingFailed, tlog);
	}
	
	ShaderProgram *Shader::ProgramWithLookup(const ShaderLookup& lookup)
	{
		if(!SupportsProgramOfType(lookup.type))
			return 0;
		
		auto iterator = _programs.find(lookup);
		if(iterator != _programs.end())
			return iterator->second;
		
		ShaderProgram *program = new ShaderProgram;
		program->program = glCreateProgram();
		
		if(program->program == 0)
		{
			delete program;
			
			RN_CHECKOPENGL();				
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderLinkingFailed, "Couldn't create program");
		}
		
		_programs[lookup] = program;
		
		ScopeGuard scopeGuard = ScopeGuard([&]() {
			glDeleteProgram(program->program);
			delete program;
			
			_programs.erase(lookup);
			_temporaryDefines.clear();
		});
		
		
		// Prepare the state
		if(lookup.type & ShaderProgram::TypeInstanced)
			_temporaryDefines.emplace_back(ShaderDefine("RN_INSTANCING", ""));
		
		if(lookup.type & ShaderProgram::TypeAnimated)
			_temporaryDefines.emplace_back(ShaderDefine("RN_ANIMATION", ""));
		
		if(lookup.type & ShaderProgram::TypeLighting)
			_temporaryDefines.emplace_back(ShaderDefine("RN_LIGHTING", ""));
		
		if(lookup.type & ShaderProgram::TypeDiscard)
			_temporaryDefines.emplace_back(ShaderDefine("RN_DISCARD", ""));
		
		if(lookup.type & ShaderProgram::TypeDirectionalShadows)
			_temporaryDefines.emplace_back(ShaderDefine("RN_DIRECTIONAL_SHADOWS", ""));
		
		_temporaryDefines.insert(_temporaryDefines.end(), lookup.defines.begin(), lookup.defines.end());
		
		// Compile all required shaders
		GLuint shader[3] = {0};
		
		if(_vertexShader.length() > 0)
		{
			CompileShader(GL_VERTEX_SHADER, &shader[0]);
			glAttachShader(program->program, shader[0]);
		}
		
		if(_fragmentShader.length() > 0)
		{
			CompileShader(GL_FRAGMENT_SHADER, &shader[1]);
			glAttachShader(program->program, shader[1]);
		}
		
#ifdef GL_GEOMETRY_SHADER
		if(_geometryShader.length() > 0)
		{
			CompileShader(GL_GEOMETRY_SHADER, &shader[2]);
			glAttachShader(program->program, shader[2]);
		}
#endif
		
		// Link the program
		glLinkProgram(program->program);
		RN_CHECKOPENGL();
		
		for(int i=0; i<3; i++)
		{
			if(shader[i] != 0)
			{
				glDetachShader(program->program, shader[i]);
				glDeleteShader(shader[i]);
			}
		}
		
		// Get the program link status
		GLint status;
		
		glGetProgramiv(program->program, GL_LINK_STATUS, &status);
		if(!status)
		{
			DumpLinkStatusAndDie(program);
			return 0;
		}
		
		// Dump the scope guard and clear all defines that were just visible in this compilation unit
		scopeGuard.Commit();
		_temporaryDefines.clear();
		
		// Get Shader data
#define GetUniformLocation(uniform) program->uniform = glGetUniformLocation(program->program, #uniform)
#define GetAttributeLocation(attribute) program->attribute = glGetAttribLocation(program->program, #attribute)
#define GetBlockLocation(block) program->block = glGetUniformBlockIndex(program->program, #block)
		
		// Get uniforms
		GetUniformLocation(matProj);
		GetUniformLocation(matProjInverse);
		
		GetUniformLocation(matView);
		GetUniformLocation(matViewInverse);
		
		GetUniformLocation(matModel);
		GetUniformLocation(matModelInverse);
		
		GetUniformLocation(matViewModel);
		GetUniformLocation(matViewModelInverse);
		
		GetUniformLocation(matProjView);
		GetUniformLocation(matProjViewInverse);
		
		GetUniformLocation(matProjViewModel);
		GetUniformLocation(matProjViewModelInverse);
		
		GetUniformLocation(matBones);
		GetUniformLocation(instancingData);
		
		GetUniformLocation(time);
		GetUniformLocation(frameSize);
		GetUniformLocation(clipPlanes);
		GetUniformLocation(discardThreshold);
		
		GetUniformLocation(ambient);
		GetUniformLocation(diffuse);
		GetUniformLocation(specular);
		GetUniformLocation(emissive);
		GetUniformLocation(shininess);
		
		GetUniformLocation(viewPosition);
		
		GetUniformLocation(lightPointCount);
		GetUniformLocation(lightPointPosition);
		GetUniformLocation(lightPointColor);
		GetUniformLocation(lightPointList);
		GetUniformLocation(lightPointListOffset);
		GetUniformLocation(lightPointListData);
		
		GetUniformLocation(lightSpotCount);
		GetUniformLocation(lightSpotPosition);
		GetUniformLocation(lightSpotColor);
		GetUniformLocation(lightSpotDirection);
		GetUniformLocation(lightSpotList);
		GetUniformLocation(lightSpotListOffset);
		GetUniformLocation(lightSpotListData);
		
		GetUniformLocation(lightDirectionalDirection);
		GetUniformLocation(lightDirectionalColor);
		GetUniformLocation(lightDirectionalCount);
		GetUniformLocation(lightDirectionalMatrix);
		GetUniformLocation(lightDirectionalDepth);
		
		GetUniformLocation(lightTileSize);
		
		char string[32];
		for(machine_uint i=0; ; i++)
		{
			sprintf(string, "targetmap%i", (int)i);
			GLuint location = glGetUniformLocation(program->program, string);
			
			if(location == -1)
				break;
			
			program->targetmaplocations.AddObject(location);
		}
		
		for(machine_uint i=0; ; i++)
		{
			sprintf(string, "mTexture%i", (int)i);
			GLuint location = glGetUniformLocation(program->program, string);
			
			if(location == -1)
				break;
			
			program->texlocations.AddObject(location);
		}
		
		GetUniformLocation(depthmap);
		
		// Get attributes
		GetAttributeLocation(attPosition);
		GetAttributeLocation(attNormal);
		GetAttributeLocation(attTangent);
		
		GetAttributeLocation(attTexcoord0);
		GetAttributeLocation(attTexcoord1);
		
		GetAttributeLocation(attColor0);
		GetAttributeLocation(attColor1);
		
		GetAttributeLocation(attBoneWeights);
		GetAttributeLocation(attBoneIndices);
		
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		do
		{
			GLint maxDrawbuffers;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawbuffers);
			
			for(GLint i=0; i<maxDrawbuffers; i++)
			{
				sprintf(string, "fragColor%i", i);
				GLint location = glGetFragDataLocation(program->program, string);
				
				if(location == -1)
					break;
				
				program->fraglocations.AddObject(location);
			}
		} while(0);
#endif
		
#undef GetUniformLocation
#undef GetAttributeLocation
#undef GetBlockLocation
		
		RN_CHECKOPENGL();
		glFlush();
		
		return program;
	}
	
	ShaderProgram *Shader::ProgramOfType(uint32 type)
	{
		return ProgramWithLookup(ShaderLookup(type));
	}
	
	bool Shader::SupportsProgramOfType(uint32 type)
	{
		return (_supportedPrograms & type || type == 0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Setter
	// ---------------------
	
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
	
	std::string Shader::IncludeShader(File *source, const std::string& name)
	{
		File *includeFile = 0;
		
		try
		{
			std::string path = PathManager::PathForName(PathManager::Join(source->Path(), name));
			includeFile = new File(path);
			includeFile->Autorelease();
		}
		catch(ErrorException e)
		{
			throw ErrorException(e.Error(), "Couldn't include file " + name, "Failed to pre-process " + source->Name() + "." + source->Extension());
		}
		
		
		return PreProcessFile(includeFile);
	}
	
	std::string Shader::PreProcessFile(File *file)
	{
		std::string data = file->String();
		size_t index = 0;
		
		// Search for includes files
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
			data.insert(index, IncludeShader(file, name));
		}
		
		return data;
	}
	
	void Shader::SetShaderForType(File *file, GLenum type)
	{		
		// Preprocess the shader
		std::string data = PreProcessFile(file);
		
		// Check what program types the shader supports
		_supportedPrograms |= (data.find("#ifdef RN_INSTANCING") != std::string::npos) ? (ShaderProgram::TypeInstanced) : 0;
		_supportedPrograms |= (data.find("#ifdef RN_ANIMATION") != std::string::npos) ? (ShaderProgram::TypeAnimated) : 0;
		_supportedPrograms |= (data.find("#ifdef RN_LIGHTING") != std::string::npos) ? (ShaderProgram::TypeLighting) : 0;
		_supportedPrograms |= (data.find("#ifdef RN_DISCARD") != std::string::npos) ? (ShaderProgram::TypeDiscard) : 0;
		_supportedPrograms |= (data.find("#ifdef RN_DIRECTIONAL_SHADOWS") != std::string::npos) ? (ShaderProgram::TypeDirectionalShadows) : 0;
		
		switch(type)
		{
			case GL_VERTEX_SHADER:
				_vertexShader = data;
				_vertexFile   = file->Name() + "." + file->Extension();
				break;
				
			case GL_FRAGMENT_SHADER:
				_fragmentShader = data;
				_fragmentFile   = file->Name() + "." + file->Extension();
				break;
				
#ifdef GL_GEOMETRY_SHADER
			case GL_GEOMETRY_SHADER:
				_geometryShader = data;
				_geometryFile   = file->Name() + "." + file->Extension();
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
}
