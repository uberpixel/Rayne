//
//  RNOpenGL.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <dlfcn.h>
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
