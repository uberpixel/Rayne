//
//  RNOpenGL.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
#if RN_PLATFORM_LINUX
		void *symbol = glXGetProcAddress(name);
#elif RN_PLATFORM_POSIX
		void *symbol = dlsym(RTLD_DEFAULT, name);
#elif RN_PLATFORM_WINDOWS
		void *symbol = wglGetProcAddress(name);
#endif
		
		if(!symbol)
		{
			std::string symbolname = std::string(name);
			
#if RN_PLATFORM_LINUX
#define TryOpenGLFunction(name) do { \
				std::string trysymbol = symbolname.append(#name); \
				void *temp = glXGetProcAddress(trysymbol.c_str()); \
				if(temp) \
					return temp; \
			} while(0)
#elif RN_PLATFORM_POSIX
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

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX

#if RN_PLATFORM_WINDOWS
#define RN_LINKGL(name, type) do{ name = (type)wglGetProcAddress(#name); if(!name) throw "wglGetProcAddress failed!"; } while(0)
#elif RN_PLATFORM_LINUX
#define RN_LINKGL(name, type) do{ name = (type)glXGetProcAddress(#name); if(!name) throw "glXGetProcAddress failed!"; } while(0)
#endif

		RN_LINKGL(glCreateProgram, PFNGLCREATEPROGRAMPROC);
		RN_LINKGL(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
		RN_LINKGL(glUseProgram, PFNGLUSEPROGRAMPROC);
		RN_LINKGL(glAttachShader, PFNGLATTACHSHADERPROC);
		RN_LINKGL(glDetachShader, PFNGLDETACHSHADERPROC);
		RN_LINKGL(glLinkProgram, PFNGLLINKPROGRAMPROC);
		RN_LINKGL(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
		RN_LINKGL(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
		RN_LINKGL(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
		RN_LINKGL(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
		RN_LINKGL(glUniform1i, PFNGLUNIFORM1IPROC);
		RN_LINKGL(glUniform1iv, PFNGLUNIFORM1IVPROC);
		RN_LINKGL(glUniform2iv, PFNGLUNIFORM2IVPROC);
		RN_LINKGL(glUniform3iv, PFNGLUNIFORM3IVPROC);
		RN_LINKGL(glUniform4iv, PFNGLUNIFORM4IVPROC);
		RN_LINKGL(glUniform1f, PFNGLUNIFORM1FPROC);
		RN_LINKGL(glUniform2f, PFNGLUNIFORM2FPROC);
		RN_LINKGL(glUniform3f, PFNGLUNIFORM3FPROC);
		RN_LINKGL(glUniform4f, PFNGLUNIFORM4FPROC);
		RN_LINKGL(glUniform1fv, PFNGLUNIFORM1FVPROC);
		RN_LINKGL(glUniform2fv, PFNGLUNIFORM2FVPROC);
		RN_LINKGL(glUniform3fv, PFNGLUNIFORM3FVPROC);
		RN_LINKGL(glUniform4fv, PFNGLUNIFORM4FVPROC);
		RN_LINKGL(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
		RN_LINKGL(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
		RN_LINKGL(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC);
		RN_LINKGL(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC);
		RN_LINKGL(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC);
		RN_LINKGL(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC);
		RN_LINKGL(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC);
		RN_LINKGL(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
		RN_LINKGL(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
		RN_LINKGL(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
		RN_LINKGL(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC);
		RN_LINKGL(glBindFragDataLocation, PFNGLBINDFRAGDATALOCATIONPROC);
		RN_LINKGL(glDrawBuffers, PFNGLDRAWBUFFERSPROC);
		RN_LINKGL(glTexBuffer, PFNGLTEXBUFFERPROC);

		// Shader
		RN_LINKGL(glCreateShader, PFNGLCREATESHADERPROC);
		RN_LINKGL(glDeleteShader, PFNGLDELETESHADERPROC);
		RN_LINKGL(glShaderSource, PFNGLSHADERSOURCEPROC);
		RN_LINKGL(glCompileShader, PFNGLCOMPILESHADERPROC);
		RN_LINKGL(glGetShaderiv, PFNGLGETSHADERIVPROC);

		// VBO
		RN_LINKGL(glGenBuffers, PFNGLGENBUFFERSPROC);
		RN_LINKGL(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
		RN_LINKGL(glBindBuffer, PFNGLBINDBUFFERPROC);
		RN_LINKGL(glBufferData, PFNGLBUFFERDATAPROC);
		RN_LINKGL(glBufferSubData, PFNGLBUFFERSUBDATAPROC);
		RN_LINKGL(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);

		// FBO
		RN_LINKGL(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC);
		RN_LINKGL(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC);
		RN_LINKGL(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
		RN_LINKGL(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC);
		RN_LINKGL(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC);
		RN_LINKGL(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC);
		RN_LINKGL(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC);
		RN_LINKGL(glRenderbufferStorage, PFNGLRENDERBUFFERSTORAGEPROC);
		RN_LINKGL(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC);
		RN_LINKGL(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC);

		// Textures
		#if RN_PLATFORM_WINDOWS
		RN_LINKGL(glActiveTexture, PFNGLACTIVETEXTUREPROC);
		#endif
		RN_LINKGL(glGenerateMipmap, PFNGLGENERATEMIPMAPPROC);

#endif
	}
}


#if RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX

#if RN_PLATFORM_WINDOWS
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 0;
#endif

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
PFNGLUNIFORM2FPROC glUniform2f = 0;
PFNGLUNIFORM3FPROC glUniform3f = 0;
PFNGLUNIFORM4FPROC glUniform4f = 0;
PFNGLUNIFORM1FVPROC glUniform1fv = 0;
PFNGLUNIFORM2FVPROC glUniform2fv = 0;
PFNGLUNIFORM3FVPROC glUniform3fv = 0;
PFNGLUNIFORM4FVPROC glUniform4fv = 0;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = 0;
PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv = 0;
PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv = 0;
PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv = 0;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = 0;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = 0;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = 0;
PFNGLDRAWBUFFERSPROC glDrawBuffers = 0;
PFNGLTEXBUFFERPROC glTexBuffer = 0;

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
PFNGLBUFFERSUBDATAPROC glBufferSubData = 0;
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
#if RN_PLATFORM_WINDOWS
PFNGLACTIVETEXTUREPROC glActiveTexture = 0;
#endif
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = 0;

#endif
