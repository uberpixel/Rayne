//
//  RNOpenGL.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenGL.h"

namespace RN
{
	namespace gl
	{
		OGLFunctionGen GenVertexArrays;
		OGLFunctionDelete DeleteVertexArrays;
		OGLFunctionBind BindVertexArray;
		OGLBlitFramebuffer BlitFramebuffer;
	}
	
	bool OpenGLFeatures[__kOpenGLFeatureMax] = { false };
	
	void *LookupOpenGLFunction(const char *name)
	{		
#if RN_PLATFORM_POSIX
		void *symbol = dlsym(RTLD_DEFAULT, name);
		
		if(!symbol)
		{
			std::string symbolname = std::string(name);
			
			std::string symbolOES = symbolname.append("OES");
			std::string symbolARB = symbolname.append("ARB");
			
			symbol = dlsym(RTLD_DEFAULT, symbolOES.c_str());
			if(symbol)
				return symbol;
			
			
			symbol = dlsym(RTLD_DEFAULT, symbolARB.c_str());
			if(symbol)
				return symbol;
		}
		
		return symbol;
#endif
		
#if RN_PLATFORM_WINDOWS
		return wglGetProcAddress(name);
#endif
	}
	
	bool SupportsOpenGLFeature(OpenGLFeature feature)
	{
		int index = (int)feature;
		return OpenGLFeatures[index];
	}
	
	void ReadOpenGLExtensions()
	{		
		gl::GenVertexArrays = (gl::OGLFunctionGen)LookupOpenGLFunction("glGenVertexArrays");
		gl::DeleteVertexArrays = (gl::OGLFunctionDelete)LookupOpenGLFunction("glDeleteVertexArrays");
		gl::BindVertexArray = (gl::OGLFunctionBind)LookupOpenGLFunction("glBindVertexArray");
		
		OpenGLFeatures[kOpenGLFeatureVertexArrays] = (gl::GenVertexArrays && gl::DeleteVertexArrays && gl::BindVertexArray);
		
		gl::BlitFramebuffer = (gl::OGLBlitFramebuffer)LookupOpenGLFunction("glBlitFramebuffer");
	}
}


#if RN_PLATFORM_WINDOWS

PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;
PFNGLUSEPROGRAMPROC glUseProgram = 0;
PFNGLATTACHSHADERPROC glAttachShader = 0;
PFNGLDETACHSHADERPROC glDetachShader = 0;
PFNGLLINKPROGRAMPROC glLinkProgram = 0;
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;
PFNGLUNIFORM1IPROC glUniform1i = 0;
PFNGLUNIFORM1IVPROC glUniform1iv = 0;
PFNGLUNIFORM2IVPROC glUniform2iv = 0;
PFNGLUNIFORM3IVPROC glUniform3iv = 0;
PFNGLUNIFORM4IVPROC glUniform4iv = 0;
PFNGLUNIFORM1FPROC glUniform1f = 0;
PFNGLUNIFORM1FVPROC glUniform1fv = 0;
PFNGLUNIFORM2FVPROC glUniform2fv = 0;
PFNGLUNIFORM3FVPROC glUniform3fv = 0;
PFNGLUNIFORM4FVPROC glUniform4fv = 0;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = 0;
PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f = 0;
PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv = 0;
PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv = 0;
PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv = 0;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = 0;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = 0;

// Shader
PFNGLCREATESHADERPROC glCreateShader = 0;
PFNGLDELETESHADERPROC glDeleteShader = 0;
PFNGLSHADERSOURCEPROC glShaderSource = 0;
PFNGLCOMPILESHADERPROC glCompileShader = 0;
PFNGLGETSHADERIVPROC glGetShaderiv = 0;

// VBO
PFNGLGENBUFFERSPROC glGenBuffers = 0;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = 0;
PFNGLBINDBUFFERPROC	glBindBuffer = 0;
PFNGLBUFFERDATAPROC	glBufferData = 0;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = 0;

// FBO
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = 0;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = 0;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = 0;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = 0;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = 0;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = 0;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = 0;

// Textures
PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = 0;

#endif
