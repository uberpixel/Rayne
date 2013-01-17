//
//  RNWindow.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WINDOW_H__
#define __RAYNE_WINDOW_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNContext.h"
#include "RNRendererBackend.h"

#if RN_PLATFORM_MAC_OS

namespace RN
{
	class Window;
}

@interface RNNativeWindow : NSWindow
{
@public
	RN::Window *_controller;
	NSOpenGLView *_openGLView;
	CVDisplayLinkRef _displayLink;
}

@end

#endif

#if RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS

namespace RN
{
	class Kernel;
	class Window : public Object
	{
	public:
		Window(const std::string& title, Kernel *kernel);
		virtual ~Window();
		
		void Show();
		void Hide();
		
		void SetContext(Context *context);
		void SetTitle(const std::string& title);
		
		Rect Frame() const;
		
	private:
#if RN_PLATFORM_MAC_OS
		RNNativeWindow *_nativeWindow;
#endif
		
#if RN_PLATFORM_IOS
		UIWindow *_nativeWindow;
		UIViewController *_rootViewController;
		UIView *_renderingView;
#endif
		
		Context *_context;
		Kernel *_kernel;
	};
}

#endif

#if RN_PLATFORM_WINDOWS

namespace RN
{
	class Kernel;
	class Window : public Object
	{
	public:
		RNAPI Window(const std::string& title, Kernel *kernel);
		RNAPI virtual ~Window();

		RNAPI void Show();
		RNAPI void Hide();

		RNAPI void SetContext(Context *context);
		RNAPI void SetTitle(const std::string& title);

		RNAPI Rect Frame() const;

	private:
		std::string _title;

		Context *_context;
		Kernel *_kernel;
		HWND _hWnd;
		HDC _hDC;
	};
}

#endif

#endif /* __RAYNE_WINDOW_H__ */
