//
//  RNContext.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXT_H__
#define __RAYNE_CONTEXT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"
#include "RNColor.h"

namespace RN
{
	class Window;
	class Context : public Object
	{
	public:
		friend class Window;
		friend class Kernel;
		friend class Thread;
		
		RNAPI Context(Context *shared=0);
		RNAPI virtual ~Context();

		RNAPI void MakeActiveContext();
		RNAPI void DeactivateContext();
		
		RNAPI void SetDepthClear(GLfloat depth);
		RNAPI void SetStencilClear(GLint stencil);
		RNAPI void SetClearColor(const Color& color);
		
		static Context *ActiveContext();

	protected:
		RNAPI void Activate();
		RNAPI void Deactivate();

	private:
		void ForceDeactivate();
		
		bool _active;
		Thread *_thread;
		Context *_shared;
		bool _firstActivation;
		
		GLfloat _depthClear;
		GLint _stencilClear;
		Color _clearColor;
		
#if RN_PLATFORM_MAC_OS
		void *_oglContext;
		void *_oglPixelFormat;
		void *_cglContext;
#endif

#if RN_PLATFORM_IOS
		void *_oglContext;
#endif

#if RN_PLATFORM_WINDOWS
		HWND CreateOffscreenWindow();

		HWND _hWnd;
		HDC _hDC;
		HGLRC _context;
		int _pixelFormat;
#endif

#if RN_PLATFORM_LINUX
		static Display *_dpy;

		XVisualInfo *_vi;
		GLXContext _context;
		XID _win;
#endif
		
		RNDefineMeta(Context, Object)
	};
}

#endif /* __RAYNE_CONTEXT_H__ */
