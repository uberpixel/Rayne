//
//  RNOpenGL.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBase.h"
#include "RNOpenGL.h"
#include "RNContextInternal.h"
#include "RNLogging.h"
#include "RNSettings.h"
#include "RNString.h"

namespace RN
{
	extern void BindOpenGLCore();
	extern void BindOpenGLFunctions(gl::Version version);
	extern void BindOpenGLExtensions(gl::Version version);
	
	namespace gl
	{
		std::set<Feature> __features;
		std::unordered_set<std::string> __extensions;
		
		Version __maximumVersion;
		
		Version MaximumVersion()
		{
			return __maximumVersion;
		}
		
		Version WantedVersion()
		{
			Version preferredVersion = __maximumVersion;
			
			try
			{
				String *renderer = Settings::GetSharedInstance()->GetObjectForKey<String>(kRNSettingsRendererKey);
				if(renderer)
				{
					if(renderer->IsEqual(RNCSTR("3.2")))
						preferredVersion = Version::Core3_2;
					
					if(renderer->IsEqual(RNCSTR("4.1")))
						preferredVersion = Version::Core4_1;
				}
			}
			catch(Exception e)
			{
				preferredVersion = __maximumVersion;
			}
			
			return std::min(preferredVersion, __maximumVersion);
		}
		
		
		
		void CheckForError(const char *file, int line)
		{
			GLenum error;
			while((error = gl::GetError()) != GL_NO_ERROR)
			{
				Log::Loggable loggable(Log::Level::Error);
				loggable << "OpenGL error: ";
				
				switch(error)
				{
					case GL_INVALID_ENUM:
						loggable << "GL_INVALID_ENUM";
						break;
						
					case GL_INVALID_VALUE:
						loggable << "GL_INVALID_VALUE";
						break;
						
					case GL_INVALID_OPERATION:
						loggable << "GL_INVALID_OPERATION";
						break;
						
					case GL_OUT_OF_MEMORY:
						loggable << "GL_OUT_OF_MEMORY";
						break;
						
#if RN_TARGET_OPENGL
					case GL_INVALID_FRAMEBUFFER_OPERATION:
						loggable << "GL_INVALID_FRAMEBUFFER_OPERATION";
						break;
#endif
						
					default:
						loggable << std::hex << error;
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
		
		Context *Initialize()
		{
			InitializeSupportedOpenGLVersions();
			
			Context *context = new Context(WantedVersion());
			Version version = context->GetVersion();
			
			context->MakeActiveContext();
			BindOpenGLCore();
			BindOpenGLFunctions(version);
			
			// Read all extensions
			GLint extensions;
			gl::GetIntegerv(GL_NUM_EXTENSIONS, &extensions);
			
			for(GLint i = 0; i < extensions; i ++)
			{
				const GLubyte *extension = gl::GetStringi(GL_EXTENSIONS, i);
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
						AddFeature(Feature::AnisotropicFilter);
					
					if(SupportsExtensions("GL_ARB_get_program_binary"))
						AddFeature(Feature::ShaderBinary);
					
					break;
#endif
			}
			
			BindOpenGLExtensions(version);
			
			Log::Logger *logger = Log::Logger::GetSharedInstance();
			
			logger->Log(Log::Message(Log::Level::Info, "Vendor", reinterpret_cast<const char *>(gl::GetString(GL_VENDOR))));
			logger->Log(Log::Message(Log::Level::Info, "Renderer", reinterpret_cast<const char *>(gl::GetString(GL_RENDERER))));
			logger->Log(Log::Message(Log::Level::Info, "Version", reinterpret_cast<const char *>(gl::GetString(GL_VERSION))));
			
			std::stringstream exts;
			std::copy(__extensions.begin(), __extensions.end(), std::ostream_iterator<std::string>(exts, "\n"));
			
			logger->Log(Log::Message(Log::Level::Info, "Extensions", exts.str()));
			
			OpenGLQueue::GetSharedInstance()->SwitchContext(context);
			return context;
		}
	}
}
