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
	namespace gl
	{
		enum class Version
		{
#if RN_TARGET_OPENGL_ES
			ES2,
#endif
#if RN_TARGET_OPENGL
			Core3_2,
			Core4_1
#endif
		};
		
		enum class Feature : int
		{
			VertexArrays,
			GeometryShaders,
			TessellationShaders,
			AnisotropicFilter,
			ShaderBinary
		};
		
		RNAPI Version MaximumVersion();
		RNAPI Version WantedVersion();
		
		RNAPI void CheckForError(const char *file, int line);
		RNAPI bool SupportsFeature(Feature feature);
		RNAPI bool SupportsExtensions(const std::string& extension);
		
#ifdef GL_ARB_get_program_binary
		extern PFNGLGETPROGRAMBINARYPROC GetProgramBinary;
		extern PFNGLPROGRAMBINARYPROC ProgramBinary;
		extern PFNGLPROGRAMPARAMETERIPROC ProgramParameteri;
#endif
		
#ifdef GL_ARB_vertex_array_object
		extern PFNGLBINDVERTEXARRAYPROC BindVertexArray;
		extern PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
		extern PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
		extern PFNGLISVERTEXARRAYPROC IsVertexArray;
#endif
	}
}

#ifndef NDEBUG
	#define RN_CHECKOPENGL() RN::gl::CheckForError(__FILE__, __LINE__)
	#define RN_CHECKOPENGL_AGGRESSIVE() RN::gl::CheckForError(__FILE__, __LINE__)
#else
	#define RN_CHECKOPENGL() (void)0
	#define RN_CHECKOPENGL_AGGRESSIVE() (void)0
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
	
	// VAO
	extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
	extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
	extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
	extern PFNGLISVERTEXARRAYPROC glIsVertexArray;
	
	// Textures
#if RN_PLATFORM_WINDOWS
	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif
	extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
}

#endif /* RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX */

#endif /* __RAYNE_OPENGL_H__ */
