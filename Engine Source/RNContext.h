//
//  RNContext.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXT_H__
#define __RAYNE_CONTEXT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"
#include "RNRenderingResource.h"

namespace RN
{
	class Window;
	class Context : public Object, public RenderingResource
	{
	friend class Window;
	public:
		enum
		{
			DepthBufferSize8  = (1 << 1),
			DepthBufferSize16 = (1 << 2),
			DepthBufferSize24 = (1 << 3),
			
			StencilBufferSize8  = (1 << 4),
			StencilBufferSize16 = (1 << 5),
			StencilBufferSize24 = (1 << 6)
		};
		typedef int32 ContextFlags;
		
		Context(ContextFlags flags, Context *shared=0);
		Context(Context *shared);
		
		virtual ~Context();
		
		void MakeActiveContext();
		void DeactivateContext();
		
		virtual void Flush();
		virtual void SetName(const char *name);
		
		static Context *ActiveContext();
		
	protected:
		virtual void Activate();
		virtual void Deactivate();
		
	private:
		bool _active;
		Thread *_thread;
		
		int32 _glsl;
		ContextFlags _flags;
		
#if RN_PLATFORM_MAC_OS
		NSOpenGLPixelFormat *CreatePixelFormat(ContextFlags flags);
		
		Context *_shared;
		NSOpenGLContext *_oglContext;
		NSOpenGLPixelFormat *_oglPixelFormat;
		CGLContextObj _cglContext;
#endif
		
#if RN_PLATFORM_IOS
		Context *_shared;
		EAGLContext *_oglContext;
#endif
	};
}

#endif /* __RAYNE_CONTEXT_H__ */
