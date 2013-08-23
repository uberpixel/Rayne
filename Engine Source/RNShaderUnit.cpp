//
//  RNShaderUnit.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderUnit.h"
#include "RNShader.h"

namespace RN
{
	ShaderUnit::ShaderUnit(Shader *shader, ShaderType type) :
		_type(type),
		_host(shader)
	{
		RN_ASSERT(_host, "Shader mustn't be NULL");
		_shader = 0;
	}
	
	ShaderUnit::~ShaderUnit()
	{
		if(_shader)
			glDeleteShader(_shader);
	}
	
	
	
	
	void ShaderUnit::Compile(const std::vector<ShaderDefine>& tdefines)
	{
		std::vector<ShaderDefine> defines(tdefines);
		
		switch(_type)
		{
			case ShaderType::VertexShader:
				defines.emplace_back(ShaderDefine("RN_VERTEX_SHADER", "1"));
				break;
				
			case ShaderType::FragmentShader:
				defines.emplace_back(ShaderDefine("RN_FRAGMENT_SHADER", "1"));
				break;
				
			case ShaderType::GeometryShader:
				defines.emplace_back(ShaderDefine("RN_GEOMETRY_SHADER", "1"));
				break;
				
			case ShaderType::TessellationControlShader:
				defines.emplace_back(ShaderDefine("RN_TESSELLATION_CONTROL_SHADER", "1"));
				break;
				
			case ShaderType::TessellationEvaluationShader:
				defines.emplace_back(ShaderDefine("RN_TESSELLATION_EVALUATION_SHADER", "1"));
				break;
		}
		
		auto source = std::move(PreProcessedShaderSource(_host->GetShaderSource(_type), defines));
		const GLchar *data = source.first.c_str();
		
		if(_shader)
			glDeleteShader(_shader);
		
		_shader = glCreateShader(GetOpenGLType());
		
		glShaderSource(_shader, 1, &data, NULL);
		glCompileShader(_shader);
		
		// Check the compilation status
		GLint status, length;
		glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
		
		if(!status)
		{
			glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
			
			char *log = new char[length];
			glGetShaderInfoLog(_shader, length, &length, static_cast<GLchar *>(log));
			glDeleteShader(_shader);
			
			_shader = 0;
			
			std::string error(log);
			delete [] log;
			
			ParseErrorAndThrow(error, source.second);
		}
	}
	
	std::pair<std::string, size_t> ShaderUnit::PreProcessedShaderSource(const std::string& source, const std::vector<ShaderDefine>& defines)
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
		
		for(const ShaderDefine& define : defines)
		{
			cleanedDefines.insert(std::unordered_map<std::string, ShaderDefine>::value_type(define.name, define));
		}
		
		for(auto i=cleanedDefines.begin(); i!=cleanedDefines.end(); i++)
		{
			std::string exploded = "#define " + i->second.name + " " + i->second.value + "\n";
			
			data.insert(index, exploded);
			index += exploded.length();
		}
		
		return std::pair<std::string, size_t>(data, cleanedDefines.size());
	}
	
	void ShaderUnit::ParseErrorAndThrow(const std::string& error, size_t offst)
	{
		std::string result(error);
		
		// Parse the error
		if(_host)
		{
			std::string log = "Failed to compile shader\n";
			std::regex regex("ERROR: [0-9]{0,}:[0-9]{0,}: .*\n", std::regex_constants::ECMAScript | std::regex_constants::icase);
			
			if(std::regex_search(error, regex))
			{
				std::regex lineRegex("[0-9]{0,}:[0-9]{0,}:", std::regex_constants::ECMAScript | std::regex_constants::icase);
				std::string parsedError = "";
				
				for(auto i = std::sregex_iterator(error.begin(), error.end(), regex); i != std::sregex_iterator(); i ++)
				{
					std::string match = i->str();
					std::smatch lineMatch;
					
					std::regex_search(match, lineMatch, lineRegex);
					std::string lineString = lineMatch.str();
					
					bool skippedColumn = false;
					uint32 line = 0;
					
					for(size_t j = 0; j < lineString.length() - 1; j ++)
					{
						if(!skippedColumn && lineString[j] == ':')
						{
							skippedColumn = true;
							continue;
						}
						
						if(skippedColumn)
						{
							std::string temp = lineString.substr(j, (lineString.length() - 1) - j);
							line = atoi(temp.c_str());
							
							break;
						}
					}
					
					
					Shader::DebugMarker marker = _host->ResolveFileForLine(_type, line);
					char buffer[32];
					
					sprintf(buffer, "%u", static_cast<uint32>(marker.line - offst));
					parsedError += marker.file + " " + buffer + ", Error: " + match.substr(lineString.length() + 8) + "\n";
				}
				
				result = parsedError;
			}
		}
		
		throw Exception(Exception::Type::ShaderCompilationFailedException, result);
	}
	
	
	
	GLenum ShaderUnit::GetOpenGLType() const
	{
		switch(_type)
		{
			case ShaderType::VertexShader:
				return GL_VERTEX_SHADER;
				
			case ShaderType::FragmentShader:
				return GL_FRAGMENT_SHADER;
				
			case ShaderType::GeometryShader:
				return GL_GEOMETRY_SHADER;
				
#ifdef GL_TESS_CONTROL_SHADER
			case ShaderType::TessellationControlShader:
				if(gl::SupportsFeature(gl::Feature::TessellationShaders))
				   return GL_TESS_CONTROL_SHADER;
				
				break;
#endif
				
#ifdef GL_TESS_EVALUATION_SHADER
			case ShaderType::TessellationEvaluationShader:
			   if(gl::SupportsFeature(gl::Feature::TessellationShaders))
				  return GL_TESS_CONTROL_SHADER;
				  
				break;
#endif
				
			default:
				   break;
		}
				   
		throw Exception(Exception::Type::ShaderUnsupportedException, "");
	}
}
