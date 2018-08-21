//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "../Debug/RNLogger.h"

namespace RN
{
	RNDefineMeta(Window, Object)

	Window::Window(Screen *screen) :
		_screen(screen)
	{
		SafeRetain(_screen);
	}

	Window::Window() :
		_screen(nullptr)
	{}

	Window::~Window()
	{
		SafeRelease(_screen);
	}

	void Window::TrapMouseCursor()
	{
		void *windowHandle = GetWindowHandle();
		if(!windowHandle)
		{
			RNDebug("TrapMouseCursor is not supported by this window type.");
			return;
		}

#if RN_PLATFORM_WINDOWS
		HWND hwnd = static_cast<HWND>(windowHandle);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		// Convert the client area to screen coordinates.
		POINT pt = { clientRect.left, clientRect.top };
		POINT pt2 = { clientRect.right, clientRect.bottom };
		ClientToScreen(hwnd, &pt);
		ClientToScreen(hwnd, &pt2);
		SetRect(&clientRect, pt.x + 1, pt.y + 1, pt2.x - 1, pt2.y - 1);

		ClipCursor(&clientRect);
#elif RN_PLATFORM_MAC_OS

#elif RN_PLATFORM_LINUX

#endif
	}

	void Window::ReleaseMouseCursor()
	{
#if RN_PLATFORM_WINDOWS
		ClipCursor(nullptr);

#elif RN_PLATFORM_MAC_OS

#elif RN_PLATFORM_LINUX

#endif
	}

	void Window::ShowMouseCursor()
	{
#if RN_PLATFORM_WINDOWS
		ShowCursor(true);

#elif RN_PLATFORM_MAC_OS

#elif RN_PLATFORM_LINUX

#endif
	}

	void Window::HideMouseCursor()
	{
#if RN_PLATFORM_WINDOWS
		ShowCursor(false);

#elif RN_PLATFORM_MAC_OS

#elif RN_PLATFORM_LINUX

#endif
	}
}
