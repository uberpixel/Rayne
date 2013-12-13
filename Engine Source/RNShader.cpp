//
//  RNShader.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShader.h"
#include "RNShaderCache.h"
#include "RNKernel.h"
#include "RNThreadPool.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNScopeGuard.h"
#include "RNSHA2.h"

namespace RN
{
	RNDeclareMeta(Shader)
	
	GLuint ShaderProgram::GetCustomLocation(const std::string& name)
	{
		auto iterator = _customLocations.find(name);
		if(iterator != _customLocations.end())
			return iterator->second;
		
		GLuint location = gl::GetUniformLocation(program, name.c_str());
		_customLocations.insert(std::unordered_map<std::string, GLuint>::value_type(name, location));
		
		return location;
	}
	
	void ShaderProgram::ReadLocations()
	{
#define __GetUniformLocation(uniform) uniform = gl::GetUniformLocation(program, #uniform)
#define __GetAttributeLocation(attribute) attribute = gl::GetAttribLocation(program, #attribute)
#define __GetBlockLocation(block) block = gl::GetUniformBlockIndex(program, #block)
		
		// Get uniforms
		__GetUniformLocation(matProj);
		__GetUniformLocation(matProjInverse);
		
		__GetUniformLocation(matView);
		__GetUniformLocation(matViewInverse);
		
		__GetUniformLocation(matModel);
		__GetUniformLocation(matModelInverse);
		
		__GetUniformLocation(matNormal);
		__GetUniformLocation(matNormalInverse);
		
		__GetUniformLocation(matViewModel);
		__GetUniformLocation(matViewModelInverse);
		
		__GetUniformLocation(matProjView);
		__GetUniformLocation(matProjViewInverse);
		
		__GetUniformLocation(matProjViewModel);
		__GetUniformLocation(matProjViewModelInverse);
		
		__GetUniformLocation(matBones);
		__GetUniformLocation(instancingData);
		__GetUniformLocation(instancingIndices);
		
		__GetUniformLocation(time);
		__GetUniformLocation(frameSize);
		__GetUniformLocation(clipPlanes);
		__GetUniformLocation(discardThreshold);
		
		__GetUniformLocation(fogPlanes);
		__GetUniformLocation(fogColor);
		__GetUniformLocation(clipPlane);
		__GetUniformLocation(cameraAmbient);
		
		__GetUniformLocation(ambient);
		__GetUniformLocation(diffuse);
		__GetUniformLocation(specular);
		__GetUniformLocation(emissive);
		
		__GetUniformLocation(viewPosition);
		__GetUniformLocation(viewNormal);
		
		__GetUniformLocation(lightPointCount);
		__GetUniformLocation(lightPointPosition);
		__GetUniformLocation(lightPointColor);
		
		__GetUniformLocation(lightSpotCount);
		__GetUniformLocation(lightSpotPosition);
		__GetUniformLocation(lightSpotColor);
		__GetUniformLocation(lightSpotDirection);
		__GetUniformLocation(lightSpotMatrix);
		
		__GetUniformLocation(lightDirectionalDirection);
		__GetUniformLocation(lightDirectionalColor);
		__GetUniformLocation(lightDirectionalCount);
		__GetUniformLocation(lightDirectionalMatrix);
		__GetUniformLocation(lightDirectionalDepth);
		
		__GetUniformLocation(lightListIndices);
		__GetUniformLocation(lightListOffsetCount);
		__GetUniformLocation(lightListDataPoint);
		__GetUniformLocation(lightListDataSpot);
		
		__GetUniformLocation(lightClusterSize);
		__GetUniformLocation(hdrSettings);
		
		char string[32];
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "targetmap%i", (int)i);
			GLuint location = gl::GetUniformLocation(program, string);
			
			if(location == -1)
				break;
			
			targetmaplocations.push_back(location);
			
