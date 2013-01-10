//
//  RNContext.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
		Context(Context *shared=0);		
		virtual ~Context();
		
		void MakeActiveContext();
		void DeactivateContext();
		
		virtual void Flush();
		virtual void SetName(const char *name);
		
		static Context *ActiveContext();
		
	protected:
		void Activate();
		void Deactivate();
		
	private:
		bool _active;
		Thread *_thread;
		
#if RN_PLATFORM_MAC_OS
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
