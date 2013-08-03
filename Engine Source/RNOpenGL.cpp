//
//  RNOpenGL.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenGL.h"
#include "RNContext.h"

namespace RN
{
	namespace gl
	{
		// Extensions
#ifdef GL_ARB_get_program_binary
		PFNGLGETPROGRAMBINARYPROC GetProgramBinary = nullptr;
		PFNGLPROGRAMBINARYPROC ProgramBinary = nullptr;
		PFNGLPROGRAMPARAMETERIPROC ProgramParameteri = nullptr;
#endif
		
#ifdef GL_ARB_vertex_array_object
		PFNGLBINDVERTEXARRAYPROC BindVertexArray = nullptr;
		PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays = nullptr;
		PFNGLGENVERTEXARRAYSPROC GenVertexArrays = nullptr;
		PFNGLISVERTEXARRAYPROC IsVertexArray = nullptr;
#endif
		
		//
		std::set<Feature> __features;
		std::unordered_set<std::string> __extensions;
		
		Version __maximumVersion;
		
		Version MaximumVersion()
		{
			return __maximumVersion;
		}
		
		Version WantedVersion()
		{
			return __maximumVersion;
		}
		
		
		
		void CheckForError(const char *file, int line)
		{
			GLenum error;
			while((error = glGetError()) != GL_NO_ERROR)
			{
				switch(error)
				{
					case GL_INVALID_ENUM:
						printf("OpenGL error: GL_INVALID_ENUM\n");
						break;
						
					case GL_INVALID_VALUE:
						printf("OpenGL error: GL_INVALID_VALUE\n");
						break;
						
					case GL_INVALID_OPERATION:
						printf("OpenGL error: GL_INVALID_OPERATION\n");
						break;
						
					case GL_OUT_OF_MEMORY:
						printf("OpenGL error: GL_OUT_OF_MEMOR\n");
						break;
						
					default:
						break;
				}
			}
		}
		
		bool SupportsFeature(Feature feature)
		{
			return (__features.find(feature) != __features.end());
		}
		
		bool SupportsExtensions(const std::string& extension)
		{
			return (__extensions.find(extension) != __extensions.end());
		}
		
		void AddFeature(Feature feature)
		{
			__features.insert(feature);
		}
		
		
		bool CanCreateContext(Version version)
		{
			try
			{
				Context *context = new Context(version);
				context->Release();
			}
			catch(Exception e)
			{
				return false;
			}
			
			return true;
		}
		
		void InitializeSupportedOpenGLVersions()
		{
#if RN_TARGET_OPENGL
			if(CanCreateContext(Version::Core4_1))
			{
				__maximumVersion = Version::Core4_1;
				return;
			}
			
			if(CanCreateContext(Version::Core3_2))
			{
				__maximumVersion = Version::Core3_2;
				return;
			}
#endif
			
			throw Exception(Exception::Type::NoGPUException, "Couldn't create a valid OpenGL context!");
		}
		
		void *BindOpenGLFunction(const char *tname)
		{
			static std::vector<std::string> extensions;
			static std::once_flag flag;
			
			std::call_once(flag, []{
				extensions.emplace_back("");
#if RN_TARGET_OPENGL
				extensions.emplace_back("ARB");
				extensions.emplace_back("EXT");
#endif
#if RN_TARGET_OPENGL_ES
				extensions.emplace_back("OES");
#endif
			});
			
			void *ptr;
			
			for(auto i = extensions.begin(); i != extensions.end(); i ++)
			{
				std::string symbol(tname);
				symbol += *i;
				
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS
				ptr = dlsym(RTLD_DEFAULT, symbol.c_str());
#endif
				
				if(ptr)
					return ptr;
			}
			
			return nullptr;
		}
		
		template<class T>
		void BindOpenGLFunction(T& ptr, const char *name)
		{
			ptr = reinterpret_cast<T>(BindOpenGLFunction(name));
		}
		
		Context *Initialize()
		{
			InitializeSupportedOpenGLVersions();
			
			Context *context = new Context(WantedVersion());
			Version version = context->Version();
			
			context->MakeActiveContext();
			
			// Read all extensions
			GLint extensions;
			glGetIntegerv(GL_NUM_EXTENSIONS, &extensions);
			
			for(GLint i = 0; i < extensions; i ++)
			{
				const GLubyte *extension = glGetStringi(GL_EXTENSIONS, i);
				__extensions.insert(reinterpret_cast<const char *>(extension));
			}
			
			// Get all features
			switch(version)
			{
#if RN_TARGET_OPENGL
				case Version::Core4_1:
					AddFeature(Feature::TessellationShaders);
					AddFeature(Feature::ShaderBinary);
					
				case Version::Core3_2:
					AddFeature(Feature::VertexArrays);
					AddFeature(Feature::GeometryShaders);
					
					if(SupportsExtensions("GL_EXT_texture_filter_anisotropic"))
					{
						AddFeature(Feature::AnisotropicFilter);
					}
					
					if(SupportsExtensions("GL_ARB_get_program_binary"))
					{
						AddFeature(Feature::ShaderBinary);
					}
					
#if RN_PLATFORM_MAC_OS
					{
						GLuint temp = glCreateShader(GL_TESS_CONTROL_SHADER);
						if(temp != 0)
						{
							AddFeature(Feature::TessellationShaders);
							glDeleteShader(temp);
						}
					}
#endif
					break;
					
#endif
			}
			
			if(SupportsFeature(Feature::VertexArrays))
			{
#ifdef GL_ARB_vertex_array_object
				BindOpenGLFunction(BindVertexArray, "glBindVertexArray");
				BindOpenGLFunction(DeleteVertexArrays, "glDeleteVertexArrays");
				BindOpenGLFunction(GenVertexArrays, "glGenVertexArrays");
				BindOpenGLFunction(IsVertexArray, "glIsVertexArray");
#endif
			}
			
			if(SupportsFeature(Feature::ShaderBinary))
			{
#ifdef GL_ARB_get_program_binary
				BindOpenGLFunction(GetProgramBinary, "glGetProgramBinary");
				BindOpenGLFunction(ProgramBinary, "glProgramBinary");
				BindOpenGLFunction(ProgramParameteri, "glProgramParameteri");
#endif
			}
			
			return context;
		}
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
PFNGLMAPBUFFERPROC glMapBuffer = 0;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange = 0;
PFNGLUNMAPBUFFERPROC glUnmapBuffer = 0;

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

#endif /* RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX */
