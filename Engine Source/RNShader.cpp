//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNKernel.h"
#include "RNThreadPool.h"
#include "RNPathManager.h"
#include "RNScopeGuard.h"

namespace RN
{
	RNDeclareMeta(Shader)
	
	GLuint ShaderProgram::GetCustomLocation(const std::string& name)
	{
		auto iterator = _customLocations.find(name);
		if(iterator != _customLocations.end())
			return iterator->second;
		
		GLuint location = glGetUniformLocation(program, name.c_str());
		_customLocations.insert(std::unordered_map<std::string, GLuint>::value_type(name, location));
		
		return location;
	}
	
	void ShaderProgram::ReadLocations()
	{
#define GetUniformLocation(uniform) uniform = glGetUniformLocation(program, #uniform)
#define GetAttributeLocation(attribute) attribute = glGetAttribLocation(program, #attribute)
#define GetBlockLocation(block) block = glGetUniformBlockIndex(program, #block)
		
		// Get uniforms
		GetUniformLocation(matProj);
		GetUniformLocation(matProjInverse);
		
		GetUniformLocation(matView);
		GetUniformLocation(matViewInverse);
		
		GetUniformLocation(matModel);
		GetUniformLocation(matModelInverse);
		
		GetUniformLocation(matNormal);
		GetUniformLocation(matNormalInverse);
		
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
		
		GetUniformLocation(fogPlanes);
		GetUniformLocation(fogColor);
		GetUniformLocation(clipPlane);
		
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
		GetUniformLocation(hdrSettings);
		
		char string[32];
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "targetmap%i", (int)i);
			GLuint location = glGetUniformLocation(program, string);
			
			if(location == -1)
				break;
			
			targetmaplocations.push_back(location);
			
			sprintf(string, "targetmap%iInfo", static_cast<int>(i));
			location = glGetUniformLocation(program, string);
			targetmapinfolocations.push_back(location);
		}
		
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "mTexture%i", static_cast<int>(i));
			GLuint location = glGetUniformLocation(program, string);
			
			if(location == -1)
				break;
			
			texlocations.push_back(location);
			
			sprintf(string, "mTexture%iInfo", static_cast<int>(i));
			location = glGetUniformLocation(program, string);
			texinfolocations.push_back(location);
		}
		
		GetUniformLocation(depthmap);
		GetUniformLocation(depthmapinfo);
		
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
		
#if RN_TARGET_OPENGL
		do
		{
			GLint maxDrawbuffers;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawbuffers);
			
			for(GLint i = 0; i < maxDrawbuffers; i ++)
			{
				sprintf(string, "fragColor%i", static_cast<int>(i));
				GLint location = glGetFragDataLocation(program, string);
				
				if(location == -1)
					break;
				
				fraglocations.push_back(location);
			}
		} while(0);
#endif
		
