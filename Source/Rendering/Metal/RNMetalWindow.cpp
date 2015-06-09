//
//  RNMetalWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../../Base/RNBaseInternal.h"
#include "RNMetalWindow.h"

namespace RN
{
	struct MetalWindow::Internals
	{
		NSWindow *rawWindow;
		CAMetalLayer *metalLayer;
	};

	MetalWindow::MetalWindow(const Rect &frame, Screen *screen)
	{
		_internals->rawWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(frame.x, frame.y, frame.width, frame.height) styleMask:NSTitledWindowMask | NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO];
		[_internals->rawWindow setBackgroundColor:[NSColor blackColor]];
		[_internals->rawWindow setIgnoresMouseEvents:NO];
		//[_internals->rawWindow setContentView:_view];
		[_internals->rawWindow makeKeyAndOrderFront:nil];
	}

	void MetalWindow::SetTitle(const String *title)
	{
		[_internals->rawWindow setTitle:[NSString stringWithUTF8String:title->GetUTF8String()]];
	}
}