			sprintf(string, "targetmap%iInfo", static_cast<int>(i));
			location = gl::GetUniformLocation(program, string);
			targetmapinfolocations.push_back(location);
		}
		
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "mTexture%i", static_cast<int>(i));
			GLuint location = gl::GetUniformLocation(program, string);
			
			if(location == -1)
				break;
			
			texlocations.push_back(location);
			
			sprintf(string, "mTexture%iInfo", static_cast<int>(i));
			location = gl::GetUniformLocation(program, string);
			texinfolocations.push_back(location);
		}
		
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "lightPointDepth%i", static_cast<int>(i));
			GLuint location = gl::GetUniformLocation(program, string);
			
			if(location == -1)
				break;
			
			lightPointDepthLocations.push_back(location);
		}
		
		for(size_t i = 0; i < 32; i ++)
		{
			sprintf(string, "lightSpotDepth%i", static_cast<int>(i));
			GLuint location = gl::GetUniformLocation(program, string);
			
			if(location == -1)
			break;
			
			lightSpotDepthLocations.push_back(location);
		}
		
		__GetUniformLocation(depthmap);
		__GetUniformLocation(depthmapinfo);
		
		// Get attributes
		__GetAttributeLocation(attPosition);
		__GetAttributeLocation(attNormal);
		__GetAttributeLocation(attTangent);
		
		__GetAttributeLocation(attTexcoord0);
		__GetAttributeLocation(attTexcoord1);
		
		__GetAttributeLocation(attColor0);
		__GetAttributeLocation(attColor1);
		
		__GetAttributeLocation(attBoneWeights);
		__GetAttributeLocation(attBoneIndices);
		
#if RN_TARGET_OPENGL
		do
		{
			GLint maxDrawbuffers;
			gl::GetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawbuffers);
			
			for(GLint i = 0; i < maxDrawbuffers; i ++)
			{
				sprintf(string, "fragColor%i", static_cast<int>(i));
				GLint location = gl::GetFragDataLocation(program, string);
				
				if(location == -1)
					break;
				
				fraglocations.push_back(location);
			}
		} while(0);
#endif
		
