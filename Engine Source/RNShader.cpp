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
		
		SetShaderForType(PathManager::PathForName(shader + ".vsh"), ShaderType::VertexShader);
		SetShaderForType(PathManager::PathForName(shader + ".fsh"), ShaderType::FragmentShader);
		
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
			SetShaderForType(path, ShaderType::GeometryShader);
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
		
		std::unordered_map<std::string, ShaderDefine> cleanedDefines;
		
		for(ShaderDefine& define : _defines)
		{
			cleanedDefines.insert(std::unordered_map<std::string, ShaderDefine>::value_type(define.name, define));
		}
		
		for(ShaderDefine& define : _temporaryDefines)
		{
			cleanedDefines.insert(std::unordered_map<std::string, ShaderDefine>::value_type(define.name, define));
		}
		
		
		for(auto i=cleanedDefines.begin(); i!=cleanedDefines.end(); i++)
		{
			std::string exploded = "#define " + i->second.name + " " + i->second.value + "\n";
			
			data.insert(index, exploded);
			index += exploded.length();
		}
		
		return data;
	}
	
	void Shader::CompileShader(ShaderType type, const std::string& file, GLuint *outShader)
	{
		std::string source;
		
		switch(type)
		{
			case ShaderType::VertexShader:
				_temporaryDefines.emplace_back(ShaderDefine("RN_VERTEX_SHADER", "1"));
				source = PreProcessedShaderSource(_vertexShader);
				_temporaryDefines.pop_back();
				break;
				
			case ShaderType::FragmentShader:
				_temporaryDefines.emplace_back(ShaderDefine("RN_FRAGMENT_SHADER", "1"));
				source = PreProcessedShaderSource(_fragmentShader);
				_temporaryDefines.pop_back();
				break;
				
			case ShaderType::GeometryShader:
				_temporaryDefines.emplace_back(ShaderDefine("RN_GEOMETRY_SHADER", "1"));
				source = PreProcessedShaderSource(_geometryShader);
				_temporaryDefines.pop_back();
				break;
				
			default:
				throw ErrorException(0);
		}
		
		const GLchar *data = source.c_str();
		GLuint shader = glCreateShader(GLTypeForShaderType(type));
		
		glShaderSource(shader, 1, &data, NULL);
		glCompileShader(shader);
		
		*outShader = shader;
		
		// Check the compilation status of the shader
		GLint status, length;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		
		if(!status)
		{
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			
			char *log = new char[length];
			glGetShaderInfoLog(shader, length, &length, (GLchar *)log);
			glDeleteShader(shader);
			
			*outShader = 0;

			std::string result(log);
			std::string tlog = "Failed to compile " + file;
			
			// Parse the error
			std::regex regex("ERROR: [0-9]{0,}:[0-9]{0,}: .*\n", std::regex_constants::ECMAScript | std::regex_constants::icase);
			
			if(std::regex_search(result, regex))
			{
				std::regex lineRegex("[0-9]{0,}:[0-9]{0,}:", std::regex_constants::ECMAScript | std::regex_constants::icase);
				std::string parsedError = "";
				
				for(auto i=std::sregex_iterator(result.begin(), result.end(), regex); i!=std::sregex_iterator(); i++)
				{
					std::string match = i->str();
					std::smatch lineMatch;
					
					std::regex_search(match, lineMatch, lineRegex);
					std::string lineString = lineMatch.str();
					
					bool skippedColumn = false;
					uint32 line = 0;
					
					for(size_t i=0; i<lineString.length()-1; i++)
					{
						if(!skippedColumn && lineString[i] == ':')
						{
							skippedColumn = true;
							continue;
						}
						
						if(skippedColumn)
						{
							std::string temp = lineString.substr(i, (lineString.length() - 1) - i);
							line = atoi(temp.c_str());
							
							break;
						}
					}
					
					
					DebugMarker marker = ResolveFileForLine(type, line);
					char buffer[32];
					
					sprintf(buffer, "%u", marker.line);
					parsedError += marker.file + " " + buffer + ", Error: " + match.substr(lineString.length() + 8) + "\n";
				}
				
				result = parsedError;
			}
			
			// Clean up and throw the exception
			delete [] log;
			throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderCompilingFailed, tlog, result);
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
		
		if(lookup.type & ShaderProgram::TypeFog)
			_temporaryDefines.emplace_back(ShaderDefine("RN_FOG", ""));
		
		if(lookup.type & ShaderProgram::TypeClipPlane)
			_temporaryDefines.emplace_back(ShaderDefine("RN_CLIPPLANE", ""));
		
		if(lookup.type & ShaderProgram::TypeGammaCorrection)
			_temporaryDefines.emplace_back(ShaderDefine("RN_GAMMA_CORRECTION", ""));
		
		_temporaryDefines.insert(_temporaryDefines.end(), lookup.defines.begin(), lookup.defines.end());
		
		// Compile all required shaders
		GLuint shader[3] = {0};
		
		if(_vertexShader.length() > 0)
		{
			CompileShader(ShaderType::VertexShader, _vertexFile, &shader[0]);
			glAttachShader(program->program, shader[0]);
		}
		
		if(_fragmentShader.length() > 0)
		{
			CompileShader(ShaderType::FragmentShader, _fragmentFile, &shader[1]);
			glAttachShader(program->program, shader[1]);
		}
		
		if(_geometryShader.length() > 0)
		{
			CompileShader(ShaderType::GeometryShader, _geometryFile, &shader[2]);
			glAttachShader(program->program, shader[2]);
		}
		
		// Link the program
		glLinkProgram(program->program);
		RN_CHECKOPENGL();
		
		for(int i=0; i<3; i++)
		{
			if(shader[i])
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
		for(machine_uint i=0; ; i++)
		{
			sprintf(string, "targetmap%i", (int)i);
			GLuint location = glGetUniformLocation(program->program, string);
			
			if(location == -1)
				break;
			
			program->targetmaplocations.push_back(location);
			
			sprintf(string, "targetmap%iInfo", (int)i);
			location = glGetUniformLocation(program->program, string);
			program->targetmapinfolocations.push_back(location);
		}
		
		for(machine_uint i=0; ; i++)
		{
			sprintf(string, "mTexture%i", (int)i);
			GLuint location = glGetUniformLocation(program->program, string);
			
			if(location == -1)
				break;
			
			program->texlocations.push_back(location);
			
			sprintf(string, "mTexture%iInfo", (int)i);
			location = glGetUniformLocation(program->program, string);
			program->texinfolocations.push_back(location);
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
				
				program->fraglocations.push_back(location);
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
	
	void Shader::IncludeShader(const std::string& name, IncludeMode mode, File *parent, PreProcessedFile& output)
	{
		File *includeFile = 0;
		
		try
		{
			std::string path = PathManager::PathForName(PathManager::Join(parent->Path(), name));
			includeFile = new File(path);
			includeFile->Autorelease();
		}
		catch(ErrorException e)
		{
			throw ErrorException(e.Error(), "Couldn't include file " + name, "Failed to pre-process " + parent->Name() + "." + parent->Extension());
		}
		
		output.fullpath = includeFile->FullPath();
		PreProcessFile(includeFile, output);
	}
	
	void Shader::PreProcessFile(File *file, PreProcessedFile& output)
	{
		std::string data = file->String();
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
						throw ErrorException(0);
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
					throw ErrorException(0);
				
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
		
		switch(type)
		{
			case ShaderType::VertexShader:
				_vertexShader = std::move(result.data);
				_vertexMarker = std::move(result.marker);
				_vertexFile   = file->Name() + "." + file->Extension();
				break;
				
			case ShaderType::FragmentShader:
				_fragmentShader = std::move(result.data);
				_fragmentMarker = std::move(result.marker);
				_fragmentFile   = file->Name() + "." + file->Extension();
				break;
				
			case ShaderType::GeometryShader:
				_geometryShader = std::move(result.data);
				_geometryMarker = std::move(result.marker);
				_geometryFile   = file->Name() + "." + file->Extension();
				break;
				
			default:
				throw ErrorException(kErrorGroupGraphics, 0, kGraphicsShaderTypeNotSupported);
				break;
		}
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
	
	GLenum Shader::GLTypeForShaderType(ShaderType type)
	{
		switch(type)
		{
			case ShaderType::VertexShader:
				return GL_VERTEX_SHADER;
				
			case ShaderType::FragmentShader:
				return GL_FRAGMENT_SHADER;
				
			case ShaderType::GeometryShader:
				return GL_GEOMETRY_SHADER;
				
			default:
				break;
		}
		
		throw ErrorException(0);
	}
	
	Shader::DebugMarker Shader::ResolveFileForLine(ShaderType type, uint32 line)
	{
		std::vector<DebugMarker>& markers = _vertexMarker;
		DebugMarker closestMarker = markers[0];
		
		switch(type)
		{
			case ShaderType::VertexShader:
				markers = _vertexMarker;
				break;
				
			case ShaderType::FragmentShader:
				markers = _fragmentMarker;
				break;
				
			case ShaderType::GeometryShader:
				markers = _geometryMarker;
				break;
				
			default:
				throw ErrorException(0);
				break;
		}
		
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
