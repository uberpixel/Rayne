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
		void (APIENTRYP GenVertexArrays)(GLsizei n, GLuint *arrays) = 0;
		void (APIENTRYP DeleteVertexArrays)(GLsizei n, const GLuint *arrays) = 0;
		void (APIENTRYP BindVertexArray)(GLuint array) = 0;
		
		void (APIENTRYP BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) = 0;
		
		void (APIENTRYP VertexAttribDivisor)(GLuint index, GLuint divisor);
		void (APIENTRYP DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount);
		
		bool OpenGLFeatures[__kOpenGLFeatureMax] = { false };
	}
	
	void *LookupOpenGLFunction(const char *name)
	{		
#if RN_PLATFORM_POSIX
		void *symbol = dlsym(RTLD_DEFAULT, name);
#elif RN_PLATFORM_WINDOWS
		void *symbol = wglGetProcAddress(name);
#endif
		
		if(!symbol)
		{
			std::string symbolname = std::string(name);
			
#if RN_PLATFORM_POSIX
#define TryOpenGLFunction(name) do { \
				std::string trysymbol = symbolname.append(#name); \
				void *temp = dlsym(RTLD_DEFAULT, trysymbol.c_str()); \
				if(temp) \
					return temp; \
			} while(0)
#elif RN_PLATFOM_WINDOWS
#define TryOpenGLFunction(name) do { \
				std::string trysymbol = symbolname.append(#name); \
				void *temp = wglGetProcAddress(trysymbol.c_str()); \
				if(temp) \
					return temp; \
				} while(0)
#endif
			
			
#if RN_TARGET_OPENGL_ES
			TryOpenGLFunction(OES);
			TryOpenGLFunction(EXT);
			
#if RN_PLATFORM_IOS
			TryOpenGLFunction(APPLE);
#endif
#endif
			
#if RN_TARGET_OPENGL
			TryOpenGLFunction(ARB);
			TryOpenGLFunction(EXT);
			
#if RN_PLATFORM_MAC_OS
			TryOpenGLFunction(APPLE);
#endif
#endif
		}
		
		return symbol;
	}
	
	
	
	bool LookupOpenGLExtension(void ***fpointers, const char **fnames, uint32 functions)
	{
		bool hasSupport = true;
		
		for(uint32 i=0; i<functions; i++)
		{
			const char *name = fnames[i];
			void **fpointer = fpointers[i];
			void *pointer = LookupOpenGLFunction(name);
			
			if(fpointer)
				*fpointer = pointer;
			
			if(!pointer)
				hasSupport = false;
		}
		
		return hasSupport;
	}
	
	bool SupportsOpenGLFeature(OpenGLFeature feature)
	{
		int index = (int)feature;
		return gl::OpenGLFeatures[index];
	}

	

	
	void __ReadVAOExtension()
	{
		void **fpointer[] =
		{
			(void **)&gl::GenVertexArrays,
			(void **)&gl::DeleteVertexArrays,
			(void **)&gl::BindVertexArray
		};
		
		const char *fnames[] =
		{
			"glGenVertexArrays",
			"glDeleteVertexArrays",
			"glBindVertexArray"
		};
		
		bool supports = LookupOpenGLExtension(fpointer, fnames, 3);
		gl::OpenGLFeatures[(int)kOpenGLFeatureVertexArrays] = supports;
	}
	
	void __ReadBlitFramebufferExtension()
	{
		void **fpointer[] =
		{
			(void **)&gl::BlitFramebuffer
		};
		
		const char *fnames[] =
		{
			"glBlitFramebuffer"
		};
		
		bool supports = LookupOpenGLExtension(fpointer, fnames, 1);
		gl::OpenGLFeatures[(int)kOpenGLFeatureBlitFramebuffer] = supports;
	}
	
	void __ReadInstancingExtension()
	{
		void **fpointer[] =
		{
			(void **)&gl::VertexAttribDivisor,
			(void **)&gl::DrawElementsInstanced
		};
		
		const char *fnames[] =
		{
			"glVertexAttribDivisor",
			"glDrawElementsInstanced"
		};
		
		bool supports = LookupOpenGLExtension(fpointer, fnames, 2);
		gl::OpenGLFeatures[(int)kOpenGLFeatureInstancing] = supports;
	}
	
	
	
	void ReadOpenGLExtensions()
	{
		__ReadVAOExtension();
		__ReadBlitFramebufferExtension();
		__ReadInstancingExtension();

#if RN_PLATFORM_WINDOWS
#define RN_LINKWGL(name, type) do{ name = (type)wglGetProcAddress(#name); if(!name) throw "Fuck that shit!"; } while(0)

		RN_LINKWGL(glCreateProgram, PFNGLCREATEPROGRAMPROC);
		RN_LINKWGL(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
		RN_LINKWGL(glUseProgram, PFNGLUSEPROGRAMPROC);
		RN_LINKWGL(glAttachShader, PFNGLATTACHSHADERPROC);
		RN_LINKWGL(glDetachShader, PFNGLDETACHSHADERPROC);
		RN_LINKWGL(glLinkProgram, PFNGLLINKPROGRAMPROC);
		RN_LINKWGL(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
		RN_LINKWGL(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
		RN_LINKWGL(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
		RN_LINKWGL(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
		RN_LINKWGL(glUniform1i, PFNGLUNIFORM1IPROC);
		RN_LINKWGL(glUniform1iv, PFNGLUNIFORM1IVPROC);
		RN_LINKWGL(glUniform2iv, PFNGLUNIFORM2IVPROC);
		RN_LINKWGL(glUniform3iv, PFNGLUNIFORM3IVPROC);
		RN_LINKWGL(glUniform4iv, PFNGLUNIFORM4IVPROC);
		RN_LINKWGL(glUniform1f, PFNGLUNIFORM1FPROC);
		RN_LINKWGL(glUniform1fv, PFNGLUNIFORM1FVPROC);
		RN_LINKWGL(glUniform2fv, PFNGLUNIFORM2FVPROC);
		RN_LINKWGL(glUniform3fv, PFNGLUNIFORM3FVPROC);
		RN_LINKWGL(glUniform4fv, PFNGLUNIFORM4FVPROC);
		RN_LINKWGL(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
		RN_LINKWGL(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
		RN_LINKWGL(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC);
		RN_LINKWGL(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC);
		RN_LINKWGL(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC);
		RN_LINKWGL(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC);
		RN_LINKWGL(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC);
		RN_LINKWGL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
		RN_LINKWGL(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
		RN_LINKWGL(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
		RN_LINKWGL(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC);
		RN_LINKWGL(glBindFragDataLocation, PFNGLBINDFRAGDATALOCATIONPROC);

		// Shader
		RN_LINKWGL(glCreateShader, PFNGLCREATESHADERPROC);
		RN_LINKWGL(glDeleteShader, PFNGLDELETESHADERPROC);
		RN_LINKWGL(glShaderSource, PFNGLSHADERSOURCEPROC);
		RN_LINKWGL(glCompileShader, PFNGLCOMPILESHADERPROC);
		RN_LINKWGL(glGetShaderiv, PFNGLGETSHADERIVPROC);

		// VBO
		RN_LINKWGL(glGenBuffers, PFNGLGENBUFFERSPROC);
		RN_LINKWGL(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
		RN_LINKWGL(glBindBuffer, PFNGLBINDBUFFERPROC);
		RN_LINKWGL(glBufferData, PFNGLBUFFERDATAPROC);
		RN_LINKWGL(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);

		// FBO
		RN_LINKWGL(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
		RN_LINKWGL(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
		RN_LINKWGL(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
		RN_LINKWGL(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
		RN_LINKWGL(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC);
		RN_LINKWGL(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC);
		RN_LINKWGL(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC);
		RN_LINKWGL(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC);
		RN_LINKWGL(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC);
		RN_LINKWGL(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);

		// Textures
		RN_LINKWGL(glActiveTexture, PFNGLACTIVETEXTUREPROC);
		RN_LINKWGL(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);

#endif
	}
}


#if RN_PLATFORM_WINDOWS

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 0;

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
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = 0;

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
