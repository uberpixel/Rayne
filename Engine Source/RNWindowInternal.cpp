//
//  RNWindowInternal.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWindowInternal.h"
#include "RNInput.h"
#include "RNKernel.h"
#include "RNUIServer.h"
#include "RNLogging.h"

#if RN_PLATFORM_MAC_OS

// ---------------------
// MARK: -
// MARK: NSWindow
// ---------------------

@implementation RNNativeWindow

- (BOOL)windowShouldClose:(id)sender
{
	RN::Kernel::GetSharedInstance()->Exit();
	return NO;
}

- (void)keyDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)keyUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)mouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	RN::Input::GetSharedInstance()->HandleEvent(theEvent);
}


- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}


- (void)performMenuBarAction:(id)sender
{
	RN::UI::Server::GetSharedInstance()->PerformMenuBarAction(sender);
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
	RN::Input::GetSharedInstance()->InvalidateMouse();
}


- (void)setOpenGLContext:(NSOpenGLContext *)context andPixelFormat:(NSOpenGLPixelFormat *)pixelFormat
{
	[_openGLView setOpenGLContext:context];
	[_openGLView setPixelFormat:pixelFormat];
	
	[context setView:_openGLView];
}

- (id)initWithFrame:(NSRect)frame andStyleMask:(NSUInteger)stylemask
{
	if((self = [super initWithContentRect:frame styleMask:stylemask backing:NSBackingStoreBuffered defer:NO]))
	{
		NSRect rect = [self contentRectForFrameRect:frame];
		_openGLView = [[NSOpenGLView alloc] initWithFrame:rect];
		
		[_openGLView setWantsBestResolutionOpenGLSurface:YES];
		
		[self setContentView:_openGLView];
		[self setDelegate:self];
	}
	
	return self;
}

- (void)dealloc
{
	[_openGLView release];
	[super dealloc];
}

@end

#endif

#if RN_PLATFORM_WINDOWS

namespace RN
{
	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch(message)
		{
			case WM_CLOSE:
				Kernel::GetSharedInstance()->Exit();
				return 0;

			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_MOUSEMOVE:
			case WM_KEYDOWN:
			case WM_KEYUP:
				RN::Input::GetSharedInstance()->HandleSystemEvent(window, message, wparam, lparam);
				return 0;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				switch(wparam)
				{
					case VK_MENU:
						return 0;
				}

				break;

			case WM_COMMAND:
			{
				uint32 command = (wparam & 0xffff);
				RN::UI::Server::GetSharedInstance()->PerformMenuCommand(command);
				break;
			}
		}

		return DefWindowProcW(window, message, wparam, lparam);
	}
}
#endif
