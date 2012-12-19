//
//  RNShader.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADER_H__
#define __RAYNE_SHADER_H__

#include <string>
#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Shader : public Object
	{
	public:
		Shader();
		virtual ~Shader();
		
		bool SetVertexShader(const std::string& path, Error *error);
		bool SetFragmentShader(const std::string& path, Error *error);
		bool SetGeometryShader(const std::string& path, Error *error);
		
		bool Link(Error *error);
		
	private:
		bool SetShaderForType(const std::string& path, GLenum type, Error *error);
		
		GLuint _vertexShader;
		GLuint _fragmentShader;
		GLuint _geometryShader;
		
		GLuint _program;
		
		GLuint _matProj;
		GLuint _matProjInverse;
		GLuint _matView;
		GLuint _matViewInverse;
		GLuint _matModel;
		GLuint _matModelInverse;
		GLuint _matProjViewModel;
		GLuint _matProjViewModelInverse;
		
		GLuint _position;
		GLuint _texcoord0;
		GLuint _texcoord1;
	};
}

#endif /* __RAYNE_SHADER_H__ */