#undef __GetUniformLocation
#undef __GetAttributeLocation
#undef __GetBlockLocation
	}
	
	
	
	Shader::Shader()
	{
		_supportedPrograms = 0;
	}
	
	Shader::Shader(const std::string& shader)
	{
		_supportedPrograms = 0;
		
		SetShaderForType(FileManager::GetSharedInstance()->GetFilePathWithName(shader + ".vsh"), ShaderType::VertexShader);
		SetShaderForType(FileManager::GetSharedInstance()->GetFilePathWithName(shader + ".fsh"), ShaderType::FragmentShader);
		
#if GL_GEOMETRY_SHADER
		std::string path = "";
		
		try
		{
			path = FileManager::GetSharedInstance()->GetFilePathWithName(shader + ".gsh");
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
			
			gl::DeleteProgram(program->program);
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
		std::stringstream stream;
		stream << value;
		
		Define(define, stream.str());
	}
	
	void Shader::Define(const std::string& define, float value)
	{
		std::stringstream stream;
		stream << value;
		
		Define(define, stream.str());
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
		
		gl::GetProgramiv(program->program, GL_INFO_LOG_LENGTH, &length);
		
		log = new GLchar[length];
		
		gl::GetProgramInfoLog(program->program, length, &length, log);
		
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

		ShaderProgram *program = ShaderCache::GetSharedInstance()->DequeShaderProgram(this, lookup);
		if(program)
		{
			_programs[lookup] = program;
			
			gl::Flush();
			return program;
		}
		
		
		program = new ShaderProgram;
		program->program = gl::CreateProgram();
		
		_programs[lookup] = program;
		
#if GL_ARB_get_program_binary
		if(gl::SupportsFeature(gl::Feature::ShaderBinary))
			gl::ProgramParameteri(program->program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
#endif
		
		ScopeGuard scopeGuard = ScopeGuard([&]() {
			gl::DeleteProgram(program->program);
			delete program;
			
			_programs.erase(lookup);
		});
		
		std::vector<ShaderDefine> temporaryDefines;
		
		// Prepare the state
		if(lookup.type & ShaderProgram::TypeDiffuse)
			temporaryDefines.emplace_back(ShaderDefine("RN_TEXTURE_DIFFUSE", ""));
		
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
		if(lookup.type & ShaderProgram::TypePointShadows)
			temporaryDefines.emplace_back(ShaderDefine("RN_POINT_SHADOWS", ""));
		if(lookup.type & ShaderProgram::TypeSpotShadows)
			temporaryDefines.emplace_back(ShaderDefine("RN_SPOT_SHADOWS", ""));
		
		if(lookup.type & ShaderProgram::TypeFog)
			temporaryDefines.emplace_back(ShaderDefine("RN_FOG", ""));
		
		if(lookup.type & ShaderProgram::TypeClipPlane)
			temporaryDefines.emplace_back(ShaderDefine("RN_CLIPPLANE", ""));
		
		if(lookup.type & ShaderProgram::TypeGammaCorrection)
			temporaryDefines.emplace_back(ShaderDefine("RN_GAMMA_CORRECTION", ""));
		
		if(lookup.lightDirectionalCount > 0)
			temporaryDefines.emplace_back(ShaderDefine("RN_DIRECTIONAL_LIGHTS", static_cast<int>(lookup.lightDirectionalCount)));
		if(lookup.lightPointSpotCount > 0)
			temporaryDefines.emplace_back(ShaderDefine("RN_POINTSPOT_LIGHTS", 1));
		
		temporaryDefines.insert(temporaryDefines.end(), lookup.defines.begin(), lookup.defines.end());
		
		// Compile all units
		std::vector<ShaderUnit *> units;
		
		for(auto i = _shaderData.begin(); i != _shaderData.end(); i ++)
		{
			ShaderUnit *unit = new ShaderUnit(this, i->first);
			unit->Compile(temporaryDefines);
			
			units.push_back(unit);
			gl::AttachShader(program->program, unit->GetShader());
		}
		
		// Link the program
		gl::LinkProgram(program->program);
		RN_CHECKOPENGL();
		
		for(auto i = units.begin(); i != units.end(); i ++)
		{
			ShaderUnit *unit = *i;
			gl::DetachShader(program->program, unit->GetShader());
			
			delete unit;
		}
		
		// Get the program link status
		GLint status;
		
		gl::GetProgramiv(program->program, GL_LINK_STATUS, &status);
		if(!status)
		{
			DumpLinkStatusAndDie(program);
			return nullptr;
		}
		
		// Dump the scope guard and clear all defines that were just visible in this compilation unit
		scopeGuard.Commit();
		program->ReadLocations();
		
		RN_CHECKOPENGL();
		gl::Flush();
		
		// Update caches
		ShaderCache::GetSharedInstance()->CacheShaderProgram(this, program, lookup);
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
					path = FileManager::GetSharedInstance()->GetFilePathWithName(PathManager::Join(parent->GetPath(), name));
					break;
					
				case IncludeMode::IncludeDir:
					path = FileManager::GetSharedInstance()->GetFilePathWithName(name);
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
		_supportedPrograms |= IsDefined(result.data, "RN_POINT_SHADOWS") ? ShaderProgram::TypePointShadows : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_SPOT_SHADOWS") ? ShaderProgram::TypeSpotShadows : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_FOG") ? ShaderProgram::TypeFog : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_CLIPPLANE") ? ShaderProgram::TypeClipPlane : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_GAMMA_CORRECTION") ? ShaderProgram::TypeGammaCorrection : 0;
		_supportedPrograms |= IsDefined(result.data, "RN_TEXTURE_DIFFUSE") ? ShaderProgram::TypeDiffuse : 0;
		
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
	
	std::string Shader::GetFileHash() const
	{
		std::vector<std::pair<ShaderType, std::string>> files;
		
		for(auto& pair : _shaderData)
			files.emplace_back(std::make_pair(pair.first, pair.second.file));
		
		std::sort(files.begin(), files.end(), [](const std::pair<ShaderType, std::string>& a, const std::pair<ShaderType, std::string>& b) {
			return (a.first < b.first);
		});
		
		
		stl::sha2_context context;
		
		for(auto& pair : files)
		{
			context.update(reinterpret_cast<const uint8 *>(pair.second.data()), pair.second.length());
		}
		
		std::vector<uint8> sha2;
		context.finish(sha2);
		
		std::stringstream formatted;
		formatted << std::hex << std::setfill('0');
		
		for(uint8 byte : sha2)
		{
			formatted << std::setw(2) << static_cast<uint32>(byte);
		}
		
		return formatted.str();
	}
	
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
