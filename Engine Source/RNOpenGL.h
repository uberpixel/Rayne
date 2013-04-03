//
//  RNOpenGL.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENGL_H__
#define __RAYNE_OPENGL_H__

#include "RNBase.h"
#include "RNOpenGLExtensions.h"

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

namespace RN
{
	typedef enum
	{
		kOpenGLFeatureVertexArrays,
		kOpenGLFeatureBlitFramebuffer,
		kOpenGLFeatureInstancing,
		
		__kOpenGLFeatureMax
	} OpenGLFeature;
	
	RNAPI void ReadOpenGLExtensions();
	RNAPI bool SupportsOpenGLFeature(OpenGLFeature feature);
	
	namespace gl
	{
		extern void (APIENTRYP GenVertexArrays)(GLsizei n, GLuint *arrays);
		extern void (APIENTRYP DeleteVertexArrays)(GLsizei n, const GLuint *arrays);
		extern void (APIENTRYP BindVertexArray)(GLuint array);
		
		extern void (APIENTRYP BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	
		extern void (APIENTRYP VertexAttribDivisor)(GLuint index, GLuint divisor);
		extern void (APIENTRYP DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount);
	}
}

#define __RN_EXPANDOPENGLERROR(error) #error
#define __RN_OPENGLTOKEN(error) __RN_EXPANDOPENGLERROR(error)

#define __RN_REPORTOPENGLERROR(error) \
do { printf("OpenGL error: %s. File: %s:%i", __RN_OPENGLTOKEN(error), __FILE__, __LINE__); } while(0)

#define __RN_CHECKOPENGLERROR(error) \
case error: \
__RN_REPORTOPENGLERROR(error); \
break;

#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 0xffffffff
#endif

#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW (0xffffffff - 1)
#endif

#ifndef GL_TABLE_TOO_LARGE
#define GL_TABLE_TOO_LARGE (0xffffffff - 2)
#endif

#ifndef NDEBUG
#define RN_CHECKOPENGL() \
	while(0) { \
		GLenum error; \
		while((error = glGetError()) != GL_NO_ERROR) \
		{ \
			switch(error) \
			{ \
					__RN_CHECKOPENGLERROR(GL_INVALID_ENUM) \
					__RN_CHECKOPENGLERROR(GL_INVALID_VALUE) \
					__RN_CHECKOPENGLERROR(GL_INVALID_OPERATION) \
					__RN_CHECKOPENGLERROR(GL_INVALID_FRAMEBUFFER_OPERATION) \
					__RN_CHECKOPENGLERROR(GL_OUT_OF_MEMORY) \
					__RN_CHECKOPENGLERROR(GL_STACK_UNDERFLOW) \
					__RN_CHECKOPENGLERROR(GL_STACK_OVERFLOW) \
					__RN_CHECKOPENGLERROR(GL_TABLE_TOO_LARGE) \
				default: \
					printf("Unknown OpenGL error: %i. File: %s:%i", error, __FILE__, __LINE__); \
					break; \
			} \
		} \
	}
#else
#define RN_CHECKOPENGL() (void)0
#endif

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX

extern "C"
{	
	#if RN_PLATFORM_WINDOWS
	extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	extern PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
	extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	#endif
	
	extern PFNGLCREATEPROGRAMPROC glCreateProgram;
	extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
	extern PFNGLUSEPROGRAMPROC glUseProgram;
	extern PFNGLATTACHSHADERPROC glAttachShader;
	extern PFNGLDETACHSHADERPROC glDetachShader;
	extern PFNGLLINKPROGRAMPROC glLinkProgram;
	extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
	extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	extern PFNGLUNIFORM1IPROC glUniform1i;
	extern PFNGLUNIFORM1IVPROC glUniform1iv;
	extern PFNGLUNIFORM2IVPROC glUniform2iv;
	extern PFNGLUNIFORM3IVPROC glUniform3iv;
	extern PFNGLUNIFORM4IVPROC glUniform4iv;
	extern PFNGLUNIFORM1FPROC glUniform1f;
	extern PFNGLUNIFORM2FPROC glUniform2f;
	extern PFNGLUNIFORM3FPROC glUniform3f;
	extern PFNGLUNIFORM4FPROC glUniform4f;
	extern PFNGLUNIFORM1FVPROC glUniform1fv;
	extern PFNGLUNIFORM2FVPROC glUniform2fv;
	extern PFNGLUNIFORM3FVPROC glUniform3fv;
	extern PFNGLUNIFORM4FVPROC glUniform4fv;
	extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
	extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
	extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
	extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
	extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
	extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
	extern PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
	extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
	extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
	extern PFNGLTEXBUFFERPROC glTexBuffer;
	
	// Shader
	extern PFNGLCREATESHADERPROC glCreateShader;
	extern PFNGLDELETESHADERPROC glDeleteShader;
	extern PFNGLSHADERSOURCEPROC glShaderSource;
	extern PFNGLCOMPILESHADERPROC glCompileShader;
	extern PFNGLGETSHADERIVPROC glGetShaderiv;
	
	// VBO
	extern PFNGLGENBUFFERSPROC glGenBuffers;
	extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
	extern PFNGLBINDBUFFERPROC glBindBuffer;
	extern PFNGLBUFFERDATAPROC glBufferData;
	extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
	extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	extern PFNGLMAPBUFFERPROC glMapBuffer;
	extern PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
	extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
	
	// FBO
	extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
	extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	
	// Textures
#if RN_PLATFORM_WINDOWS
	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif
	extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
}

#endif

#endif /* __RAYNE_OPENGL_H__ */
