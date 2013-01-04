//
//  RNWindow.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
	class Window : public Object
	{
	public:
		Window(const std::string& title);
		virtual ~Window();
		
		void Show();
		void Hide();
		
		void SetContext(Context *context, RendererBackend *renderer);
		void SetTitle(const std::string& title);
		
#if RN_PLATFORM_IOS
		void DrawFrame();
#endif
		
	private:
#if RN_PLATFORM_MAC_OS
		RNNativeWindow *_nativeWindow;
#endif
		
#if RN_PLATFORM_IOS
		UIWindow *_nativeWindow;
		UIViewController *_rootViewController;
#endif
		Context *_context;
	};
}

#endif

#endif /* __RAYNE_WINDOW_H__ */
