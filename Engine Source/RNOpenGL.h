//
//  RNOpenGL.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENGL_H__
#define __RAYNE_OPENGL_H__

#include "RNBase.h"

namespace RN
{
	typedef enum
	{
		kOpenGLFeatureVertexArrays,
		
		__kOpenGLFeatureMax
	} OpenGLFeature;
	
	void ReadOpenGLExtensions();
	bool SupportsOpenGLFeature(OpenGLFeature feature);
	
	namespace gl
	{
		typedef void (*OGLFunctionGen)(GLsizei n, GLuint *a);
		typedef void (*OGLFunctionDelete)(GLsizei n, const GLuint *a);
		typedef void (*OGLFunctionBind)(GLuint n);
		typedef void (*OGLBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
		
		extern OGLFunctionGen GenVertexArrays;
		extern OGLFunctionDelete DeleteVertexArrays;
		extern OGLFunctionBind BindVertexArray;
		extern OGLBlitFramebuffer BlitFramebuffer;
	}
}

#endif /* __RAYNE_OPENGL_H__ */
