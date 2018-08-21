//
//  RNWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindow.h"
#include "../Debug/RNLogger.h"
#include "../Base/RNKernel.h"

#if RN_PLATFORM_MAC_OS
#include <AppKit/AppKit.h>
#endif

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
		uint64 windowHandle = GetWindowHandle();
		if(windowHandle == -1)
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
		NSWindow *window = reinterpret_cast<NSWindow*>(windowHandle);
		CGAssociateMouseAndMouseCursorPosition(false);
		CGWarpMouseCursorPosition(CGPointMake(window.frame.origin.x + window.frame.size.width / 2, window.frame.origin.y + window.frame.size.height / 2));
#elif RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_grab_pointer(connection, 1, windowHandle, 0, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, windowHandle, XCB_NONE, XCB_CURRENT_TIME);
#endif
	}

	void Window::ReleaseMouseCursor()
	{
		uint64 windowHandle = GetWindowHandle();
		if(windowHandle == -1)
		{
			RNDebug("ReleaseMouseCursor is not supported by this window type.");
			return;
		}

#if RN_PLATFORM_WINDOWS
		ClipCursor(nullptr);
#elif RN_PLATFORM_MAC_OS
		CGAssociateMouseAndMouseCursorPosition(true);
#elif RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_ungrab_pointer(connection, XCB_CURRENT_TIME);
#endif
	}

	void Window::ShowMouseCursor()
	{
		uint64 windowHandle = GetWindowHandle();
		if(windowHandle == -1)
		{
			RNDebug("ShowMouseCursor is not supported by this window type.");
			return;
		}

#if RN_PLATFORM_WINDOWS
		ShowCursor(true);
#elif RN_PLATFORM_MAC_OS
		[NSCursor unhide];
#elif RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();
		xcb_change_window_attributes(connection, windowHandle, XCB_CW_CURSOR, nullptr);
#endif
	}

	void Window::HideMouseCursor()
	{
		uint64 windowHandle = GetWindowHandle();
		if(windowHandle == -1)
		{
			RNDebug("HideMouseCursor is not supported by this window type.");
			return;
		}

#if RN_PLATFORM_WINDOWS
		ShowCursor(false);
#elif RN_PLATFORM_MAC_OS
		[NSCursor hide];
#elif RN_PLATFORM_LINUX
		xcb_connection_t *connection = Kernel::GetSharedInstance()->GetXCBConnection();

		xcb_pixmap_t pixmap = xcb_generate_id(connection);
		xcb_create_pixmap(connection, 1, pixmap, windowHandle, 1, 1);

		xcb_cursor_t cursor = xcb_generate_id(connection);
		xcb_create_cursor(connection, cursor, pixmap, pixmap, 0, 0, 0, 0, 0, 0, 1, 1);
		xcb_free_pixmap(connection, pixmap);

		xcb_change_window_attributes(connection, windowHandle, XCB_CW_CURSOR, &cursor);
		xcb_free_cursor(connection, cursor);
#endif
	}
}
