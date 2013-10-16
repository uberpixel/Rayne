//
//  RNContextInternal.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CONTEXTINTERNAL_H__
#define __RAYNE_CONTEXTINTERNAL_H__

#include "RNBase.h"
#include "RNBaseInternal.h"

namespace RN
{
	struct ContextInternals
	{
#if RN_PLATFORM_MAC_OS
		NSOpenGLContext *context;
		NSOpenGLPixelFormat *pixelFormat;
		CGLContextObj cglContext;
#endif
		
#if RN_PLATFORM_IOS
		EAGLContext *context;
#endif
		
#if RN_PLATFORM_WINDOWS
		HWND hWnd;
		HDC hDc;
		HGLRC context;
		int pixelFormat;
#endif
		
#if RN_PLATFORM_LINUX
		XVisualInfo vi;
		GLXContext context;
		XID win;
#endif
	};
	
#if RN_PLATFORM_WINDOWS
	static HWND CreateOffscreenWindow()
	{
		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION  | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		
		HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
		RECT windowRect;
		
		windowRect.left   = 0;
		windowRect.right  = 1024;
		windowRect.top    = 0;
		windowRect.bottom = 768;
		
		AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);
		
		HWND desktop = GetDesktopWindow();
		RECT desktopRect;
		
		GetWindowRect(desktop, &desktopRect);
		
		
		LONG desktopWidth  = desktopRect.right - desktopRect.left;
		LONG desktopHeight = desktopRect.bottom - desktopRect.top;
		
		LONG width  = windowRect.right - windowRect.left;
		LONG height = windowRect.bottom - windowRect.top;
		
		windowRect.left = (desktopWidth / 2) - (width / 2);
		windowRect.top  = (desktopHeight / 2) - (height / 2);
		
		HWND hWnd = CreateWindowExA(dwExStyle, "RNWindowClass", "", dwStyle, windowRect.left, windowRect.top, width, height, 0, 0, hInstance, 0);
		return hWnd;
	}
#endif
}

#endif /* __RAYNE_CONTEXTINTERNAL_H__ */
