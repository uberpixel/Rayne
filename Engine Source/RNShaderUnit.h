//
//  RNShaderUnit.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADERUNIT_H__
#define __RAYNE_SHADERUNIT_H__

#include "RNBase.h"
#include "RNShaderLookup.h"

namespace RN
{
	enum class ShaderType : int
	{
		VertexShader,
		FragmentShader,
		GeometryShader,
		TessellationControlShader,
		TessellationEvaluationShader
	};
	
	class Shader;
	class ShaderUnit
	{
	public:
		RNAPI ShaderUnit(Shader *shader, ShaderType type);
		RNAPI ~ShaderUnit();
		
		RNAPI void Compile(const std::vector<ShaderDefine>& defines);
		
		RNAPI ShaderType GetType() const { return _type; }
		RNAPI GLuint GetShader() const { return _shader; }
		RNAPI GLenum GetOpenGLType() const;
		
	private:
		std::pair<std::string, size_t> PreProcessedShaderSource(const std::string& source, const std::vector<ShaderDefine>& defines);
		void ParseErrorAndThrow(const std::string& error, size_t offset);
		
		Shader *_host;
		ShaderType _type;
		GLuint _shader;
	};
}

#endif /* __RAYNE_SHADERUNIT_H__ */