#undef GetUniformLocation
#undef GetAttributeLocation
#undef GetBlockLocation
	}
	
	
	
	Shader::Shader()
	{
		_supportedPrograms = 0;
	}
	
	Shader::Shader(const std::string& shader)
	{
		_supportedPrograms = 0;
		
		SetShaderForType(PathManager::PathForName(shader + ".vsh"), ShaderType::VertexShader);
		SetShaderForType(PathManager::PathForName(shader + ".fsh"), ShaderType::FragmentShader);
		
#if GL_GEOMETRY_SHADER
		std::string path = "";
		
		try
		{
			path = PathManager::PathForName(shader + ".gsh");
		}
		catch(Exception)
		{}
		
		if(path.length() > 0)
			SetShaderForType(path, ShaderType::GeometryShader);
#endif
		
		GetProgramOfType(ShaderProgram::TypeNormal);
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
		for(auto i = _defines.begin(); i != _defines.end(); i ++)
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
	
	void Shader::DumpLinkStatusAndDie(ShaderProgram *program)
	{
		GLint length;
		GLchar *log;
		
		glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &length);
		
		log = new GLchar[length];
		
		glGetProgramInfoLog(program->program, length, &length, log);
		
		std::string tlog = std::string((char *)log);
		delete [] log;
		
		throw Exception(Exception::Type::ShaderLinkingFailedException, tlog);
	}
	
	ShaderProgram *Shader::CompileProgram(const ShaderLookup& lookup)
	{
		auto iterator = _programs.find(lookup);
		if(iterator != _programs.end())
		{
			ShaderProgram *program = iterator->second;
			return program;
		}

		ShaderProgram *program = new ShaderProgram;
		_programs[lookup] = program;
		
		program->program = glCreateProgram();
		
		ScopeGuard scopeGuard = ScopeGuard([&]() {
			glDeleteProgram(program->program);
			delete program;
			
			_programs.erase(lookup);
		});
		
		std::vector<ShaderDefine> temporaryDefines;
		
		// Prepare the state
		if(lookup.type & ShaderProgram::TypeInstanced)
			temporaryDefines.emplace_back(ShaderDefine("RN_INSTANCING", ""));
		
		if(lookup.type & ShaderProgram::TypeAnimated)
			temporaryDefines.emplace_back(ShaderDefine("RN_ANIMATION", ""));
		
		if(lookup.type & ShaderProgram::TypeLighting)
			temporaryDefines.emplace_back(ShaderDefine("RN_LIGHTING", ""));
		
		if(lookup.type & ShaderProgram::TypeDiscard)
			temporaryDefines.emplace_back(ShaderDefine("RN_DISCARD", ""));
		
		if(lookup.type & ShaderProgram::TypeDirectionalShadows)
			temporaryDefines.emplace_back(ShaderDefine("RN_DIRECTIONAL_SHADOWS", ""));
		
		if(lookup.type & ShaderProgram::TypeFog)
			temporaryDefines.emplace_back(ShaderDefine("RN_FOG", ""));
		
		if(lookup.type & ShaderProgram::TypeClipPlane)
			temporaryDefines.emplace_back(ShaderDefine("RN_CLIPPLANE", ""));
		
		if(lookup.type & ShaderProgram::TypeGammaCorrection)
			temporaryDefines.emplace_back(ShaderDefine("RN_GAMMA_CORRECTION", ""));
		
		temporaryDefines.insert(temporaryDefines.end(), lookup.defines.begin(), lookup.defines.end());
		
		// Compile all units
		std::vector<ShaderUnit *> units;
		
		for(auto i = _shaderData.begin(); i != _shaderData.end(); i ++)
		{
			ShaderUnit *unit = new ShaderUnit(this, i->first);
			unit->Compile(temporaryDefines);
			
			units.push_back(unit);
			glAttachShader(program->program, unit->GetShader());
		}
		
		// Link the program
		glLinkProgram(program->program);
		RN_CHECKOPENGL();
		
		for(auto i = units.begin(); i != units.end(); i ++)
		{
			ShaderUnit *unit = *i;
			glDetachShader(program->program, unit->GetShader());
			
			delete unit;
		}
		
		// Get the program link status
		GLint status;
		
		glGetProgramiv(program->program, GL_LINK_STATUS, &status);
		if(!status)
		{
			DumpLinkStatusAndDie(program);
			return nullptr;
		}
		
		// Dump the scope guard and clear all defines that were just visible in this compilation unit
		scopeGuard.Commit();
		program->ReadLocations();
		
		RN_CHECKOPENGL();
		glFlush();
		
		return program;
	}
	
	ShaderProgram *Shader::GetProgramWithLookup(const ShaderLookup& lookup)
	{
		if(!SupportsProgramOfType(lookup.type))
			return 0;
		
		return CompileProgram(lookup);
	}
	
	ShaderProgram *Shader::GetProgramOfType(uint32 type)
	{
		return GetProgramWithLookup(ShaderLookup(type));
	}
	
	bool Shader::SupportsProgramOfType(uint32 type)
	{
		return ((_supportedPrograms & type) == type || type == 0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Setter
	// ---------------------
	
	void Shader::IncludeShader(const std::string& name, IncludeMode mode, File *parent, PreProcessedFile& output)
	{
		File *includeFile = 0;
		
		try
		{
			std::string path;
			
			switch(mode)
			{
				case IncludeMode::CurrentDir:
					path = PathManager::PathForName(PathManager::Join(parent->GetPath(), name));
					break;
					
				case IncludeMode::IncludeDir:
					path = PathManager::PathForName(name);
					break;
			}
			
			includeFile = new File(path);
			includeFile->Autorelease();
		}
		catch(Exception e)
		{
			throw Exception(e.GetType(), "Couldn't include file " + name);
		}
		
		output.fullpath = includeFile->GetFullPath();
		PreProcessFile(includeFile, output);
	}
	
	void Shader::PreProcessFile(File *file, PreProcessedFile& output)
	{
		std::string data = file->GetString();
		size_t index = -1;
		
		uint32 lines = output.offset;
		uint32 offset = 0;
		
		bool atEnd = false;
		output.marker.push_back(DebugMarker(lines, offset, file));
		
		// Scan the file line by line...
		do
		{
			index ++;
			
			if(atEnd)
			{
				lines ++;
				offset ++;
				continue;
			}
			
			size_t next = data.find("\n", index);
			size_t include = data.find("#include", index);
			
			if(include == std::string::npos)
			{
				lines ++;
				offset ++;
				atEnd = true;
				
				continue;
			}
			else
			if(include < next)
			{
				size_t begin = include + 8;
				while(data[begin] == ' ')
					begin ++;
				
				IncludeMode mode;
				switch(data[begin])
				{
					case '"':
						mode = IncludeMode::CurrentDir;
						begin ++;
						break;
						
					case '<':
						mode = IncludeMode::IncludeDir;
						begin ++;
						break;
						
					default:
						throw Exception(Exception::Type::GenericException, "");
						break;
				}
				
				size_t end = 0;
				switch(mode)
				{
					case IncludeMode::CurrentDir:
						end = data.find("\"", begin);
						break;
						
					case IncludeMode::IncludeDir:
						end = data.find(">", begin);
						break;
				}
				
				if(end == std::string::npos)
					throw Exception(Exception::Type::GenericException, "");
				
				// Include the file
				std::string name = data.substr(begin, end - begin);
				data.erase(include, (end + 1) - include);
				
				PreProcessedFile result;
				result.offset = lines;
				
				IncludeShader(name, mode, file, result);
				
				data.insert(include, result.data);
				output.marker.insert(output.marker.end(), result.marker.begin(), result.marker.end());
				
				lines  += result.lines;
				index  += result.data.length();
				
				output.marker.emplace_back(DebugMarker(lines, offset, file));
			}
			
			lines ++;
			offset ++;
		} while((index = data.find("\n", index)) != std::string::npos);
		
		output.data = std::move(data);
		output.lines = (lines - 1) - output.offset;
	}
	
	bool Shader::IsDefined(const std::string& source, const std::string& define)
	{
		return (source.find("#ifdef " + define) != std::string::npos || source.find("defined(" + define + ")") != std::string::npos);
	}
	
	void Shader::SetShaderForType(File *file, ShaderType type)
	{
		// Preprocess the shader
		PreProcessedFile result;
		result.offset = 0;
		
		PreProcessFile(file, result);
		
		// Check what program types the shader supports
		_supportedPrograms |= IsDefined(result.data, "RN_INSTANCING") ? ShaderProgram::TypeInstanced : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_ANIMATION") ? ShaderProgram::TypeAnimated : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_LIGHTING") ? ShaderProgram::TypeLighting : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_DISCARD") ? ShaderProgram::TypeDiscard : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_DIRECTIONAL_SHADOWS") ? ShaderProgram::TypeDirectionalShadows : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_FOG") ? ShaderProgram::TypeFog : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_CLIPPLANE") ? ShaderProgram::TypeClipPlane : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_GAMMA_CORRECTION") ? ShaderProgram::TypeGammaCorrection : 0;
		
		ShaderData data;
		data.shader = std::move(result.data);
		data.marker = std::move(result.marker);
		data.file   = file->GetName() + "." + file->GetExtension();
		
		_shaderData.insert(std::map<ShaderType, ShaderData>::value_type(type, std::move(data)));
	}
	
	void Shader::SetShaderForType(const std::string& path, ShaderType type)
	{
		File *file = new File(path);
		SetShaderForType(file, type);
		file->Release();
	}
	
	// ---------------------
	// MARK: -
	// MARK: helper
	// ---------------------
	
	const std::string& Shader::GetShaderSource(ShaderType type)
	{
		return _shaderData[type].shader;
	}
	
	Shader::DebugMarker Shader::ResolveFileForLine(ShaderType type, uint32 line)
	{
		std::vector<DebugMarker>& markers = _shaderData[type].marker;
		DebugMarker closestMarker = markers[0];
		
		for(const DebugMarker& marker : markers)
		{
			if(marker.line > line)
				break;
			
			closestMarker = marker;
		}
		
		uint32 rline = (line - closestMarker.line) + closestMarker.offset;
		return DebugMarker(rline, 0, closestMarker.file);
	}
}
